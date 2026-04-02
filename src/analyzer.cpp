#include "analyzer.h"
#include <iostream>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <vector>

namespace lufs {

KWeightingFilter::KWeightingFilter(double sampleRate) {
    BiquadCoeffs s1, s2;
    if (std::abs(sampleRate - 48000.0) < 1.0) {
        // Stage 1 (48 kHz) - Head Model
        s1 = {1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585};
        // Stage 2 (48 kHz) - RLB
        s2 = {1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621};
    } else if (std::abs(sampleRate - 44100.0) < 1.0) {
        // Stage 1 (44.1 kHz)
        s1 = {1.5309095994509831, -2.651771422231242, 1.1690820038815147, -1.6637221141740625, 0.7125301614643149};
        // Stage 2 (44.1 kHz)
        s2 = {1.0, -2.0, 1.0, -1.990444327070211, 0.9904722389417079};
    } else {
        // Generic fallback to 48k coeffs (best effort)
        s1 = {1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585};
        s2 = {1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621};
    }
    stage1.setCoeffs(s1);
    stage2.setCoeffs(s2);
}

double KWeightingFilter::process(double x) {
    return stage2.process(stage1.process(x));
}

void KWeightingFilter::reset() {
    stage1.reset();
    stage2.reset();
}

LoudnessAnalyzer::LoudnessAnalyzer(int numChannels, double sampleRate)
    : numChannels(numChannels), sampleRate(sampleRate) {
    blockSizeFrames = static_cast<size_t>(0.4 * sampleRate);
    hopSizeFrames = static_cast<size_t>(0.1 * sampleRate); // 75% overlap means step is 25%, i.e. 100ms
    
    for (int i = 0; i < numChannels; ++i) {
        filters.emplace_back(sampleRate);
        sampleBuffers.emplace_back();
    }

    // Default weights (ITU-R BS.1770-5)
    channelWeights.resize(numChannels, 1.0);
    // Standard 5.1 mapping: L, R, C, LFE, Ls, Rs
    if (numChannels == 6) {
        channelWeights[3] = 0.0;  // LFE
        channelWeights[4] = 1.41; // Ls
        channelWeights[5] = 1.41; // Rs
    }
}

void LoudnessAnalyzer::process(const float* buffer, size_t numFrames) {
    for (size_t f = 0; f < numFrames; ++f) {
        for (int c = 0; c < numChannels; ++c) {
            double filtered = filters[c].process(buffer[f * numChannels + c]);
            sampleBuffers[c].push_back(static_cast<float>(filtered));
        }

        while (sampleBuffers[0].size() >= blockSizeFrames) {
            std::vector<std::vector<float>> blockSamples(numChannels);
            for (int c = 0; c < numChannels; ++c) {
                blockSamples[c].assign(sampleBuffers[c].begin(), sampleBuffers[c].begin() + blockSizeFrames);
                sampleBuffers[c].erase(sampleBuffers[c].begin(), sampleBuffers[c].begin() + hopSizeFrames);
            }
            calculateLoudnessForBlock(blockSamples);
        }
    }
}

void LoudnessAnalyzer::calculateLoudnessForBlock(const std::vector<std::vector<float>>& blockSamples) {
    Block b;
    b.channelMeanSquares.resize(numChannels);
    double weightedSum = 0;
    
    for (int c = 0; c < numChannels; ++c) {
        double ms = 0;
        for (float val : blockSamples[c]) {
            ms += static_cast<double>(val) * val;
        }
        ms /= blockSizeFrames;
        b.channelMeanSquares[c] = ms;
        weightedSum += channelWeights[c] * ms;
    }

    if (weightedSum > 0) {
        b.loudness = -0.691 + 10.0 * std::log10(weightedSum);
    } else {
        b.loudness = -1000.0;
    }
    blocks.push_back(b);
}

double LoudnessAnalyzer::getIntegratedLoudness() {
    if (blocks.empty()) return -1000.0;

    double gammaA = -70.0;
    std::vector<size_t> absGatedIndices;
    for (size_t j = 0; j < blocks.size(); ++j) {
        if (blocks[j].loudness > gammaA) {
            absGatedIndices.push_back(j);
        }
    }

    if (absGatedIndices.empty()) return -70.0;

    std::vector<double> channelMS_AbsGated(numChannels, 0.0);
    for (size_t idx : absGatedIndices) {
        for (int c = 0; c < numChannels; ++c) {
            channelMS_AbsGated[c] += blocks[idx].channelMeanSquares[c];
        }
    }
    
    double weightedSum_AbsGated = 0;
    for (int c = 0; c < numChannels; ++c) {
        weightedSum_AbsGated += channelWeights[c] * (channelMS_AbsGated[c] / absGatedIndices.size());
    }
    
    double l_AbsGated = -0.691 + 10.0 * std::log10(weightedSum_AbsGated);
    double gammaR = l_AbsGated - 10.0;

    std::vector<size_t> finalGatedIndices;
    for (size_t idx : absGatedIndices) {
        if (blocks[idx].loudness > gammaR) {
            finalGatedIndices.push_back(idx);
        }
    }

    if (finalGatedIndices.empty()) return l_AbsGated;

    std::vector<double> channelMS_Final(numChannels, 0.0);
    for (size_t idx : finalGatedIndices) {
        for (int c = 0; c < numChannels; ++c) {
            channelMS_Final[c] += blocks[idx].channelMeanSquares[c];
        }
    }

    double weightedSum_Final = 0;
    for (int c = 0; c < numChannels; ++c) {
        weightedSum_Final += channelWeights[c] * (channelMS_Final[c] / finalGatedIndices.size());
    }

    return -0.691 + 10.0 * std::log10(weightedSum_Final);
}

double LoudnessAnalyzer::getLoudnessRange() {
    int blocksPer3s = 30; // 3 seconds / 0.1s step = 30 blocks
    if (blocks.size() < static_cast<size_t>(blocksPer3s)) return 0.0;

    std::vector<double> stLoudness;
    for (size_t i = 0; i <= blocks.size() - blocksPer3s; ++i) {
        std::vector<double> chMS(numChannels, 0.0);
        for (int j = 0; j < blocksPer3s; ++j) {
            for (int c = 0; c < numChannels; ++c) {
                chMS[c] += blocks[i + j].channelMeanSquares[c];
            }
        }
        
        double weightedSum = 0;
        for (int c = 0; c < numChannels; ++c) {
            weightedSum += channelWeights[c] * (chMS[c] / blocksPer3s);
        }
        
        if (weightedSum > 1e-12) {
            stLoudness.push_back(-0.691 + 10.0 * std::log10(weightedSum));
        }
    }

    if (stLoudness.empty()) return 0.0;

    std::vector<double> absGated;
    for (double l : stLoudness) {
        if (l > -70.0) absGated.push_back(l);
    }
    if (absGated.empty()) return 0.0;

    double sumLIN = 0;
    for (double l : absGated) {
        sumLIN += std::pow(10.0, l / 10.0);
    }
    double l_AbsGated = 10.0 * std::log10(sumLIN / absGated.size());
    double gammaR = l_AbsGated - 20.0;

    std::vector<double> finalGated;
    for (double l : absGated) {
        if (l > gammaR) finalGated.push_back(l);
    }

    if (finalGated.size() < 2) return 0.0;

    std::sort(finalGated.begin(), finalGated.end());
    
    double p10 = finalGated[static_cast<size_t>(0.1 * (finalGated.size() - 1))];
    double p95 = finalGated[static_cast<size_t>(0.95 * (finalGated.size() - 1))];

    return p95 - p10;
}

} // namespace lufs
