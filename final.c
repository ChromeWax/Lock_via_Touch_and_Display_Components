#include <u.h>
#include <libc.h>

int gpio, spi;

enum {
	TFTWidth = 128,
	TFTHeight = 128,
};

int getnumoftapsinfive(void);
void pitftdraw(int);
void pitftsetup(void);
void setwindow(int, int, int, int);
int candrawone(int, int, int);
int candrawtwo(int, int, int);
int candrawthree(int, int, int);
int candrawfour(int, int, int);
int candrawfive(int, int, int);
void spicmd(uchar);
void spidata(uchar *, int);
void clearmem(void);
void writedevicemem(int, int, long, long);
void writedevice(uchar*, int);
void readdevicemem(uchar*);
void readdevice(uchar*, int);
int resetdevice(void);
void udelay(ulong);
void setupinterface(void);

/*
	Three countdowns will appear on the screen each starting at 5.
	During each countdown, the user will press the touch sensor a certain number of times.
	If the user enters the combo right, the screen will light up green else red.
	The combo is 5-2-3.
*/
void
main()
{
	int first, second, third;
	uchar buf[129];
	long combo, out;

	first = second = third = 0;
	combo = 523;

	// Sets up interfaces and pitft
	setupinterface();
	pitftsetup();

	// Clears 1wire and writes combo
	clearmem();
	writedevicemem(0, 0, combo, 0);

	// Gets number of taps for first countdown
	first = getnumoftapsinfive();
	pitftdraw(8);
	sleep(2000);

	// Gets number of taps for second countdown
	second = getnumoftapsinfive();
	pitftdraw(8);
	sleep(2000);

	// Gets number of taps for third countdown
	third = getnumoftapsinfive();
	pitftdraw(8);
	sleep(2000);

	// Grabs combo from 1wire
	readdevicemem(buf);
	out = buf[1] << 8 | buf[0];
	
	// Draws either green or red depending if combo inputted correclty
	if (first == (out / 100) && second == (out % 100 / 10) && third == (out % 100 % 10))
		pitftdraw(6);
	else
		pitftdraw(7);

	exits(0);
}

int
getnumoftapsinfive()
{
	int i, timer, count, hold;
	uvlong x;
	char buf[17];

	timer = count = hold = 0;
	i = 6;

	while(timer < 5000) {	
		// For every 1000 of the timer decrement i and update display
		if (timer % 1000 == 0) {
			i--;
			pitftdraw(i);
		}

		// Reads gpio
		if(read(gpio, buf, 16) < 0)
			print("read error %r\n");
		buf[16] = 0;
		x = strtoull(buf, nil, 16);

		if (x & (1 << 23)) {
			// Runs only once even if finger is still on touch pad
			if (hold == 0)
				count++;

			hold = 1;

			// Turns on led
			fprint(gpio, "set 27 1");
		}
		else {
			hold = 0;

			// Turns off led
			fprint(gpio, "set 27 0");
		}

		timer += 5;
		sleep(5);
	}
	
	// Prints number of taps to console
	print("%d\n", count);

	return count;
}

void
pitftdraw(int num)
{
	int i, r, c;
	uchar spibuf[32];
	
	// Select command to write data
	spicmd(0x2c);

	// Loops through render
	for (r = 0; r < TFTHeight; ++r) {
		for (c = 0; c < TFTWidth; c +=8) {
			for (i = 0; i < 8; ++ i) {
				// Renders green square
				if (num == 6) {
					spibuf[i * 3] = 0x00;
					spibuf[i * 3 + 1] = 0xff;
					spibuf[i * 3 + 2] = 0x00;
				}

				// Renders red square
				else if (num == 7) {
					spibuf[i * 3] = 0xff;
					spibuf[i * 3 + 1] = 0x00;
					spibuf[i * 3 + 2] = 0x00;
				}

				// Renders black square
				else if (num == 8) {
					spibuf[i * 3] = 0x00;
					spibuf[i * 3 + 1] = 0x00;
					spibuf[i * 3 + 2] = 0x00;
				}

				// Renders number
				else if (candrawone(num, r, c) || candrawtwo(num, r, c) || candrawthree(num, r, c) || candrawfour(num, r, c) || candrawfive(num, r, c)) {
					spibuf[i * 3] = 0x00;
					spibuf[i * 3 + 1] = 0x00;
					spibuf[i * 3 + 2] = 0x00;
				}

				// Renders white
				else {
					spibuf[i * 3] = 0xff;
					spibuf[i * 3 + 1] = 0xff;
					spibuf[i * 3 + 2] = 0xff;
				}
			}
		
			// Send render to TFT
			spidata(spibuf, 24);
		}
	}
}

