#include "Arduino.h"
#include <vector>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266HTTPClient.h>

#include <Adafruit_GFX.h>
#include <GPNBadge.hpp>
#include <FS.h>
#include "rboot.h"
#include "rboot-api.h"
#include "Colors.h"

#define CONNECTION_TIMEOUT 10000																	//HTTP Probe Connection timeout in ms
#define CONNECTION_TEST_URL "http://example.org/"													//HTTP Probe target
#define CONNECTION_TEST_PREFIX "<!doctype html>\n<html>\n<head>\n    <title>Example Domain</title>"	//HTTP Probe expected data

/* 
 * Device States
 */
enum State {
	SCANNING,	//Currently scanning for networks
	PROBE,		//HTTP Probe running
	FOUND		//Open WiFi available
} deviceState = SCANNING;
int switchi = 0;

//Device state colors
uint32_t colorBooting   = pixels.Color(255, 255, 255);
uint32_t colorScanning  = pixels.Color(0, 0, 255);
uint32_t colorProbe     = pixels.Color(255, 255, 0);
uint32_t colorFound     = pixels.Color(0, 255, 0);

String netSSID;					//SSID of the currently probed network
String netBSSID;				//BSSID of the currently probed network
uint32_t netProbeStart;			//time of http probe start
std::vector<String> blacklist;	//BSSID blacklist, contains networks witch have been probed and failed

//The setup function is called once at startup of the sketch
Badge badge;

int pixelBrightness = 32;

void setup() {
	badge.init();
	badge.setBacklight(true);
 

	tft.begin(); //initialize the tft. This also sets up SPI to 80MHz Mode 0
	tft.setRotation(2); //turn screen
	tft.scroll(32); //move down by 32 pixels (needed)
 
	pixels.setBrightness(255);

	rboot_config rboot_config = rboot_get_config();
	SPIFFS.begin();
	File f = SPIFFS.open("/rom"+String(rboot_config.current_rom),"w");
	f.println("WiFinder\n");


	ESP.eraseConfig();				//Clear WiFi credentials
	WiFi.mode(WIFI_STA);			//Switch device into station mode

}

void setPixels(int from, int until, uint32_t c){
	int r, g, b;
	r = pixelBrightness*getRedValueFromColor(c)/255;
	g = pixelBrightness*getGreenValueFromColor(c)/255;
	b = pixelBrightness*getBlueValueFromColor(c)/255;
	for(int i = from; i <= until; i++){
		pixels.setPixelColor(i, r, g, b);
	}
	pixels.show();
}

uint8_t getRedValueFromColor(uint32_t c) {
    return c >> 16;
}

uint8_t getGreenValueFromColor(uint32_t c) {
    return c >> 8;
}

uint8_t getBlueValueFromColor(uint32_t c) {
    return c;
}

void printLines(uint32_t clr, std::vector<String> lines) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(clr);
    int x = _min(lines.size(),13);
    for (uint32_t index = 0; index < x; index++) {
        tft.setCursor(0,index*10);
        tft.print(lines[index]);
    }
    tft.writeFramebuffer();
}

// The loop function is called in an endless loop
void loop() {

	//Process
  switch (switchi) {
    case 0:
      tft.fillScreen(TFT_BLACK);
      switchi = 0;https://github.com/towarischtsch/gpn_wifinder/tree/master
    case 1:
      tft.fillScreen(TFT_WHITE);
      switchi = 0;
  }
	switch (deviceState) {
		case SCANNING    : doScan(); break;
		case PROBE       : doProbe(); break;
		case FOUND       : doRangeTest(); break;
	}

	//Set Strip color
	switch (deviceState) {
		case SCANNING   : setPixels(0,4,colorScanning); break;
		case PROBE      : setPixels(0,3,colorProbe); break;
		case FOUND      : setPixels(0,3,colorFound); break;
	}

	yield();
}

/**
 * Check networks in range if there is a open one.
 * If so, store network data, connect to network
 * and switch deviceState to probe
 */
void doScan() {

	//Fetch scanner class
	ESP8266WiFiScanClass scan = (ESP8266WiFiScanClass) WiFi;

	//Scan networks in range
    int n = scan.scanNetworks();
    std::vector<String> lines;
    for (int i = 0; i < n; i++) {
      lines.push_back(scan.SSID(i));
    }
    printLines(TFT_BLUE,lines);
    
    for (int i = 0; i < n; i++) {  
    	if (scan.encryptionType(i) == ENC_TYPE_NONE && !isOnBlacklist(scan.BSSIDstr(i))) {
    		Serial.print("Found network SSID : ");
    		Serial.println(scan.SSID(i));
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN);
    		tft.setCursor(0, 20);
        tft.print("Found network:");
        tft.setCursor(2, 91);
        tft.print(scan.SSID(i));
        tft.writeFramebuffer();

    		//Switch device state to probe
    		netSSID = scan.SSID(i);
    		netBSSID = scan.BSSIDstr(i);
    		netProbeStart = millis();
    		WiFi.begin(netSSID.c_str(), "");
    		deviceState = PROBE;
    	}
    }

}

/**
 * Run HTTP Probe. Request a page from the Internet and test
 * if the returned string matched the expected.
 */
void doProbe() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 20);
    tft.print("probing");
    tft.setCursor(2, 91);
    tft.print(netBSSID);
    tft.writeFramebuffer();


	//Test connection timeout
	if (millis() - netProbeStart > CONNECTION_TIMEOUT) {
		Serial.println("Probe aborted : send timeout");
		blacklist.push_back(netBSSID);
		deviceState = SCANNING;
		netSSID = "";
		netBSSID = "";
		return;
	}

	//Await WiFi connection
	while (WiFi.status() != WL_CONNECTED) return;

	//If the WiFi is connected, download test file
	if (WiFi.status() == WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 20);
    tft.print("connected!");
    tft.writeFramebuffer();

		HTTPClient client;
		client.begin(CONNECTION_TEST_URL);
	    int code = client.GET();
	    Serial.println("HTTP Code : ");
	    Serial.print(code);
		if (code == 200) {
			String data = client.getString();
			if (data.startsWith(CONNECTION_TEST_PREFIX)) {
				Serial.println("Connected to the Internet !");
				deviceState = FOUND;
			}
		}
		client.end();

	}

}


/**
 * There is now a open WiFi in range and we are connected to it.
 * If we move out of range, we lose the connection and switch back
 * to the scanning state.
 */
void doRangeTest() {

	if (WiFi.status() != WL_CONNECTED) {
		deviceState = SCANNING;
		netSSID = "";
		netBSSID = "";
	}

}

/**
 * Return whether or not the given bssid string has been blacklisted
 * @return true if the bssid has been blacklisted
 */
bool isOnBlacklist(String bssid) {

	for (uint32_t index = 0; index < blacklist.size(); index++) {
		if (blacklist[index].equals(bssid))
			return true;
	}

	return false;
}

/**
 * Clamp the given value between min and max
 * @param x the value to clamp
 * @param min the lower bound
 * @param max the upper bound
 * @return the clamped value
 */
float clamp(float x, float min, float max) {
	return x > max ? max : x < min ? min : x;
}



