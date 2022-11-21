# To build for SPHE8368 SoC run it as:
# make CFLAGS+="-DSOC_SPHE8368"

all: ispe

ispe: ispe.cpp utils.cpp
	$(CXX) $(CFLAGS) -lssl -lcrypto -Wall -o ispe ispe.cpp utils.cpp

install:
	install -d $(DESTDIR)/
	install -d $(DESTDIR)/ispe-templates/
	install -d $(DESTDIR)/ispe-helpers/
	install -p -m0755 ispe $(DESTDIR)/
	install -p -m0755 ispe-helpers/genisp.emmc.sh $(DESTDIR)/ispe-helpers/
	install -p -m0755 ispe-helpers/script_enc.sh $(DESTDIR)/ispe-helpers/
	install -p -m0644 ispe-helpers/sh.defs $(DESTDIR)/ispe-helpers/
	install -p -m0644 ispe-templates/sp7021.hdr.T $(DESTDIR)/ispe-templates/

clean:
	rm -rf ./ispe
