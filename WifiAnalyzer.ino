#include "Arduino.h"
#include <vector>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266HTTPClient.h>

#include "LEDStripCtrl.h"

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

//Device state colors
uint32_t colorBooting  = rgb(1.0f, 0.0f, 0.0f);	//Boot color (red)
uint32_t colorScanning = rgb(0.0f, 0.0f, 1.0f);	//Scanning color (blue)
uint32_t colorProbe    = rgb(1.0f, 1.0f, 0.0f);	//HTTP Probe running (yellow)
uint32_t colorFound    = rgb(0.0f, 1.0f, 0.0f);	//Open WiFi available (green)

String netSSID;					//SSID of the currently probed network
String netBSSID;				//BSSID of the currently probed network
uint32_t netProbeStart;			//time of http probe start
std::vector<String> blacklist;	//BSSID blacklist, contains networks witch have been probed and failed

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(115200);			//Begin serial communication at 115200 baud
	Serial.println("Booting...");	//Print booting message

	initStrip();					//Initialize the ws2812 strip
	setStripColor(colorBooting);	//Change strip color

	ESP.eraseConfig();				//Clear WiFi credentials
	WiFi.mode(WIFI_STA);			//Switch device into station mode

	Serial.println("Done");			//Print boot done message
}

// The loop function is called in an endless loop
void loop() {

	//Process
	switch (deviceState) {
		case SCANNING    : doScan(); break;
		case PROBE       : doProbe(); break;
		case FOUND       : doRangeTest(); break;
	}

	//Set Strip color
	switch (deviceState) {
		case SCANNING   : setStripColor(colorScanning); break;
		case PROBE      : setStripColor(colorProbe); break;
		case FOUND      : setStripColor(colorFound); break;
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
    for (int i = 0; i < n; i++) {
    	if (scan.encryptionType(i) == ENC_TYPE_NONE && !isOnBlacklist(scan.BSSIDstr(i))) {
    		Serial.print("Found network SSID : ");
    		Serial.println(scan.SSID(i));

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
		Serial.println("Probe : connected to network");

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



