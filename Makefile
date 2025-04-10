# Compiler
CXX = clang++

# Compiler Flags
CXXFLAGS = -std=c++17 -I./kissfft -I/opt/homebrew/include

# Linker Flags
LDFLAGS = -L/opt/homebrew/lib -lsndfile

# Source files
SRC = main.cpp compressor.cpp kissfft/kiss_fft.c

# Output binary name
OUT = wavloader

# Default target
all:
	$(CXX) $(SRC) $(CXXFLAGS) $(LDFLAGS) -o $(OUT)
