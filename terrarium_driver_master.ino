
#include "credentials.h"
#include <Wire.h>
#include "easyautomate_network.h"
#include <string.h>
#include <ArduinoJson.h>

const char* json_buffer = "";
const char* device_name = "kermits_terrarium";

unsigned long currentMillis = 0;
unsigned long previousReportMillis = 0;

bool light_on = false;
bool uv_on = false;
float temp0set = 30.0;
float dim_factor = 0;
float p_factor = 5.0;
float i_factor = 0.05;
float d_factor = 3.0;

int ind0;
int ind1;
int ind2;

WiFiClientSecure secureClient;
EasyautomateNetwork client(device_name, api_key, secureClient);

char command_buffer[32];

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setCACert();
  Wire.begin();
}

void loop() {
  delay(60000);
  DynamicJsonDocument root = client.getSettings();
  
  sendSettingsMessage();
  decodeJsonObjectSettings(root);
  String msg;
  Wire.requestFrom(0x02, 21);    // request 21 bytes from slave device #8
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read(); // receive a byte as character
    if (isAscii(c)) {  // tests if c is an Ascii character to avoid garbage if message is shorter than requested
      msg += c;
    }    
  }
  Serial.println(msg);

  ind0 = msg.indexOf(',');
  String temp0 = msg.substring(0, ind0);
  ind1 = msg.indexOf(',', ind0+1 );
  String temp1 = msg.substring(ind0+1, ind1);
  ind2 = msg.indexOf(',', ind1+1);
  String pid = msg.substring(ind1+1);

  DynamicJsonDocument doc(1024);
  JsonObject device  = doc.createNestedObject("device");
  JsonObject measurements  = device.createNestedObject("measurements");
  measurements["t0"] = temp0;
  measurements["t1"] = temp1;
  measurements["pid"] = pid;
  device["name"] = device_name;
  serializeJson(doc, Serial);

  String output;
  serializeJson(doc, output);
  client.sendReports(output);
}

void decodeJsonObjectSettings(DynamicJsonDocument root){
  if (root.containsKey("data")) {
    JsonObject data = root["data"];
    if (data.containsKey("attributes")) {
      JsonObject attributes = data["attributes"];
      if (attributes.containsKey("current_settings")) {
        JsonObject current_settings = attributes["current_settings"];
        if (current_settings.containsKey("lights")) {
          JsonObject lights = current_settings["lights"];
          light_on = lights["on"];
          dim_factor = lights["dim_factor"];
          p_factor = lights["p_factor"];
          i_factor = lights["i_factor"];
          d_factor = lights["d_factor"];
          temp0set = lights["temp0set"];
          uv_on = lights["uv_on"];
        }
      }
    }
  }
}

void sendSettingsMessage()
{
  Wire.beginTransmission(0x02);
  String msg;
  byte error;
  msg += light_on == true ? "1" : "0";
  msg += ",";
  msg += String(temp0set);
  msg += ",";
  msg += String(dim_factor);
  msg += ",";
  msg += String(p_factor);
  msg += ",";
  msg += String(i_factor);
  msg += ",";
  msg += String(d_factor);
  msg += ",";
  msg += uv_on == true ? "1" : "0";
  Serial.println(msg);
  msg.toCharArray(command_buffer, 32);
  Wire.write(command_buffer);
  error = Wire.endTransmission(); //message buffer is sent with Wire.endTransmission()
  Serial.println(error);
}
