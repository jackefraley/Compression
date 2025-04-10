#include <iostream>
#include <vector>
#include <sndfile.h>
#include <cmath>
#include <fstream>

#include "kissfft/kiss_fft.h"


/*
* Funtion: LowPass
* Desc: Removes all frequency components above 9Khz to allow for lower sample rate
*/
void lowPass(std::vector<kiss_fft_cpx>& out, int sampleRate){
    int resolution = static_cast<float>(sampleRate / 1024);
    float cutoffFreq = 9000.0f;
    int cutoffBin = static_cast<int>(cutoffFreq / resolution);

    for(int i = cutoffBin; i < 512; i++){
        out[i].r = 0.0f;
        out[i].i = 0.0f;
    }
}

/*
* Function: IFFT
* Desc: Takes the inverse laplace transform to reproduce time domain signal
*/
std::vector<float> IFFT(const std::vector<kiss_fft_cpx>& fft){
    const int N = 1024;

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
* Function: FFT
* Desc: Takes the fourier transform to turn time domain signal into its frequency components
*/
std::vector<kiss_fft_cpx> FFT(const std::vector<float>& samples){
    const int N = 1024;
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
* Function: Interpolate
* Desc: If the derivative of the incoming signal is less than 1e-5 this averages it with the 
* samples before and after it to smooth out small changes which are unimportant
*/
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

/*
* Function: DerivativeFunction
* Desc: Finds the finite difference approximation derivative of a signal 
* using the simple formula d[i] = s[i] - s[i-1]
*/
std::vector<float> derivativeFunction(std::vector<float> samples){
    std::vector<float> derivative(samples.size() - 1);
    for(int i = 1; i < samples.size(); i++){
        derivative[i] = samples[i] - samples[i - 1];
    }
    return derivative;
}

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

    std::vector<float> compressedAudio;

    for(int i = 0; i < interpolatedSamples.size(); i += 1024){
        std::vector<float> block(1024, 0.0f);
        for(int j = 0; j < 1024 && (i + j) < interpolatedSamples.size(); j++){
            block[j] = interpolatedSamples[i + j];
        }

        std::vector<kiss_fft_cpx> FFTOut = FFT(block);
        lowPass(FFTOut, sampleRate);
        std::vector<float> filteredBlock = IFFT(FFTOut);

        compressedAudio.insert(compressedAudio.end(), filteredBlock.begin(), filteredBlock.end());
    }

    std::vector<float> downsampled;

    for(int i = 0; i < compressedAudio.size(); i += 2){
        downsampled.push_back(compressedAudio[i]);
    }

    SF_INFO outInfo = sfInfo;
    outInfo.samplerate = sampleRate/2;
    outInfo.channels = 1;
    outInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* outFile = sf_open("compressed_audio_9k_cutoff.wav", SFM_WRITE, &outInfo);
    sf_write_float(outFile, downsampled.data(), downsampled.size());

    sf_close(outFile);
    sf_close(file);

    exportCSV("original.csv", rawSamples);
    exportCSV("interpolated.csv", interpolatedSamples);
    exportCSV("derivative.csv", derivative);
    exportCSV("compressed.csv", compressedAudio);

    return 0;
}