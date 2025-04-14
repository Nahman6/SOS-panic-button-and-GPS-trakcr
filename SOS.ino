#include <WiFi.h> `//WiFi library`
#include "esp_bt.h" `//function specific to ESP32 boards`


**_==//DECLARING VARIBLES
==

#define SOS D3 `//it says SOS means D3 (of ESP32) in this code`
#define SLEEP_PIN D2 `// Make this pin HIGH to make A9G board to go to sleep mode //SLEEP_PIN means D2 (of ESP32)`

boolean stringComplete = false; `// reserved for future manual testing features by handling manual **AT command input**`
String inputString = ""; `for serial input
String fromGSM = ""; `after a full ring, this string is evaluated to check: If the response is "RING"`, `"OK"`, `"NO CARRIER"` 
int c = 0; `counter for timing how long SOS button is pressed` 
String SOS_NUM = "+919632997924"; `number is stored in ESP32 to make calls and send messages`

int SOS_Time = 5; `//(loop count) Press the button 5 sec to activate SOS`

bool CALL_END = 1; `//Flag to indicate if the previous call has ended.`
char* response = " ";  `//C string response`
String res = ""; `//empty variable accumulates GPS response from GSM`

_Note: AT Commands or 'ATtention' Commands is **the basis of communication between any cellular (or RF) modems and its host controller**_

==_//void setup(), runs ONCE when the board powers on or is reset==

void setup()
{
`// Making Radio OFF for power saving`
  WiFi.mode(WIFI_OFF);  `// WiFi OFF`
  btStop();   `// Bluetooth OFF`

  pinMode(SOS, INPUT_PULLUP); `//input pin SOS button`

  pinMode(SLEEP_PIN, OUTPUT); `//A9G sleep control pin`

  Serial.begin(115200); `// For Serial Monitor; USB to computer at 115200 bits per second`
  Serial1.begin(115200, SERIAL_8N1, D0, D1); `// esp32 communicated to A9G Board

 `` // Waiting for A9G to setup everything for 20 sec`
  delay(20000);

==_//power management of A9G
==

  digitalWrite(SLEEP_PIN, LOW); `// Sleep Mode OFF for A9G`

  Serial1.println("AT"); `` // Just Checking`
  delay(1000);

  Serial1.println("AT+GPS = 1");     `// Turning ON GPS`
  delay(1000);

  Serial1.println("AT+GPSLP = 2");  `  // GPS low power`
  delay(1000);

  Serial1.println("AT+SLEEP = 1");  ` // Configuring Sleep Mode to 1, when instructed`
  delay(1000);

  digitalWrite(SLEEP_PIN, HIGH); `// Sleep Mode ON` 
}

==_Looping code_
==

void loop()
{
  `//listen from GSM Module`
  if (Serial1.available()) 
  {
  char inChar = Serial1.read(); `//if there is anything in A9G board, read a single character`

if (inChar == '\n')  `// if message ends with newline, check what the message was`
{
//check the state
      if (fromGSM == "OK\r") {
        Serial.println("---------IT WORKS-------"); `at serial monitor`
      }
      
else if (fromGSM == "RING\r")
{
        digitalWrite(SLEEP_PIN, LOW); `// A9G Sleep Mode OFF`
        Serial.println("---------ITS RINGING-------");
        Serial1.println("ATA"); `//answer the call`
      }
      
else if (fromGSM == "ERROR\r") {
        Serial.println("---------IT DOESNT WORK-------");
      }

else if (fromGSM == "NO CARRIER\r") { 
        Serial.println("---------CALL ENDS-------");
        CALL_END = 1;
        digitalWrite(SLEEP_PIN, HIGH); `//A9G Sleep Mode ON`
      }

`//write the actual response ``
 Serial.println(fromGSM);  `//print full message from GSM` 
 fromGSM = ""; `clear string for upcoming message`

} 
else {
      fromGSM += inChar; `when the incoming character **is not a newline**`
    }
    delay(20);
  }

  `// read from port 0, send to port 1:`
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte); `//manually send AT commands from your PC to the GSM module.`
  }

  `// When SOS button is pressed`
  if (digitalRead(SOS) == LOW && CALL_END == 1) `//low=pressed & call_end=1=yes`
  {
    Serial.print("Calling In.."); // Waiting for 5 sec
    for (c = 0; c < SOS_Time; c++)
    {
      Serial.println((SOS_Time - c));
      delay(1000);
      if (digitalRead(SOS) == HIGH)
        break; `// Waits for 5 seconds while checking if the button is still pressed. If the user lets go early → cancels the operation.`
    }
    if (c == 5) `//it’s a valid SOS trigger.`
    {

	  
 ==**// Request GPS Location 
==

   digitalWrite(SLEEP_PIN, LOW); `// Wake up GSM`
   delay(1000);

  Serial1.println("AT+LOCATION = 2");  `// Request current GPS coordinates`
  Serial.println("AT+LOCATION = 2");  `//print string 'AT+LOCATION=2' to serial monitor`

`// Wait and read the full response`
  while (!Serial1.available()); `//wait`
  while (Serial1.available()) { `//read one character`
  char add = Serial1.read(); `//store in 'char add'`
  res = res + add; `//add char to 'res' string`
  delay(1); `// small delay for reliable reading`

   }

  response = &res[0];  `// Convert res to C-string for strstr()->used to search inside the string`
  Serial.print("Received Data - ");
  Serial.println(response);
  Serial.println();

  

 ``  // === If GPS not available ===`

   if (strstr(response, "GPS NOT")) {
   Serial.println("No Location data");
   } 
   else {
   `// === Extract Coordinates ===`
   int i = 0;
   
   while (response[i] != ',') i++; `finds the comma ,`
    String location = String(response);
    String lat = location.substring(2, i);         `` // Latitude`
    String longi = location.substring(i + 1);     `// Longitude`
    Serial.println(lat);
    Serial.println(longi);

   String Gmaps_link = "http://maps.google.com/maps?q=" + lat + "+" + longi;**
`//http://maps.google.com/maps?q=38.9419+-78.3020`


==_//Sending SMS with Google Maps Link with our Location_
==

Serial1.println("AT+CMGF=1"); `//Configure Message Format i.e. sets GSM to SMS text mode`
delay(1000);
Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"\r"); `//commands GSM to send SMS to the sos number`
`// "\"\r is used for GSM to understand the command i.e. carriage return`
delay(1000);

Serial1.println ("I'm here " + Gmaps_link);
delay(1000);
Serial1.println((char)26); `/sends Ctrl+Z - go ahead and send the typed message`
delay(1000);
}

response = "";  `//clear the string `
res = "";

==`// Calling on that same number after sending SMS
==

	  
Serial.println("Calling Now");
 Serial1.println("ATD" + SOS_NUM); `// Dial number`
CALL_END = 0;  `// Call is now active`
    }
  }

  `//Send manual commands from Serial Monitor/
  if (stringComplete) {
    Serial1.print(inputString);
    inputString = "";
    stringComplete = false;
  }

}