# Makefile
# Sample for raw example on Raspberry Pi
# Caution: requires bcm2835 library to be already installed
# http://www.airspayce.com/mikem/bcm2835/

CC       = g++-4.7
CFLAGS   = -std=c++11 -DFTDI_SPI -D__BASEFILE__=\"$*\"
CXXFLAGS = -Wall -O3 -g
LIBS     = -lbcm2835 -lftd2xx
LMICBASE = ../../../src
INCLUDE  = -I$(LMICBASE) 

all: raw 

raspi.o: $(LMICBASE)/raspi/raspi.cpp
				$(CC) $(CFLAGS) -c $(LMICBASE)/raspi/raspi.cpp $(INCLUDE)

radio.o: $(LMICBASE)/lmic/radio.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/lmic/radio.c $(INCLUDE)

oslmic.o: $(LMICBASE)/lmic/oslmic.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/lmic/oslmic.c $(INCLUDE)

lmic.o: $(LMICBASE)/lmic/lmic.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/lmic/lmic.c $(INCLUDE)

hal.o: $(LMICBASE)/hal/hal.cpp
				$(CC) $(CFLAGS) -c $(LMICBASE)/hal/hal.cpp $(INCLUDE)

aes.o: $(LMICBASE)/aes/lmic.c
				$(CC) $(CFLAGS) -c $(LMICBASE)/aes/lmic.c $(INCLUDE) -o aes.o

raw.o: raw.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

#spiftdi.o: $(LMICBASE)/hal/spiftdi.cpp
#				$(CC) $(CXXFLAGS) -c $(LMICBASE)/hal/spiftdi.cpp $(INCLUDE)

ftdispill.o: $(LMICBASE)/raspi/ftdispill.c
				$(CC) $(CXXFLAGS) -c $(LMICBASE)/raspi/ftdispill.c $(INCLUDE) 
raftdispi.o: $(LMICBASE)/raspi/raftdispi.cpp
				$(CC) $(CFLAGS) -c $(LMICBASE)/raspi/raftdispi.cpp $(INCLUDE)


raw: raw.o raspi.o radio.o oslmic.o lmic.o hal.o aes.o raftdispi.o ftdispill.o
				$(CC) $^ $(LIBS) -o raw

clean:
				rm -rf *.o raw
