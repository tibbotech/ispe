# To build for SPHE SoC run it as:
# make CFLAGS+="-DSOC_SPHE"

all: ispe

ispe: ispe.cpp utils.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -Wall -o ispe ispe.cpp utils.cpp -lcrypto -lssl

install:
	install -d $(DESTDIR)/
	install -d $(DESTDIR)/ispe-templates/
	install -d $(DESTDIR)/ispe-helpers/
	install -p -m0755 ispe $(DESTDIR)/
	install -p -m0755 ispe-helpers/genisp.*.sh $(DESTDIR)/ispe-helpers/
	install -p -m0755 ispe-helpers/script_enc.sh $(DESTDIR)/ispe-helpers/
	install -p -m0644 ispe-helpers/sh.defs $(DESTDIR)/ispe-helpers/
	install -p -m0644 ispe-templates/*.hdr.T $(DESTDIR)/ispe-templates/

clean:
	rm -rf ./ispe

ispe : isp.h
