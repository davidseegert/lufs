#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio.h"

#include "analyzer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

bool isSupportedFile(const fs::path& path) {
    if (!fs::is_regular_file(path)) return false;
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg" || ext == ".aiff" || ext == ".m4a" || ext == ".mp4");
}

void processAudioFile(const fs::path& path, bool showLufs, bool showRange, bool isDirectoryMode, const fs::path& baseDir) {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(path.string().c_str(), &config, &decoder);
    if (result != MA_SUCCESS) {
        return; // Silently skip unsupported or corrupt files in directory mode
    }

    ma_uint32 numChannels = decoder.outputChannels;
    ma_uint32 sampleRate = decoder.outputSampleRate;

    lufs::LoudnessAnalyzer analyzer(numChannels, sampleRate);

    const ma_uint32 bufferSizeFrames = 4096;
    std::vector<float> buffer(bufferSizeFrames * numChannels);

    ma_uint64 totalFramesRead = 0;
    while (true) {
        ma_uint64 framesRead;
        result = ma_decoder_read_pcm_frames(&decoder, buffer.data(), bufferSizeFrames, &framesRead);
        if (result != MA_SUCCESS || framesRead == 0) {
            break;
        }
        analyzer.process(buffer.data(), (size_t)framesRead);
        totalFramesRead += framesRead;
    }

    ma_decoder_uninit(&decoder);

    if (totalFramesRead > 0) {
        double integratedLoudness = analyzer.getIntegratedLoudness();
        double loudnessRange = analyzer.getLoudnessRange();

        std::string displayPath = isDirectoryMode ? fs::relative(path, baseDir).string() : path.filename().string();

        std::cout << std::fixed << std::setprecision(1);
        if (showLufs && showRange) {
            std::cout << displayPath << std::endl;
            std::cout << "LUFS: " << integratedLoudness << std::endl;
            std::cout << "LRA:    " << loudnessRange << std::endl << std::endl;
        } else if (showLufs) {
            if (isDirectoryMode) {
                std::cout << displayPath << " " << integratedLoudness << std::endl;
            } else {
                std::cout << integratedLoudness << std::endl;
            }
        } else if (showRange) {
            if (isDirectoryMode) {
                std::cout << displayPath << " " << loudnessRange << std::endl;
            } else {
                std::cout << loudnessRange << std::endl;
            }
        }
    }
}

void printHelp() {
    std::cout << "Usage: lufs [options] <input_file_or_directory>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -l    Print only the Integrated Loudness (LUFS)" << std::endl;
    std::cout << "  -r    Print only the Loudness Range (LU Range)" << std::endl;
    std::cout << "  -h    Print this help text" << std::endl;
    std::cout << std::endl;
    std::cout << "Guidelines:" << std::endl;
    std::cout << std::endl;
    std::cout << "Podcast / Spoken Word:" << std::endl;
    std::cout << "LUFS: -16 (Stereo)" << std::endl;
    std::cout << "      -19 (Mono)" << std::endl;
    std::cout << "LRA: 3 - 7" << std::endl;
    std::cout << std::endl;
    std::cout << "Music: " << std::endl;
    std::cout << "LUFS: -14" << std::endl;
    std::cout << "LRA: 4 - 8" << std::endl;
    std::cout << std::endl;
    std::cout << "Loudness Range (LRA) might vary on genre:" << std::endl;
    std::cout << "EDM / Club               2 - 5" << std::endl;
    std::cout << "Metal / Hardcore         3 - 5" << std::endl;
    std::cout << "Pop / Modern R&B         4 - 7" << std::endl;
    std::cout << "Hip-Hop / Rap            4 - 7" << std::endl;
    std::cout << "Modern Rock / Indie      5 - 8" << std::endl;
    std::cout << "Acoustic / Folk          6 - 10" << std::endl;
    std::cout << "Jazz / Blues             8 - 14" << std::endl;
    std::cout << "Classical / Cinematic   12 - 20+." << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::string inputPathStr = "";
    bool showLufs = false;
    bool showRange = false;
    bool showHelp = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-l") {
            showLufs = true;
        } else if (arg == "-r") {
            showRange = true;
        } else if (arg == "-h" || arg == "--help") {
            showHelp = true;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            printHelp();
            return 1;
        } else {
            inputPathStr = arg;
        }
    }

    if (showHelp) {
        printHelp();
        return 0;
    }

    if (inputPathStr.empty()) {
        std::cerr << "Error: No input specified" << std::endl;
        printHelp();
        return 1;
    }

    fs::path inputPath(inputPathStr);
    if (!fs::exists(inputPath)) {
        std::cerr << "Error: Path does not exist: " << inputPathStr << std::endl;
        return 1;
    }

    if (!showLufs && !showRange) {
        showLufs = true;
        showRange = true;
    }

    if (fs::is_directory(inputPath)) {
        std::vector<fs::path> files;
        for (const auto& entry : fs::recursive_directory_iterator(inputPath)) {
            if (isSupportedFile(entry.path())) {
                files.push_back(entry.path());
            }
        }
        
        std::sort(files.begin(), files.end());

        for (const auto& filePath : files) {
            processAudioFile(filePath, showLufs, showRange, true, inputPath);
        }
    } else {
        processAudioFile(inputPath, showLufs, showRange, false, inputPath.parent_path());
    }

    return 0;
}
