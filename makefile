# Simple makefile

all: main.cpp
	g++ -std=c++14 -Ofast -Wall main.cpp -lcurl -lssl -lcrypto -o simple_wget

debug: main.cpp
	g++ -std=c++14 -g -Wall main.cpp -lcurl -lssl -lcrypto -o simple_wget

test: all
	cat test.txt | xargs ./simple_wget

clean:
	find . -type f -not -name "main.cpp" -not -name "makefile" -not -name "test.txt" -delete
