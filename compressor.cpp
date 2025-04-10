#include <iostream>
#include <vector>
#include <sndfile.h>
#include <cmath>
#include <fstream>
#include <algorithm>

#include "compressor.h"

/*
* Function: MonoFunction
* Desc: Averages out the left and right channels of a signal to simplify processing
*/
std::vector<float> monoFunction(int frames, std::vector<float> samples){
    std::vector<float> mono(frames);
    for(int i = 0; i < frames; i++){
        mono[i] = 0.5f * (samples[i * 2] + samples[i * 2 + 1]);
    }
    std::cout << samples.size() << " samples converted to " << mono.size() << " single channel samples. \n";
    return mono;
}

/*
* Func: RemoveLeadingZeros
* Desc: Removes all leading zeros from the audio to make it start sooner
*/
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

/*
* Func: ExportCSV
* Desc: Exports CSV files to be read, graphed and analyzed
*/
void exportCSV(const std::string& filename, const std::vector<float>& data, size_t maxSamples) {
    std::ofstream file(filename);
    size_t count = std::min(maxSamples, data.size());

    for (size_t i = 0; i < count; ++i) {
        file << data[i] << "\n";
    }

    file.close();
}

/*
* Function: FFT
* Desc: Takes the fourier transform to turn time domain signal into its frequency components
*/
std::vector<kiss_fft_cpx> FFT(const std::vector<float>& samples){
    const int N = 2048;
    kiss_fft_cfg cfg = kiss_fft_alloc(N, 0, NULL, NULL);

    std::vector<kiss_fft_cpx> in(N);
    std::vector<kiss_fft_cpx> out(N);

    for(int i = 0; i < N; i++){
        in[i].r = ( i < samples.size() ? samples[i] : 0);
        in[i].i = 0.0f;
    }

    kiss_fft(cfg, in.data(), out.data());

    free(cfg);

    return out;
}

/*
* Function: IFFT
* Desc: Takes the inverse laplace transform to reproduce time domain signal
*/
std::vector<float> IFFT(const std::vector<kiss_fft_cpx>& fft){
    const int N = 2048;

    std::vector<kiss_fft_cpx> in(N);
    std::vector<float> timeDomain(N);

    kiss_fft_cfg ifftCfg = kiss_fft_alloc(N, 1, NULL, NULL);

    kiss_fft(ifftCfg, fft.data(), in.data());

    for(int i = 0; i < N; i++){
        timeDomain[i] = in[i].r / N;
    }

    free(ifftCfg);

    return timeDomain;
}

/*
* Funtion: LowPass
* Desc: Removes all frequency components above 9Khz to allow for lower sample rate
*/
void lowPass(std::vector<kiss_fft_cpx>& out, int sampleRate, float cutoffFrequency){
    int resolution = static_cast<float>(sampleRate / 2048);;
    int cutoffBin = static_cast<int>(cutoffFrequency / resolution);


    for(int i = cutoffBin; i < 512; i++){
        out[i].r = 0.0f;
        out[i].i = 0.0f;
    }
}

void keepTopFreq(std::vector<kiss_fft_cpx>& out){
    int topTenThreshold = out.size() * 0.1;
    std::vector<float> magnitude(out.size());
    std::vector<kiss_fft_cpx> topTenPercent(topTenThreshold);

    for(int i = 0; i < out.size(); i++){
        magnitude[i] = std::sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
    }
    std::sort(magnitude.begin(), magnitude.end(), std::greater<float>());

    float threshold = magnitude[magnitude.size() / 10];

    for(int i = 0; i < out.size(); i++){
        float singleMagnitude = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
        if(singleMagnitude < threshold){
            out[i].r = 0.0f;
            out[i].i = 0.0f;
        }
    }
}

int countNonZeroBins(const std::vector<kiss_fft_cpx>& fft) {
    int count = 0;
    for (const auto& bin : fft) {
        if (std::abs(bin.r) > 1e-6 || std::abs(bin.i) > 1e-6) {
            count++;
        }
    }
    return count;
}

void normalizeAudio(std::vector<float>& samples){
    float maxValue = 0.0f;
    for(int i = 0; i < samples.size(); i++){
        if(std::abs(samples[i]) > maxValue){
            maxValue = std::abs(samples[i]);
        }
    }
    if(maxValue > 0.0f){
        float scale = 0.99f / maxValue;
        for(int i = 0; i < samples.size(); i++){
            samples[i] *= scale;
        }
    }
}