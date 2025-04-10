#include <iostream>
#include <vector>
#include <sndfile.h>
#include <cmath>
#include <fstream>

#include "kissfft/kiss_fft.h"

std::vector<float> monoFunction(int frames, std::vector<float> samples);

void removeLeadingZeros(std::vector<float>& samples);

void exportCSV(const std::string& filename, const std::vector<float>& data, size_t maxSamples = 10000);

std::vector<kiss_fft_cpx> FFT(const std::vector<float>& samples);

std::vector<float> IFFT(const std::vector<kiss_fft_cpx>& fft);

void lowPass(std::vector<kiss_fft_cpx>& out, int sampleRate, float cutoffFrequency);

void keepTopFreq(std::vector<kiss_fft_cpx>& out);

int countNonZeroBins(const std::vector<kiss_fft_cpx>& fft);

void normalizeAudio(std::vector<float>& samples);

