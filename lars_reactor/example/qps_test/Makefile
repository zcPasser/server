CXX=g++
CFLAGS=-g -O2 -Wall -fPIC -Wno-deprecated -pthread

INC=-I../../include
LIB=-L../../lib -lreactor -lprotobuf -pthread
OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o server $(CFLAGS) echoMessage.pb.cc server.cpp $(INC) $(LIB)
	$(CXX) -o client $(CFLAGS) echoMessage.pb.cc client.cpp $(INC) $(LIB)

clean:
	-rm -f *.o server
	-rm -f *.o client


