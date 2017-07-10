#ifndef ftispih
#define ftispih
#include <stdio.h>
#include <unistd.h>
#include <ftd2xx.h>


int spi_init(FT_HANDLE *dev, char *descr, const char *serial);
int spi_setCS(FT_HANDLE *dev, int level);
int spi_getInt(FT_HANDLE *dev);
int spi_trans(FT_HANDLE *dev, unsigned int len, char *rxbuf, char *txbuf);
void spi_close(FT_HANDLE *dev);
#endif
