#include <iostream>
#include <vector>
#include <sndfile.h>
#include <cmath>
#include <fstream>

std::vector<float> interpolate(std::vector<float> samples, std::vector<float> derivative){
    std::vector<float> interpolatedSamples(samples.size());
    interpolatedSamples = samples;
    for(int i = 1; i < derivative.size(); i++){
        if(derivative[i] < 1e-5f){
            interpolatedSamples[i] = 0.5f * samples[i - 1] + samples[i + 1];
        }
    }
    return interpolatedSamples;
}

std::vector<float> derivativeFunction(std::vector<float> samples){
    std::vector<float> derivative(samples.size() - 1);
    for(int i = 1; i < samples.size(); i++){
        derivative[i] = samples[i] - samples[i - 1];
    }
    return derivative;
}

std::vector<float> monoFunction(int frames, std::vector<float> samples){
    std::vector<float> mono(frames);
    for(int i = 0; i < frames; i++){
        mono[i] = 0.5f * (samples[i * 2] + samples[i * 2 + 1]);
    }
    std::cout << samples.size() << " samples converted to " << mono.size() << " single channel samples. \n";
    return mono;
}

void removeLeadingZeros(std::vector<float>& samples){
    int startIndex = 0;
    for(int i = 0; i < samples.size(); i++){
        if(std::abs(samples[i]) > 1e-5f){
            startIndex = i;
            break;
        } 
    }
    samples.erase(samples.begin(), samples.begin() + startIndex);
}

void exportCSV(const std::string& filename, const std::vector<float>& data, size_t maxSamples = 10000) {
    std::ofstream file(filename);
    size_t count = std::min(maxSamples, data.size());

    for (size_t i = 0; i < count; ++i) {
        file << data[i] << "\n";
    }

    file.close();
}

int main() {
    const char* sourceFile = "samples/ethereal-vistas.wav";
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
    sf_read_float(file, rawSamples.data(), rawSamples.size());

    if(channels > 1){
        std::vector<float> monoSamples = monoFunction(frames, rawSamples);
        rawSamples = monoSamples;
    } 
    removeLeadingZeros(rawSamples);
    std::vector<float> derivative = derivativeFunction(rawSamples);
    std::vector<float> interpolatedSamples = interpolate(rawSamples, derivative);

    /*for(int i = 1000; i < 1100; i++){
        std::cout << "Sample " << i << ": " << rawSamples[i] << "\n";
    }
    for(int i = 1000; i < 1100; i++){
        std::cout << "Derivative " << i << ": " << derivative[i] << "\n";
    }
    for(int i = 1000; i < 1100; i++){
        std::cout << "Interpolated Sample " << i << ": " << interpolatedSamples[i] << "\n";
    }*/

    sf_close(file);

    exportCSV("original.csv", rawSamples);
    exportCSV("interpolated.csv", interpolatedSamples);
    exportCSV("derivative.csv", derivative);

    return 0;
}