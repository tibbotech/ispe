all: ispe

ispe: ispe.cpp
	$(CXX) $(CFLAGS) -o ispe ispe.cpp

clean:
	rm -rf ./ispe
