all: ispe

ispe: ispe.cpp
	$(CXX) $(CFLAGS) -Wall -o ispe ispe.cpp

clean:
	rm -rf ./ispe
