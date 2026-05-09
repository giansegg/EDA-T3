CXX      = g++
CXXFLAGS = -O3 -march=native -std=c++17

bench: bench.cpp
	$(CXX) $(CXXFLAGS) -o bench bench.cpp

run: bench
	./bench

clean:
	rm -f bench

.PHONY: run clean
