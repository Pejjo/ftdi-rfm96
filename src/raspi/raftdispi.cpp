// raspi.cpp
//
// Routines for implementing Arduino-LIMC on Raspberry Pi
// using BCM2835 library for GPIO
// This code has been grabbed from excellent RadioHead Library

#ifdef FTDI_SPI
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include "raftdispi.h"

//Initialize the values for sanity
static uint64_t epochMilli ;
static uint64_t epochMicro ;

void SPIClass::begin(const char *serial) {
  initialiseEpoch();
  
	if (spi_init(&ftHandle,0,device))
	{ // Error occured
		throw 20;
	}
	setDelayUsecs(0);
}

void SPIClass::end() {
  //End the SPI
  spi_close(&ftHandle);
}

void SPIClass::beginTransaction() {
  //Set SPI clock divider

}

void SPIClass::endTransaction() {
}
  
byte SPIClass::transfer(byte _data) {
	byte data;
    spi_setCS(&ftHandle,0);

	if (delay) {
		usleep(delay);
	}

        spi_trans(&ftHandle,1, (char *)&data, (char *)&_data);
	spi_setCS(&ftHandle,1);
	return data;
}
 
void pinMode(unsigned char pin, unsigned char mode) {
}

void digitalWrite(unsigned char pin, unsigned char value) {
}

unsigned char digitalRead(unsigned char pin) {
  if (pin == LMIC_UNUSED_PIN) {
    return 0;
  }
  return spi_getInt(&ftHandle);
}

//Initialize a timestamp for millis/micros calculation
// Grabbed from WiringPi
void initialiseEpoch() {
  struct timeval tv ;
  gettimeofday (&tv, NULL) ;
  epochMilli = (uint64_t)tv.tv_sec * (uint64_t)1000    + (uint64_t)(tv.tv_usec / 1000) ;
  epochMicro = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)(tv.tv_usec) ;
  pinMode(lmic_pins.nss, OUTPUT);
  digitalWrite(lmic_pins.nss, HIGH);
}

unsigned int millis() {
  struct timeval tv ;
  uint64_t now ;
  gettimeofday (&tv, NULL) ;
  now  = (uint64_t)tv.tv_sec * (uint64_t)1000 + (uint64_t)(tv.tv_usec / 1000) ;
  return (uint32_t)(now - epochMilli) ;
}

unsigned int micros() {
  struct timeval tv ;
  uint64_t now ;
  gettimeofday (&tv, NULL) ;
  now  = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec ;
  return (uint32_t)(now - epochMicro) ;
}

char * getSystemTime(char * time_buff, int len) {
	time_t t;
	struct tm* tm_info;
	
	t = time(NULL); 
	tm_info = localtime(&t);
	if (tm_info) {
		if (strftime(time_buff, len, "%H:%M:%S", tm_info)) {
		} else {
			strncpy(time_buff, "strftime() ERR", len);
		}
	} else {
		strncpy(time_buff, "localtime() ERR", len);
	}
	return time_buff;
}

void printConfig(const uint8_t led) {
	printf( "RFM95 device configuration\n" );
	if (lmic_pins.nss ==LMIC_UNUSED_PIN ) {
		printf( "!! CS pin is not defined !!\n" );
	} else {
		printf( "CS=GPIO%d", lmic_pins.nss );
	}
	
	printf( " RST=" );
	if (lmic_pins.rst==LMIC_UNUSED_PIN ) {
		printf( "Unused" );
	} else {
		printf( "GPIO%d", lmic_pins.rst );
	}

	printf( " LED=" );
	if ( led==LMIC_UNUSED_PIN ) {
		printf( "Unused" );
	} else {
		printf( "GPIO%d", led );
	}
	
	// DIO 
	for (uint8_t i=0; i<3 ; i++) {
		printf( " DIO%d=", i );
		if (lmic_pins.dio[i]==LMIC_UNUSED_PIN ) {
			printf( "Unused" );
		} else {
			printf( "GPIO%d", lmic_pins.dio[i] );
		}
	}
	printf( "\n" );
}
		
// Display a Key
// =============
void printKey(const char * name, const uint8_t * key, uint8_t len, bool lsb) 
{
  uint8_t start=lsb?len:0;
  uint8_t end = lsb?0:len;
  const uint8_t * p ;

  printf("%s : ", name);
  for (uint8_t i=0; i<len ; i++) {
    p = lsb ? key+len-i-1 : key+i;
    printf("%02X", *p);
  }
  printf("\n");
}

