CXX = g++
CXXFLAGS = -Wall -pedantic -pthread -g -std=c++17 -O3 -I /usr/local/include -L /usr/local/lib/
LIBS = -loscpack -lrtmidi

all:
	$(CXX) $(CXXFLAGS) src/main.cpp -o tidalrytm $(LIBS)
	@echo "finished building main"

run:
	./tidalrytm
