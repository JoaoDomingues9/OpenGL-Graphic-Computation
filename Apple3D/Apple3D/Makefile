CC = gcc
CFLAGS = -Wall 
LIBS = -lglfw -lGLEW -lGL -lstdc++ -lm
SRCS = $(wildcard *.cpp)
SRCS += $(wildcard ../common/*.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = main

all: $(TARGET)
	./$(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(TARGET)