// Display OTAA Keys
// =================
void printKeys(void) 
{
	// LMIC may not have used callback to fill 
	// all EUI buffer so we do it to a temp
	// buffer to be able to display them
	uint8_t buf[32];
	os_getDevEui((u1_t*) buf);
	printKey("DevEUI", buf, 8, true);
	os_getArtEui((u1_t*) buf);
	printKey("AppEUI", buf, 8, true);
	os_getDevKey((u1_t*) buf);
	printKey("AppKey", buf, 16, false);
}



bool getDevEuiFromMac(uint8_t * pdeveui) {
  struct ifaddrs *ifaddr=NULL;
  struct ifaddrs *ifa = NULL;
  int family = 0;
  int i = 0;
  bool gotit = false;

  // get linked list of the network interfaces
  if (getifaddrs(&ifaddr) == -1) {
    return false;
  } else {
    // Loop thru interfaces list
    for ( ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next) {
      // Ethernet
      if ( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family==AF_PACKET) ) {
        // Not loopback interface
        if (! (ifa->ifa_flags & IFF_LOOPBACK)) {
          char fname[128];
          int fd;
          int up=0;
          struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
         
          // Get interface status 
          // Interface can be up with no cable connected and to be sure
          // It's up, active and connected we need to get operstate
          // 
          // if up + cable    if up + NO cable    if down + cable
          // =============    ==========          ==================
          // carrier:1        carrier:0           carrier:Invalid
          // dormant:0        dormant:0           dormant:Invalid
          // operstate:up     operstate:down      operstate :own
          sprintf(fname, "/sys/class/net/%s/operstate", ifa->ifa_name);
          if ( (fd = open( fname, O_RDONLY)) > 0 ){
            char buf[2];
            if ( read(fd, buf, 2) > 0 ) {
              // only first active interface "up"
              if ( buf[0]=='u' && buf[1]=='p' ) {
                uint8_t * p = pdeveui;
                // deveui is LSB to we reverse it so TTN display 
                // will remain the same as MAC address
                // MAC is 6 bytes, devEUI 8, set first 2 ones 
                // with an arbitrary value
                *p++ = 0x00;
                *p++ = 0x04;
                // Then next 6 bytes are mac address still reversed
                for ( i=0; i<6 ; i++) {
                  *p++ = s->sll_addr[5-i];
                }
                
                gotit = true;
                close(fd);
                break;
              } 
            }
            close(fd);
          } 
        }
      }
    }
    // Free our Linked list
    freeifaddrs(ifaddr);
  }

  // just in case of error put deveui to 0102030405060708
  if (!gotit) {
    for (i=1; i<=8; i++) {
      *pdeveui++=i;
    }
  }
  return gotit;
}

void SerialSimulator::begin(int baud) {
  // No implementation neccesary - Serial emulation on Linux = standard console
  // Initialize a timestamp for millis calculation - we do this here as well in case SPI
  // isn't used for some reason
  initialiseEpoch();
}

size_t SerialSimulator::println(void) {
  fprintf(stdout, "\n");
}

size_t SerialSimulator::println(const char* s) {
  fprintf( stdout, "%s\n",s);
}

size_t SerialSimulator::print(const char* s) {
  fprintf( stdout, "%s",s);
}

size_t SerialSimulator::println(u2_t n) {
  fprintf(stdout, "%d\n", n);
}

size_t SerialSimulator::print(ostime_t n) {
  fprintf(stdout, "%d\n", n);
}

size_t SerialSimulator::print(unsigned int n, int base) {
  if (base == DEC)
    fprintf(stdout, "%d", n);
  else if (base == HEX)
    fprintf(stdout, "%02x", n);
  else if (base == OCT)
    fprintf(stdout, "%o", n);
  // TODO: BIN
}

size_t SerialSimulator::print(char ch) {
  fprintf(stdout, "%c", ch);
}

size_t SerialSimulator::println(char ch) {
  fprintf(stdout, "%c\n", ch);
}

size_t SerialSimulator::print(unsigned char ch, int base) {
  return print((unsigned int)ch, base);
}

size_t SerialSimulator::println(unsigned char ch, int base) {
  print((unsigned int)ch, base);
  fprintf( stdout, "\n");
}

size_t SerialSimulator::write(char ch) {
  fprintf( stdout, "%c", ch);
}

size_t SerialSimulator::write(unsigned char* s, size_t len) {
  for (int i=0; i<len; i++) {
    fprintf(stdout, "%c", s[i]);
  }
}

void SerialSimulator::flush(void) {
  fflush(stdout);
}

#endif // RASPBERRY_PI
