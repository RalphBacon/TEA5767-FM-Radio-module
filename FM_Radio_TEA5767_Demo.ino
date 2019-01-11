/*
 * Non-library version of demo code to run a TEA5767 radio module such as this one:
 *
 * https://www.banggood.com/TEA5767-FM-Stereo-Radio-Module-76-108MHZ-For-Arduino-With-Antenna-p-1103142.html??p=FQ040729393382015118&utm_campaign=25129675&utm_content=3312
 *
 * There are several libraries available but how do they actually work? Well, here we control
 * the TEA5767 chip directly using I2C just to prove how it is done.
 *
 * See my video on this: https://www.youtube.com/ralphbacon video #130
 *
 * This demo sketch only WRITES to the module, it does not read back the results, such as
 * the frequency of any searched radio station. But if you get this working you will have
 * no trouble writing your own sketch to READ the values back (much simpler than WRITING).
 */

#include "Arduino.h"

// We are using I2C so include the standard Arduino library for it
#include <Wire.h>

// List of local UK radio stations
double stationFrequency [] = {90.8, 103.3, 104.5, 93.0, 98.2, 88.6, 93.0};

// All the work is done in the loop
void setup() {
	// Allow debugging messages to the Serial Monitor (debugging window)
	Serial.begin(9600);
	Serial.println("Sketch begins.");
}

// Main loop - just switches radio stations every few seconds
void loop() {

	// Keep track of the station we're on
	static unsigned int stationIndex = 0;

	// We need a 5-byte array to communicate with the TEA5767
	unsigned char buffer[5];

	// We'll initialise all the bytes to zero
	memset(buffer, 0, 5);

	// Interesting value this! We should calculate which value here gives the better
	// result but for the demo we'll guess. If you get poor results change it to 1.
	int hilo = 0;

	// The known frequency of a strong FM radio in your location. This is for
	// radio stations in the UK in my area - your mileage will vary.
	double freq = stationFrequency[stationIndex];
	stationIndex++;

	// If we have run out of stations to find, just keep the last one current
	if (stationIndex == sizeof(stationFrequency) / 4) stationIndex--;

	// Bytes 1 &  2 - contain the frequency above in 14 bits. Calculate the number
	// by this convoluted formula:
	/*
	 * N = 4 x (fRF + fIF) / fREF
	 *
	 * where
	 * N = the number to be stored in the bits here
	 * fRF = the required frequency in Hz
	 * fIF = the intermediate frequency in kHz (225 kHz)
	 * fREF = the reference frequency in Hz. 32.768KHz for watch crystal as used here
	 *
	 * Example for 100MHz required radio frequency:
	 *
	 * 4 x(100 x 10^6 + 225 x 10^3) / 32768 = 12234 (in decimal)
	 *
	 * In hexadecimal, the value we store is 12234 -> 2FCA in 14 bits
	 */
	unsigned int internalFrequency;
	if (hilo == 1) {
		internalFrequency = 4 * (freq * pow(10, 6) + (225 * pow(10, 3))) / 32768;
	}
	else
		internalFrequency = 4 * (freq * pow(10, 6) - (225 * pow(10, 3))) / 32768;

	// Byte 1 requires first two (MSB) bits set to 0: bit 7 is MUTE, bit 6 is SEARCH
	// neither of which we want here, hence 0.

	// So Shift the frequency value to the right 8 bits and set first two bits to 0 by
	// ANDing 00111111 (hex 3F) in binary. Huh?
	/*
	 * OK, let's assume we have calculated the internal frequency as 2FCA. We want just
	 * the FIRST part (byte) of that number namely the 2F to be stored in Byte 1 of the
	 * buffer. But we also need the first two bits set to 1, hence the ANDing with 3F.
	 */
	buffer[0] = (internalFrequency >> 8);// & 0x3f;

	// Byte 2 is the second byte of the frequency, in this example CA (of the 2FCA).
	// As the buffer is just one byte long it will automatically truncate the two-byte
	// integer to the last byte. A bit sloppy so we'll use a better function.
	//buffer[1] = internalFrequency & 0xff;
	buffer[1] = lowByte(internalFrequency);

	// Byte 3 - see datasheet for full details on each bit (flag)
	// From left (MSB) to right (LSB)
	// 7 - SUD - Search up or down. 1 = up (towards 108MHz) 0 = down (towards 88MHz)
	// 6 - SSL - Search stop level. How "good" does the signal need to be to stop searching?
	// 5 - SSL - As above. 01 = low level signal, 10 mid level signal, 11 high level signal, 00 not allowed
	// 4 - HLSI - Hi/Lo Signal Injection.  1= high side, 0 = low side. I found UK to be 0.
	// 3 - MS - Mono to stereo. 1 = Force mono, 0 = Stereo if available
	// 2 - MR - Mute right channel. 1 = mute right channel and force to mono. Leave as 0 for normal operation.
	// 1 - ML - Mute left channel. 1= mute left channel and force to mono. Leave as 0 for normal operation.
	// 0 - SWP1 - Software programmable port 1. Keep as 1 to let system tell you it has found a station in search mode (see bit 0, byte 4)
	buffer[2] = 0b01101000;

	// Byte 4 - see datasheet for each bit
	// From left (MSB) to right (LSB)
	// 7 - SWP2 - Software programmable port 2. I couldn't see why this is useful other than to pass a bit to the "read" operation.
	// 6 - STBY - 1= In standby mode. 0 = normal operation.
	// 5 - BL - Band Limit. 1 = Japanese, 2 = USA/Europe
	// 4 - XTAL - Clock frequency. In this module set this to 1, which means 32.768KHz (the clock crystal on board)
	// 3 - SMUTE - Soft Mute. 1= on, 2 = off, I think this mutes the output whilst searching
	// 2 - HCC - High Cut Control. No idea what this does. It sounded the same either way to my old ears.
	// 1 - SNC - Stereo Noise Cancelling. 1= On, 0 = off. This reduces the noise (hiss) but partly converting to mono on poor signals.
	// 0 - SI - Search Indicator. 1 = SWPORT1 (bit 0, byte 3) is used to indicate signal detected. Otherwise under user control.
	buffer[3] = 0b00011110;

	// We don't need to set buffer[4] (5th byte) as it can be left as all zeroes.
	// From left (MSB) to right (LSB)
	// 7 - PLLREF - must be 0 as bit 4 in byte 4 (XTAL) is set to 1.
	// 6 - DTC - De-emphasis Time Constant must be 0 to indicate 50µS time constant. Might be different in your part of the world!
	// Equivalent to:
	// buffer[4] = 0b00000000;

	// Let's send the 5-byte buffer to the TEA5767 module now
	Wire.beginTransmission(0x60); // Fixed address - see datasheet

	Serial.print("Frequency: "); Serial.println(freq);
	for (int i = 0; i < 5; i++) {
		Wire.write(buffer[i]);

		// Print out the contents of the bytes as fix length binary values
		Serial.print("Byte ");
		Serial.print(i+1);
		Serial.print(": ");
		String value = "00000000" + String(buffer[i], BIN);
		Serial.println(value.substring(value.length()-8));
	}
	Serial.println("Tuning completed.");

	// End of I2C transmission. If you end the transmission before sending all 5 bytes
	// the old values in the non-sent bytes will persist.
	Wire.endTransmission();

	// Delay before changing frequency for this demo. You could end the sketch here if you have tuned to a station.
	delay(2000);
}
