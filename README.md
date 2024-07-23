# Lock via Touch, Display, and 1 Wire components
The main goal of this project was to learn how to use embedded C to
interact with various components. This project is about tapping the
touch sensor the correct number of times per 5-second countdown. If
the touch sensor was pressed the correct number of times, the
display would turn green else red. I used a Raspberry Pi, a 1-wire
device to store the combination of correct presses, a display to
show the countdown and final result, and a touch sensor for inputting
combinations.

# Components
* Adafruit 1.44" Color TFT LCD Display with MicroSD Card breakout (ST7735R)
* 8-Key Capacitive Touch Sensor Breakout (CAP1188)
* 1-Wire EEPROM (DS28E07)

# How to run this
To run this would require a Raspberry Pi with plan9.
* Connect pin 27 to LED on touch sensor
* Connect pin 23 to OUT on touch sensor
* Connect pin 26 to 1-wire
* Connect SPI pins to LCD Display
* Compile final.c with the command '5c final.c'
* Then run the executable.

# Screenshots
![](Images/Lock_1.gif)

# Built With
* C
* Plan9

# Authors
* Richard Vo
