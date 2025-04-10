CXX = clang++
CXXFLAGS = -std=c++17 -I./kissfft -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lsndfile

SRC = main.cpp kissfft/kiss_fft.c
OUT = wavloader

all:
	$(CXX) $(SRC) -o $(OUT) $(CXXFLAGS) $(LDFLAGS)
