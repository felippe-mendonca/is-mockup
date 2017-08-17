FLAGS = -static $(shell pkg-config  --static --libs --cflags libzmq) \
	    -lzmq -std=c++14 -lpthread -O3 -I../include/

TARGET = camera
SERVICE = mockup-$(TARGET)
VERSION = 0.2
DOCKER_USER = mendonca

all: $(TARGET)

clean:
	rm -rf $(TARGET)

%: %.cpp
	g++ $^ -o $@ $(FLAGS)

docker: $(TARGET)
	docker build -t $(DOCKER_USER)/$(SERVICE):$(VERSION) .
	
docker_push: docker
	docker image push $(DOCKER_USER)/$(SERVICE):$(VERSION)