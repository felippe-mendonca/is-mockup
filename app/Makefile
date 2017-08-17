COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Werror -Wextra

SO_DEPS = $(shell pkg-config --libs --cflags libzmq libSimpleAmqpClient msgpack librabbitmq opencv theoradec theoraenc)
SO_DEPS += -lboost_program_options -lboost_system -lpthread -lzmq

TARGET=visual_servoing
SERVICE=mockup-app-visual_servoing
VERSION=0.1
DOCKER_USER=mendonca

all: $(TARGET)

clean:
	rm -f $(TARGET)

$(TARGET): $(TARGET).cpp 
	$(COMPILER) $^ -o $@ $(FLAGS) $(SO_DEPS)

docker: $(TARGET)
	rm -rf libs/
	mkdir libs/
	lddcp $(TARGET) libs/
	docker build -t $(DOCKER_USER)/$(SERVICE):$(VERSION) .
	rm -rf libs/

docker_push: docker
	docker image push $(DOCKER_USER)/$(SERVICE):$(VERSION)