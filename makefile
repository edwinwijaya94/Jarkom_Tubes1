all: ./checksum.cpp ./receiver_main.cpp ./receiver.cpp ./transmitter.cpp
	g++ -o receiver.out checksum.cpp receiver.cpp receiver_main.cpp -lpthread
	g++ -o ./transmitter.out checksum.cpp transmitter.cpp -lpthread -std=c++11
	
