/****************************************************************************************************************************
  WebSocketClient_NINA.ino
  For boards using WiFiNINA Shield/Module

  Blynk_WiFiNINA_WM is a library for the Mega, Teensy, SAM DUE, nRF52, STM32 and SAMD boards
  (https://github.com/khoih-prog/Blynk_WiFiNINA_WM) to enable easy configuration/reconfiguration and
  autoconnect/autoreconnect of WiFiNINA/Blynk

  Based on and modified from WebSockets libarary https://github.com/Links2004/arduinoWebSockets
  to support other boards such as  SAMD21, SAMD51, Adafruit's nRF52 boards, etc.

  Built by Khoi Hoang https://github.com/khoih-prog/WebSockets_Generic
  Licensed under MIT license

  Created on: 24.05.2015
  Author: Markus Sattler
 *****************************************************************************************************************************/


#define _WEBSOCKETS_LOGLEVEL_     2
#define WEBSOCKETS_NETWORK_TYPE   NETWORK_WIFININA

#include <WiFiNINA_Generic.h>
#include <Arduino_LSM6DS3.h>
#include <WebSocketsClient_Generic.h>

WebSocketsClient webSocket;

int status = WL_IDLE_STATUS;

#define USE_SSL         false

#if USE_SSL
  // Deprecated echo.websocket.org to be replaced
  #define WS_SERVER           "44.207.82.113"
  #define WS_PORT             8080
#else
  // To run a local WebSocket Server
  #define WS_SERVER           "44.207.82.113"
  #define WS_PORT             8080
#endif

///////please enter your sensitive data in the Secret tab/arduino_secrets.h

char ssid[] = "UCLA_WEB";        // your network SSID (name)
char pass[] = "";         // your network password (use for WPA, or use as key for WEP), length must be 8+

bool alreadyConnected = false;

void webSocketEvent(const WStype_t& type, uint8_t * payload, const size_t& length)
{
    switch (type)
    {
        case WStype_DISCONNECTED:
            if (alreadyConnected)
            {
                Serial.println("[WSc] Disconnected!");
                alreadyConnected = false;
            }
            break;

        case WStype_CONNECTED:
            {
                alreadyConnected = true;
                Serial.print("[WSc] Connected to url: ");
                Serial.println((char *) payload);
                // send a message to server when Connected
                webSocket.sendTXT("Connected");
            }
            break;

        case WStype_TEXT:
            // Log the received text message
            Serial.print("[WSc] Received text: ");
            Serial.println((char *) payload);
            // Optionally, handle received text messages here
            break;

        case WStype_BIN:
            // Log binary messages (if needed)
            Serial.print("[WSc] Received binary length: ");
            Serial.println(length);
            // Optionally, handle binary data here
            break;

        case WStype_PING:
            // Log ping messages (if needed)
            Serial.println("[WSc] Received ping");
            break;

        case WStype_PONG:
            // Log pong messages (if needed)
            Serial.println("[WSc] Received pong");
            break;

        default:
            break;
    }
}


void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("WebSockets Client IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setup()
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);

  while (!Serial);

  Serial.print("\nStart WebSocketClient_NINA on ");
  Serial.println(WEBSOCKETS_GENERIC_VERSION);

  Serial.println("Used/default SPI pinout:");
  Serial.print("MOSI:");
  Serial.println(MOSI);
  Serial.print("MISO:");
  Serial.println(MISO);
  Serial.print("SCK:");
  Serial.println(SCK);
  Serial.print("SS:");
  Serial.println(SS);

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");

    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();

  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    //delay(10000);
  }

  printWifiStatus();

  // server address, port and URL
  Serial.print("WebSockets Server : ");
  Serial.println(WS_SERVER);

  // server address, port and URL
  webSocket.begin(WS_SERVER, WS_PORT, "/echo");

  // event handler
  webSocket.onEvent(webSocketEvent);

  // server address, port and URL
  Serial.print("Connected to WebSockets Server @ ");
  Serial.println(WS_SERVER);

    if (!IMU.begin()) {
      Serial.println("Failed to initialize IMU!");
      while (1);
  }
  
}


void loop() {
    webSocket.loop();

    static unsigned long lastSendTime = 0;
    unsigned long currentMillis = millis();

    // Send data every 100 milliseconds
    if (currentMillis - lastSendTime > 100) {
        lastSendTime = currentMillis;

        float ax, ay, az;
        if (IMU.accelerationAvailable()) {
            IMU.readAcceleration(ax, ay, az);

            // Construct JSON-formatted string
            String jsonMessage = "{\"type\": \"gyroscope\", \"value\": " + String(az) + "}";

            // Send this message over WebSocket
            webSocket.sendTXT(jsonMessage);
        }
    }
}
// void loop()
// {
//   webSocket.loop();
// }
