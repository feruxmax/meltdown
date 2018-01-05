meltdown: main.cpp
	g++ -O0 -msse2 -std=c++11 main.cpp -o meltdown

clean:meltdown
	rm meltdown

all: meltdown
