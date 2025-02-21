all: tcpselect tcpepoll tcpepoll1

tcpselect: tcpselect.cpp
	g++ -o tcpselect tcpselect.cpp -g

tcpepoll: tcpepoll.cpp
	g++ -o tcpepoll tcpepoll.cpp -g

tcpepoll1: tcpepoll1.cpp
	g++ -o tcpepoll1 tcpepoll1.cpp -g

clean: 
	rm -rf tcpselect tcpepoll tcpepoll1