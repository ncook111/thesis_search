CXX=g++
CXXFLAGS= -Os -std=c++20 -Iinclude

TARGET  = ingest
OBJS    = ingest.o coder.o tokenizer.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

ingest.o: ingest.cpp ingest.h coder.h tokenizer.h
	$(CXX) $(CXXFLAGS) -c ingest.cpp -o ingest.o

coder.o: coder.cpp coder.h
	$(CXX) $(CXXFLAGS) -c coder.cpp -o coder.o

tokenizer.o: tokenizer.cpp tokenizer.h ingest.h
	$(CXX) $(CXXFLAGS) -c tokenizer.cpp -o tokenizer.o

clean:
	rm -f $(OBJS) $(TARGET)