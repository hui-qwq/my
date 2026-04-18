CXXFLAGS = -std=c++17
LIBS = -levent

all:server client

server:server.cpp
	g++ $(CXXFLAGS) server.cpp -o server $(LIBS)

client:client.cpp
	g++ $(CXXFLAGS) client.cpp -o client $(LIBS)

clear:
	rm -f server client