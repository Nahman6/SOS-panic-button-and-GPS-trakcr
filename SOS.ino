#include <WiFi.h>
#include "esp_bt.h"

#define SOS D3
#define SLEEP_PIN D2

boolean stringComplete = false;
String inputString = "";
String fromGSM = "";
int c = 0;
String SOS_NUM = "+919632997924";
int SOS_Time = 5;
bool CALL_END = 1;
char* response = " ";
String res = "";

void setup()
{
  WiFi.mode(WIFI_OFF);
  btStop();

  pinMode(SOS, INPUT_PULLUP);
  pinMode(SLEEP_PIN, OUTPUT);

  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, D0, D1);
  delay(20000);

  digitalWrite(SLEEP_PIN, LOW);

  Serial1.println("AT");
  delay(1000);
  Serial1.println("AT+GPS = 1");
  delay(1000);
  Serial1.println("AT+GPSLP = 2");
  delay(1000);
  Serial1.println("AT+SLEEP = 1");
  delay(1000);

  digitalWrite(SLEEP_PIN, HIGH);
}

void loop()
{
  if (Serial1.available()) 
  {
    char inChar = Serial1.read();
    if (inChar == '\n') 
    {
      if (fromGSM == "OK\r") {
        Serial.println("---------IT WORKS-------");
      }
      else if (fromGSM == "RING\r") {
        digitalWrite(SLEEP_PIN, LOW);
        Serial.println("---------ITS RINGING-------");
        Serial1.println("ATA");
      }
      else if (fromGSM == "ERROR\r") {
        Serial.println("---------IT DOESNT WORK-------");
      }
      else if (fromGSM == "NO CARRIER\r") { 
        Serial.println("---------CALL ENDS-------");
        CALL_END = 1;
        digitalWrite(SLEEP_PIN, HIGH);
      }

      Serial.println(fromGSM);
      fromGSM = "";
    } 
    else {
      fromGSM += inChar;
    }
    delay(20);
  }

  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }

  if (digitalRead(SOS) == LOW && CALL_END == 1)
  {
    Serial.print("Calling In..");
    for (c = 0; c < SOS_Time; c++)
    {
      Serial.println((SOS_Time - c));
      delay(1000);
      if (digitalRead(SOS) == HIGH)
        break;
    }

    if (c == 5)
    {
      digitalWrite(SLEEP_PIN, LOW);
      delay(1000);

      Serial1.println("AT+LOCATION = 2");
      Serial.println("AT+LOCATION = 2");

      while (!Serial1.available());
      while (Serial1.available()) {
        char add = Serial1.read();
        res = res + add;
        delay(1);
      }

      response = &res[0];
      Serial.print("Received Data - ");
      Serial.println(response);
      Serial.println();

      if (strstr(response, "GPS NOT")) {
        Serial.println("No Location data");
      } 
      else {
        int i = 0;
        while (response[i] != ',') i++;
        String location = String(response);
        String lat = location.substring(2, i);
        String longi = location.substring(i + 1);
        Serial.println(lat);
        Serial.println(longi);

        String Gmaps_link = "http://maps.google.com/maps?q=" + lat + "+" + longi;

        Serial1.println("AT+CMGF=1");
        delay(1000);
        Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"\r");
        delay(1000);
        Serial1.println("I'm here " + Gmaps_link);
        delay(1000);
        Serial1.println((char)26);
        delay(1000);
      }

      response = "";
      res = "";

      Serial.println("Calling Now");
      Serial1.println("ATD" + SOS_NUM);
      CALL_END = 0;
    }
  }

  if (stringComplete) {
    Serial1.print(inputString);
    inputString = "";
    stringComplete = false;
  }
}
