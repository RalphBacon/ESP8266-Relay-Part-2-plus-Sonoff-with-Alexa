#include "Arduino.h"
// The essential ESP8266 WiFi library
#include <ESP8266WiFi.h>

// Your WiFi SSID and Password
const char* ssid = "YOUR SSID GOES HERE";
const char* password = "YOUR WIFI PASSWORD GOES HERE";

// The built in (blue) LED is on pin 1. We'll use GPIO2 for our output.
#define outPin 2
#define debug false

// Create our server and tell it the port we are listening on
WiFiServer server(80);


// Forward declaration of a function later in the sketch (not required for Arduino IDE)
void printWiFiStatus();

// -------------------------------------------------------------------------------
// SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP     SETUP
// -------------------------------------------------------------------------------
void setup(void) {

	// Speed of the serial stream, your ESP8266 may be on 115200
	Serial.begin(9600);
	Serial.println("Setup started.");

	// Set the mode as a STAtion (client) not an Access Point (server)
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	// We will assign the SAME IP Address each time otherwise you won't find the device with your phone!
	// In other words we are NOT using DHCP (a dynamically assigned IP address from your router.
	IPAddress ip(192, 168, 1, 85);
	IPAddress gateway(192,168,1,254);
	IPAddress subnet (255,255,255,0);
	WiFi.config(ip, gateway, subnet);

	// Configure GPIO2 as OUTPUT.
	pinMode(outPin, OUTPUT);

	// Start TCP server.
	server.begin();

	// At this point we close the 'debugging' serial port and re-open it as it has
	// been corrupted by the library code (don't ask. No, please do)
	Serial.end();
	Serial.begin(9600);
	Serial.println("Setup completed.");
}

// -------------------------------------------------------------------------------
// LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP     LOOP
// -------------------------------------------------------------------------------
void loop(void) {

	// Check if module is still connected to WiFi.
	if (WiFi.status() != WL_CONNECTED) {
		while (WiFi.status() != WL_CONNECTED) {
			Serial.println("ESP8266 not connected to your LAN - retrying");
			delay(500);
		}
		// Print the new IP to Serial.
		printWiFiStatus();
	}

	// Check if a client (eg browser) has connected
	WiFiClient client = server.available();

	// Some counts so we only print stuff occasionally
	static int waitCount = 0;
	static int printCount = 0;

	// Print a dot whilst we wait for a client to connect. Don't leave this in production code.
	if (!client) {
		printCount++;

		// Only print a dot after 10 tries to reduce IO
		if (printCount > 10) {
			if (debug) Serial.print(".");
			printCount = 0;
		}

		// Don't print on same line forever, throw a new line
		waitCount++;
		if (waitCount > 1000) {
			waitCount = 0;
			if (debug) Serial.print("\n\r");
		}

		delay(100);
		return;
	}

	waitCount = 0;
	if (debug) Serial.println("\n\rClient connected.");

	// Wait until the client sends some data, specifically the string "/gpio/1" or "/gpio/0"
	if (debug) Serial.println("Waiting for client to send data.");

	// Allow 5 seconds for data to arrive but then abort otherwise this code will "hang"
	unsigned long dataWait = 0;
	while (!client.available()) {
		if (dataWait > 5000) return;
		delay(1);
		dataWait++;
	}

	// Read the first line of the request
	client.setTimeout(1000);
	String request = client.readStringUntil('\r');

	// Show what we got on the debugging monitor
	if (debug) Serial.print("Read: ");
	if (debug) Serial.println(request);

	// Clear any other received characters after that carriage return
	client.flush();

	// Inspect the data (from the URI - browser request - perhaps) and extract the relevant bits
	static int status;
	if (request.indexOf("/gpio/0") != -1) {
		status = 0;
	}
	else if (request.indexOf("/gpio/1") != -1) {
		status = 1;
	}
	else {
		if (debug) Serial.println("Invalid request. Try again.");
	}

	// Tell the debugging monitor what happened
	if (debug) Serial.print("Status: ");
	if (debug) Serial.println(status ? "High" : "Low");

	// Set GPIO2 to high or low according to the request
	digitalWrite(outPin, status);

	// Prepare the response for the browser. For the phone app this is not required.
	String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
	s += (status) ? "high" : "low";
	s += "</html>\n";

	// Send the response HTML to the client
	client.print(s);
	delay(10);
	if (debug) Serial.println("Client disconnected");
	delay(100);

	// The client will actually be disconnected
	// when the function returns and 'client' object is destroyed
}

// -------------------------------------------------------------------------------
// Displays details of the WiFi connection (network ID, and IP address)
// -------------------------------------------------------------------------------
void printWiFiStatus() {
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}