void
pitftsetup()
{
	uchar buf[32];

	// Sets pin 25 to out
	fprint(gpio, "function 25 out");
	
	// Resets
	spicmd(0x01);
	sleep(120);
	
	// Gets out of sleep mode
	spicmd(0x11);
	sleep(120);

	// Turns on display
	spicmd(0x29);
	sleep(120);

	// Sets to BGR mode and renders bottom to top
	spicmd(0x36);
	buf[0] = 0xf8;
	spidata(buf, 1);

	// Sets to 18 bit / pixel for 8 bit data bus
	spicmd(0x3a);
	buf[0] = 0x06;
	spidata(buf, 1);

	// Set window size
	setwindow(0, 0, TFTWidth  - 1, TFTHeight - 1);
}

void
setwindow(int minc, int minr, int maxc, int maxr)
{
	uchar spibuf[4];

	// Set column window
	spicmd(0x2a);
	spibuf[0] = minc >> 8;
	spibuf[1] = minc & 0xff;
	spibuf[2] = maxc >> 8;
	spibuf[3] = maxc & 0xff;
	spidata(spibuf, 4);
	
	// Set row window
	spicmd(0x2b);
	spibuf[0] = minr >> 8;
	spibuf[1] = minr & 0xff;
	spibuf[2] = maxr >> 8;
	spibuf[3] = maxr & 0xff;
	spidata(spibuf, 4);
}

int
candrawone(int num, int r, int c)
{
	return (num == 1) && 
		((c >= 96 && c < 104 && r >= 32 && r < 60) || 
		(c >= 96 && c < 104 && r >= 68 && r < 96));
}

int
candrawtwo(int num, int r, int c)
{
	return (num == 2) && 
		((r >= 24 && r < 32 && c >= 32 && c < 96)  || 
		(r >= 60 && r < 68 && c >= 32 && c < 96) || 
		(r >= 96 && r < 104 && c >= 32 && c < 96) || 
		(c >= 24 && c < 32 && r >= 32 && r < 60) || 
		(c >= 96 && c < 104 && r >= 68 && r < 96));
}

int
candrawthree(int num, int r, int c)
{
	return (num == 3) && 
		((r >= 24 && r < 32 && c >= 32 && c < 96)  || 
		(r >= 60 && r < 68 && c >= 32 && c < 96) || 
		(r >= 96 && r < 104 && c >= 32 && c < 96) || 
		(c >= 96 && c < 104 && r >= 32 && r < 60) || 
		(c >= 96 && c < 104 && r >= 68 && r < 96));
}

int
candrawfour(int num, int r, int c)
{
	return (num == 4) && 
		((r >= 60 && r < 68 && c >= 32 && c < 96) || 
		(c >= 24 && c < 32 && r >= 68 && r < 96) ||
		(c >= 96 && c < 104 && r >= 32 && r < 60) || 
		(c >= 96 && c < 104 && r >= 68 && r < 96));
}

int
candrawfive(int num, int r, int c)
{
	return (num == 5) && 
		((r >= 24 && r < 32 && c >= 32 && c < 96)  || 
		(r >= 60 && r < 68 && c >= 32 && c < 96) || 
		(r >= 96 && r < 104 && c >= 32 && c < 96) || 
		(c >= 24 && c < 32 && r >= 68 && r < 96) ||
		(c >= 96 && c < 104 && r >= 32 && r < 60));
}

void
spicmd(uchar c)
{
	char buf[1];

	// Sets pin 25 to 0
	fprint(gpio, "set 25 0");
	
	// Set buf to cmd
	buf[0] = c;
	
	// Writes to spi
	pwrite(spi, buf, 1, 0);
}

void
spidata(uchar *p, int n)
{
	char buf[128];
	
	// Set n to 128 in case its over
	if(n > 128)
		n = 128;

	// Set pin 25 to 1
	fprint(gpio, "set 25 1");
	
	// Set buf to data
	memmove(buf, p, n);

	// Writes to spi
	pwrite(spi, buf, n, 0);
	
	// Set pin 25 to 0
	fprint(gpio, "set 25 0");
}

void
clearmem()
{
	int i;

	for (i = 0; i < 128; i = i + 8)
		writedevicemem(i, 0, 0, 0);
}

