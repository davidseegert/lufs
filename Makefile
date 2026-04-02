CXX = g++
CXXFLAGS = -O3 -std=c++17 -I./third_party -DMA_NO_DEVICE_IO
SRC = src/lufs.cpp src/analyzer.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = lufs

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)
