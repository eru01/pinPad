SRCS=main.cpp
OBJS=$(SRCS:.cpp=.o)
CXX=g++
CXXFLAGS=-O3
TARGET=keypadCtrl

.cpp.o:
	$(CXX) -c -o $@ $(CXXFLAGS) $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

clean:
	rm -f $(OBJS) $(TARGET)
