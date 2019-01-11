/* How to use the TEA5767 FM radio Module with Arduino
 More info: http://www.ardumotive.com/how-to-use-the-tea5767-fm-radio-module-en.html
 Dev: Vasilakis Michalis // Date: 21/9/2015 // www.ardumotive.com    */

//Libraries:
#include "Arduino.h"
#include <TEA5767.h>
#include <Wire.h>


//Variables:
double old_frequency;
double frequency;
int search_mode = 0;
int search_direction;
unsigned long last_pressed;
unsigned char buf[5];
int stereo;
int signal_level;
double current_freq;
unsigned long current_millis = millis();
char inByte;
bool newFreqFound = true;

static double currFreq = 103.3;

//Constants:
TEA5767 Radio; //Pinout SLC and SDA - Arduino uno pins A5 and A4



void getFrequency();

void setup() {
	//Init
	Serial.begin(9600);
	Radio.init();
	Radio.set_frequency(currFreq);
	Serial.println("Setup completed.");
}

void loop() {

	if (millis() > current_millis + 2000) {
		getFrequency();
		current_millis = millis();
	}

	if (Serial.available()) {
		inByte = Serial.read();
		if (inByte == '+' || inByte == '-') {
			newFreqFound = true;
			search_mode = 1;

			//If forward button is pressed, go up to next station
			if (inByte == '+') {
				Serial.println("Searching up...");
				search_direction = TEA5767_SEARCH_DIR_UP;
				Radio.search_up(buf);
			}
			else {
				Serial.println("Searching down...");
				search_direction = TEA5767_SEARCH_DIR_DOWN;
				Radio.search_down(buf);
			}
		}

		if (inByte == '?') {
			newFreqFound = true;
			getFrequency();
		}
	}

	if (search_mode == 1 && Radio.process_search(buf, search_direction) == 1) {
		search_mode = 0;
	}
}

void getFrequency() {
	return;
	if (Radio.read_status(buf) == 1) {
		current_freq = floor(Radio.frequency_available(buf) / 100000 + .5) / 10;
		stereo = Radio.stereo(buf);
		signal_level = Radio.signal_level(buf);

		//By using newFreqFound variable the message will be printed only one time.
		if (newFreqFound) {
			Serial.print("Current freq: ");
			Serial.print(current_freq);
			Serial.print("MHz Signal: ");

			//Stereo or mono ?
			if (stereo) {
				Serial.print("STEREO ");
			}
			else {
				Serial.print("MONO ");
			}
			Serial.print(signal_level);
			Serial.println("/15");
			newFreqFound = false;
		}
		//delay(500);
	}
}