void
writedevicemem(int addr1, int addr2, long c1, long c2)
{
	uchar buf[32];

	// Writes to scratchpad
	if(resetdevice() == 0) {
		print("1 wire reset failure\n");
		exits("1 wire");
	}
	buf[0] = 0xcc;
	buf[1] = 0x0f;
	buf[2] = addr1;
	buf[3] = addr2;
	buf[4] = c1 & 0xff;
	c1 >>= 8;
	buf[5] = c1 & 0xff;
	c1 >>= 8;
	buf[6] = c1 & 0xff;
	c1 >>= 8;
	buf[7] = c1 & 0xff;
	buf[8] = c2 & 0xff;
	c2 >>= 8;
	buf[9] = c2 & 0xff;
	c2 >>= 8;
	buf[10] = c2 & 0xff;
	c2 >>= 8;
	buf[11] = 	c2 & 0xff;
	writedevice(buf, 12);

	// Reads scratchpad
	if(resetdevice() == 0) {
		print("1 wire reset failure\n");
		exits("1 wire");
	}
	buf[0] = 0xcc;
	buf[1] = 0xaa;
	writedevice(buf, 2);
	readdevice(buf, 11);
	
	// Copies scratchpad to eeprom
	if(resetdevice() == 0) {
		print("1 wire reset failure\n");
		exits("1 wire");
	}
	buf[4] = buf[2];
	buf[3] = buf[1];
	buf[2] = buf[0];
	buf[0] = 0xcc;
	buf[1] = 0x55;
	writedevice(buf, 5);
	udelay(12000);
	readdevice(buf, 11);
}

void
writedevice(uchar* buf, int numbytes)
{
	int i, j;
	uchar c;
	
	// Loops for multiple bytes
	for(j = 0; j < numbytes; j++) {
		// Sends either a 1 or 0 for each bit of a byte
		c = buf[j];
		for(i = 0; i < 8; i++) {
			// Test least significant bit if 1 or 0
			if(c & 0x01) {
				fprint(gpio, "function 26 out");
				fprint(gpio, "function 26 in");
				udelay(100);
			}

			else {
				fprint(gpio, "function 26 out");
				udelay(60);
				fprint(gpio, "function 26 in");
				udelay(40);
			}

			// Move the next bit over so it becomes the new least significant bit
			c >>= 1;
		}
	}
}

void
readdevicemem(uchar* buf)
{
	uchar buf2[32];

	if(resetdevice() == 0) {
		print("1 wire reset failure\n");
		exits("1 wire");
	}
	buf2[0] = 0xcc;
	buf2[1] = 0xf0;
	buf2[2] = 0x00;
	buf2[3] = 0x00;
	writedevice(buf2, 4);
	readdevice(buf, 128);
}

void
readdevice(uchar* buf2, int numbytes)
{
	uvlong x;
	int i, j;
	uchar c;
	char buf[17];

	// Loops for multiple bytes
	for(j = 0; j < numbytes; j++)
	{
		c = 0;
		for(i = 0; i < 8; ++i) {
			// Moves significant bit down
			c >>= 1;
			
			// Reads 1wire
			fprint(gpio, "function 26 pulse");
			if(read(gpio, buf, 16) < 0)
				print("read error %r\n");
			buf[16] = 0;
			x = strtoull(buf, nil, 16);

			// If 1wire sends back a 1 make most significant bit of c the bit value 1
			if(x & (1 << 26))
				c |= 0x80;

			udelay(20);
		}

		// Assigns bit that was just read to buf2
		buf2[j] = c;
	}
}

int
resetdevice()
{
	uvlong x;
	char buf[17];
	
	// Resets 1wire
	fprint(gpio, "function 26 out");
	udelay(600);
	fprint(gpio, "function 26 in");
	udelay(90);

	// Reads 1wire
	if(read(gpio, buf, 16) < 0)
		print("read error %r\n");
	buf[16] = 0;
	x = strtoull(buf, nil, 16);
	
	udelay(100);

	// Returns if successful or not
	if(x & (1 << 26))
		return 0;
	return 1;
}

void
udelay(ulong t)
{
	ulong i;

	t *= 590;
	for(i = 0; i < t; ++i) {}
}

void
setupinterface()
{
	// Opens GPIO
	gpio = open("/dev/gpio", ORDWR);
	if (gpio < 0) {
		bind("#G", "/dev", MAFTER);
		gpio = open("/dev/gpio", ORDWR);
		if (gpio < 0) {
			print("gpio open: %r\n");
			exits("gpio");
		}
	}

	// Opens SPI
	spi = open("/dev/spi0", ORDWR);
	if (spi < 0) {
		bind("#π", "/dev", MAFTER);
		spi = open("/dev/spi0", ORDWR);
		if (spi < 0) {
			print("spi open: %r\n");
			exits("spi");
		}
	}

	// Sets gpio 27, 23, 26
	fprint(gpio, "function 27 out");
	fprint(gpio, "function 23 in");
	fprint(gpio, "function 26 in");
	fprint(gpio, "set 26 0");
}
