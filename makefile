CXX      = g++
CXXFLAGS = -Os -std=c++20 -Iinclude -I/opt/homebrew/include
LDFLAGS  = -L/opt/homebrew/lib
LIBS     = -lfst

TARGET  = ingest
OBJS    = ingest.o coder.o tokenizer.o fst.o index.o helper.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

ingest.o: ingest.cpp ingest.h coder.h tokenizer.h fst.h index.h
	$(CXX) $(CXXFLAGS) -c ingest.cpp -o ingest.o

coder.o: coder.cpp coder.h
	$(CXX) $(CXXFLAGS) -c coder.cpp -o coder.o

tokenizer.o: tokenizer.cpp tokenizer.h ingest.h
	$(CXX) $(CXXFLAGS) -c tokenizer.cpp -o tokenizer.o

fst.o: fst.cpp fst.h
	$(CXX) $(CXXFLAGS) -c fst.cpp -o fst.o

index.o: index.cpp index.h
	$(CXX) $(CXXFLAGS) -c index.cpp -o index.o

helper.o: helper.cpp helper.h
	$(CXX) $(CXXFLAGS) -c helper.cpp -o helper.o

clean:
	rm -f $(OBJS)