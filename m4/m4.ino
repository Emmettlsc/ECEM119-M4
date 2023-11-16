#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Arduino_LSM6DS3.h>
#include <ArduinoJson.h>


char ssid[] = "UCLA_WEB";                // your network SSID (name)
char pass[] = "";                        // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;             // the Wi-Fi radio's status

char serverAddress[] = "plantoml.com";     //webserver address 
int port = 80;                            //webserver port
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

unsigned long previousMillis = 0;   
const long interval = 1000;             
const int batchSize = 10;
int batchCounter = 0;               
String batchData = "";

void setup() {
  Serial.begin(9600);
  while (!Serial) { }

  pinMode(LED_BUILTIN, OUTPUT);

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.println("You're connected to the network and IMU initialized!");
  Serial.println("---------------------------------------");
}

void loop() {
  float ax, ay, az;
  unsigned long currentMillis = millis();

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    if (batchCounter > 0) {
      batchData += ",";
    }
    batchData += String(ax);
    batchCounter++;

    if (batchCounter >= batchSize) {
      //send the batch data to the server
      client.beginRequest();
      client.post("/data");
      client.sendHeader("Content-Type", "text/plain");
      client.sendHeader("Content-Length", batchData.length());
      client.beginBody();
      client.print(batchData);
      client.endRequest();

      batchData = "";
      batchCounter = 0;

      int statusCode = client.responseStatusCode();
      String response = client.responseBody();
      Serial.print("Status code: ");
      Serial.println(statusCode);
      Serial.println(response);
    }
  }
}
