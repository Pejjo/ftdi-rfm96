/* Copyright (c) 2013 Owen McAree
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
// This is a big hack to transparently support FTDI FT4232 chip SPI. The code need some more love.

#include "ftdispill.h"
#include "spiftdi.h"

// Constructor
//	Opens the SPI device and sets up some default values
SPI::SPI(const char *device) {

	if (spi_init(&ftHandle,0,device))
	{ // Error occured
		throw 20;
	}
	setDelayUsecs(0);
}

// Set the mode of the bus (see linux/spi/spidev.h)
void SPI::setMode(uint8_t mode) {
	this->mode=mode;
	perror("Set Mode Unsupported");
}

// Get the mode of the bus
uint8_t SPI::getMode() {
	return this->mode;
}

// Set the number of bits per word
void SPI::setBitsPerWord(uint8_t bits) {
	this->bits=bits;
	perror("SetBits Unsupported");
}

// Get the number of bits per word
uint8_t SPI::getBitsPerWord() {
	return this->bits;
}

// Set the bus clock speed
void SPI::setMaxSpeedHz(uint32_t speed) {
	this->speed=speed;
	perror("SetMaxSpeed Unsupported");
}

// Get the bus clock speed
uint32_t SPI::getMaxSpeedHz() {
	return this->speed;
}

// Set the bus delay
void SPI::setDelayUsecs(uint16_t delay) {
	this->delay = delay;
}

// Get the bus delay
uint16_t SPI::getDelayUsecs() {
	return this->delay;
}

int SPI::pollInt(void) {
	return spi_getInt(&ftHandle);
}

// Transfer some data
bool SPI::transfer(uint8_t *tx, uint8_t *rx, int length) {
        spi_setCS(&ftHandle,0);

	if (delay) {
		usleep(delay);
	}

        spi_trans(&ftHandle,length, (char *)rx, (char *)tx);
	spi_setCS(&ftHandle,1);
	return 1;
}

// Close the bus
void SPI::close(void) {
   spi_close(&ftHandle);
}
