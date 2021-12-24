CXX = g++
CXXFLAGS = -Wall -pedantic -pthread -std=c++17 -O3 -I /opt/homebrew/include -I /usr/local/include -L /opt/homebrew/lib/ -L /usr/local/lib/
LIBS = -loscpack -lrtmidi

all:
	$(CXX) $(CXXFLAGS) main.cpp -o tidalrytm $(LIBS)
	@echo "finished building main"

run:
	./tidalrytm
