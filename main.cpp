#include <iostream>
#include <vector>
#include <sndfile.h>
#include <cmath>
#include <fstream>

#include "compressor.h"


int main() {

    float frequencyCutoff = 0;
    int skipFactor = 0;

    while(true){
        std::cout << "Enter compression quality:\n(1 for 92% reduction, 2 for 83% reduction, 3 for 66% reduction and 4 for 50% reduction)\n";
        int userInput = 0;

        std::cin >> userInput;

        if(userInput < 1 || userInput > 4){
            std::cerr << "Enter a valid number\n";
        } else if(userInput == 1){
            frequencyCutoff = 1320.0f;
            skipFactor = 14;
            break;
        } else if(userInput == 2){
            frequencyCutoff = 4000.0f;
            skipFactor = 6;
            break;
        } else if(userInput == 3){
            frequencyCutoff = 7920.0f;
            skipFactor = 3;
            break;
        } else if(userInput == 4){
            frequencyCutoff = 9000.0f;
            skipFactor = 2;
            break;
        }
    }

    const int N = 2048;

    const char* sourceFile = "samples/macMiller.wav";
    SF_INFO sfInfo;
    SNDFILE* file = sf_open(sourceFile, SFM_READ, &sfInfo);

    if(!file){
        std::cerr << "Could not open file" << sourceFile << "\n";
        return 1;
    }

    int sampleRate = sfInfo.samplerate;
    int channels = sfInfo.channels;
    int frames = sfInfo.frames;

    if(!sampleRate || !channels || !frames){
        if(!sampleRate){
            std::cerr << "Sample Rate Error.\n";
        }
        if(!channels){
            std::cerr << "Channel Error.\n";
        }
        if(!frames){
            std::cerr << "Frame Error.\n";
        }
        return 1;
    }

    std::cout << "Sample Rate: " << sampleRate << "\n";
    std::cout << "Channels: " << channels << "\n";
    std::cout << "Frames: " << frames << "\n";

    std::vector<float> rawSamples(channels * frames);
    int preMonoSize = rawSamples.size();
    sf_read_float(file, rawSamples.data(), rawSamples.size());

    if(channels > 1){
        std::vector<float> monoSamples = monoFunction(frames, rawSamples);
        rawSamples = monoSamples;
    } 

    removeLeadingZeros(rawSamples);

    std::vector<float> compressedAudio(rawSamples.size(), 0.0f);

    std::vector<float> window(N);

    // Create Hann window
    for (int i = 0; i < N; ++i) {
        window[i] = 0.5f * (1 - cos(2 * M_PI * i / (N - 1)));
    }

    int totalBefore = 0;
    int totalAfter = 0;

    // For each 1024 chunk, increment by 512
    for(int i = 0; i + N <= rawSamples.size(); i += (N/2)){
        // Create a block
        std::vector<float> block(N);
        // Apply Hann window to each block
        for(int j = 0; j < N; j++){
            block[j] = rawSamples[i + j] * window[j];
        }

        // Use the FFT to remove higher less important frequencies
        std::vector<kiss_fft_cpx> FFTOut = FFT(block);
        totalBefore += countNonZeroBins(FFTOut);
        keepTopFreq(FFTOut);
        totalAfter += countNonZeroBins(FFTOut);
        lowPass(FFTOut, sampleRate, frequencyCutoff);
        std::vector<float> filteredBlock = IFFT(FFTOut);

        // Add the filtered block to the audio output
        for(int j = 0; j < N; j++){
            compressedAudio[j + i] += filteredBlock[j];
        }
    }

    std::cout << "Non zero bins before: " << totalBefore << " converted to " << totalAfter << " non zero bins.\n";

    // File for downsizing
    std::vector<float> downsampled;

    // Downsample by skipping every other sample, fft cuts out high frequencies to prevent aliasing
    for(int i = 0; i < compressedAudio.size(); i += skipFactor){
        downsampled.push_back(compressedAudio[i]);
    }

    float percentageReduction = (1 - ( static_cast<float>(downsampled.size()) / static_cast<float>(rawSamples.size()))) * 100;

    std::cout << rawSamples.size() << " samples downsampled to " << downsampled.size() << " samples that is a " << percentageReduction << "% reduction in size!\n";

    if(channels > 1){
        percentageReduction = (1 - ( static_cast<float>(downsampled.size()) / static_cast<float>(preMonoSize))) * 100;
        std::cout << "Or " << preMonoSize << " multi channel samples downsampled to " << downsampled.size() << " samples that is a " << percentageReduction << "% reduction in size!\n";
    }

    normalizeAudio(downsampled);


    SF_INFO outInfo = sfInfo;
    outInfo.samplerate = sampleRate/skipFactor;
    outInfo.channels = 1;
    outInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;


    SNDFILE* outFile = sf_open("output/downsampled_mac.wav", SFM_WRITE, &outInfo);
    sf_write_float(outFile, downsampled.data(), downsampled.size());

    sf_close(outFile);
    sf_close(file);

    exportCSV("original.csv", rawSamples);
    exportCSV("compressed.csv", compressedAudio);

    return 0;
}