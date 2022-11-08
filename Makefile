all: ispe

ispe: ispe.cpp utils.cpp
	$(CXX) $(CFLAGS) -lssl -lcrypto -Wall -o ispe ispe.cpp utils.cpp

clean:
	rm -rf ./ispe
