FLAGS = $(shell pkg-config --libs --cflags libzmq) \
	    -lzmq -std=c++11 -lpthread -O3 -I../../../include/

TARGET = ptgrey

all: $(TARGET)

clean:
	rm -f $(TARGET)

%: %.cpp
	g++ $^ -o $@ $(FLAGS)