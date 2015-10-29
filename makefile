all: ./checksum.cpp ./receiver_main.cpp ./receiver.cpp ./transmitter.cpp
	g++ -o receiver.out checksum.cpp receiver_main.cpp -pthread
	g++ -o ./transmitter.out checksum.cpp transmitter.cpp -pthread -std=c++11
	
