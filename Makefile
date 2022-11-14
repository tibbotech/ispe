# To build for SPHE8368 SoC run it as:
# make CFLAGS+="-DSOC_SPHE8368"

all: ispe

ispe: ispe.cpp utils.cpp
	$(CXX) $(CFLAGS) -lssl -lcrypto -Wall -o ispe ispe.cpp utils.cpp

clean:
	rm -rf ./ispe
