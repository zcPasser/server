TARGET= lars_dns_test
CXX=g++
CFLAGS=-g -O2 -Wall -Wno-deprecated

BASE=../../base
BASE_H=$(BASE)/include

PROTO=$(BASE)/proto
PROTO_H =$(PROTO)

LARS_REACTOR=../../lars_reactor
LARS_REACTOR_H =$(LARS_REACTOR)/include
LARS_REACTOR_LIB=$(LARS_REACTOR)/lib  -lreactor

# MYSQL=$(BASE)/mysql-connector-c
# MYSQL_H=$(MYSQL)/include
# MYSQL_LIB=$(MYSQL)/lib/libmysqlclient.a

OTHER_LIB = -pthread -ldl -lprotobuf

# SRC= ./src
INC= -I./include -I$(BASE_H) -I$(LARS_REACTOR_H) -I$(PROTO_H) 
LIB= -L$(LARS_REACTOR_LIB) $(OTHER_LIB)

OBJS = lars_dns_test.o
OBJS += $(PROTO)/lars.pb.o

$(TARGET): $(OBJS)
	mkdir -p bin
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(INC) $(LIB)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC) 

.PHONY: clean

clean:
	-rm -f ./*.o $(TARGET)