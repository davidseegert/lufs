#ifndef ANALYZER_H
#define ANALYZER_H

#include <vector>
#include <cmath>
#include <algorithm>

namespace lufs {

struct BiquadCoeffs {
    double b0, b1, b2, a1, a2;
};

class Biquad {
public:
    Biquad() : x1(0), x2(0), y1(0), y2(0) {}
    void setCoeffs(const BiquadCoeffs& c) { coeffs = c; }
    double process(double x) {
        double y = coeffs.b0 * x + coeffs.b1 * x1 + coeffs.b2 * x2 - coeffs.a1 * y1 - coeffs.a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
    void reset() { x1 = x2 = y1 = y2 = 0; }

private:
    BiquadCoeffs coeffs;
    double x1, x2, y1, y2;
};

class KWeightingFilter {
public:
    KWeightingFilter(double sampleRate);
    double process(double x);
    void reset();

private:
    Biquad stage1;
    Biquad stage2;
};

class TruePeakDetector {
public:
    TruePeakDetector();
    void process(double x);
    double getTruePeak() const { return maxAbs; }
    void reset();

private:
    double maxAbs;
    std::vector<double> history;
    size_t historyIdx;
};

class LoudnessAnalyzer {
public:
    LoudnessAnalyzer(int numChannels, double sampleRate);
    void process(const float* buffer, size_t numFrames);
    double getIntegratedLoudness();
    double getLoudnessRange();
    double getTruePeak();

private:
    struct Block {
        std::vector<double> channelMeanSquares;
        double loudness;
    };

    int numChannels;
    double sampleRate;
    std::vector<KWeightingFilter> filters;
    std::vector<TruePeakDetector> tpDetectors;
    std::vector<double> channelWeights;

    // Parameters for 400ms blocks
    size_t blockSizeFrames;
    size_t hopSizeFrames;
    
    // Buffer for carrying over samples for blocks
    std::vector<std::vector<float>> sampleBuffers;
    
    // Accumulators for mean square calculation
    std::vector<Block> blocks;

    void calculateLoudnessForBlock(const std::vector<std::vector<float>>& blockSamples);
};

} // namespace lufs

#endif
