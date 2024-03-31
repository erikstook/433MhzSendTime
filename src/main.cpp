#include <ESP8266WiFi.h>
#include <time.h>
#include <RCSwitch.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <NewRemoteTransmitter.h>
#include <ArduinoOTA.h>

RCSwitch mySwitch = RCSwitch();


char switchesON[9][25] = {
  "010100000000000000010101",
  "010100000100000000010101",
  "010100000001000000010101",
  "010100000101000000010101",
  "010100000000010000010101",
  "010100000100010000010101",
  "010100000001010000010101",
  "010100000101010000010101",
  "010100000000000100010101",
  };
  char switchesOFF[9][25] = {
    "010100000000000000010100",
    "010100000100000000010100",
    "010100000001000000010100",
    "010100000101000000010100",
    "010100000000010000010100",
    "010100000100010000010100",
    "010100000001010000010100",
    "010100000101010000010100",
    "010100000000000100010100",
    };
String monthOfTheYear[12] = {
  "Januari",
  "Februari",
  "Mars",
  "April",
  "Maj",
  "Juni",
  "Juli",
  "Augusti",
  "September",
  "Oktober",
  "November",
  "December",

};
//switchesON[0][] ={ "010100000000000000010101"};
uint32_t actualTime = 0;
bool lightFlag = false;
bool startFlag = false;
bool stopFlag = false;
bool allLampsOn = false;
int allLampsOnCounter = 0;
int sensorPin = A0; //define analog pin 2
int digitalLight = D6;
int digitalLightValue;
bool value = false;
int lightValue = 1000;
const char* ssid = "ASUS";
const char* password = "guildford";
int timezone = 3;
int timeGetHour = 0;
int timeGetMinutes = 0;
int dst = 0;
int minutes;
int daysOfTheWeek;
int valueOfTheLamp[9];
int startLightValue = 800;
int StartTimeMin = 00;
int StartTimeHourStop = 6;
int StartTimeHour = 14;
int StopTimeHour = 21;
int StopTimeMin = 32;
int lampsStartedMin = 00;
int lampsStartedHour = 00;
bool timerStartStop = false;
int ledPin = D0;
String timedate = "";
String timedate2 = "";
const unsigned long TRANSMITTER_ADDRESSNEW = 25606094; //My personal transmitter address
const int PIN_TRANSMITTER = 4;  // GPIO4 / D2
NewRemoteTransmitter transmitternew(TRANSMITTER_ADDRESSNEW, PIN_TRANSMITTER, 230, 3);

WiFiServer server(80);


WiFiUDP UDP;                     // Create an instance of the WiFiUDP class to send and receive

IPAddress timeServerIP;          // time.nist.gov NTP server address
const char* NTPServerName = "0.se.pool.ntp.org";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

byte NTPBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

int dayofweek(time_t now, int tz_offset) {
	// Calculate number of seconds since midnight 1 Jan 1970 local time
	time_t localtime = now + (tz_offset * 60 * 60);
	// Convert to number of days since 1 Jan 1970
	int days_since_epoch = localtime / 86400;
	// 1 Jan 1970 was a Thursday, so add 4 so Sunday is day 0, and mod 7
	int day_of_week = (days_since_epoch + 4) % 7; 

	return day_of_week;
}


uint32_t getTime() {
  if (UDP.parsePacket() == 0) { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  uint32_t UNIXtime = UNIXtime + 3600; // Vintertid
  return UNIXTime;
}

void sendNTPpacket(IPAddress& address) {
  memset(NTPBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011;   // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime) {
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime) {
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime) {
  return UNIXTime / 3600 % 24;
}


void startUDP() {
  Serial.println("Starting UDP");
  UDP.begin(123);                          // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

void setup() {
  pinMode(digitalLight, INPUT_PULLUP);
  pinMode(PIN_TRANSMITTER, OUTPUT);
  Serial.begin(115200);
  Serial.print("Serial");
  mySwitch.enableTransmit(4); // GPIO4
  mySwitch.setPulseLength(320);
  // Optional set pulse length.
  // mySwitch.setPulseLength(320);

  // Optional set protocol (default is 1, will work for most outlets)
  // mySwitch.setProtocol(2);

  // Optional set number of transmission repetitions.
  mySwitch.setRepeatTransmit(10);

  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to my WiFi");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  server.begin();
  Serial.println("Server started");
    ArduinoOTA.setPort(8266); // SET to FIXED PORT
    ArduinoOTA.onStart([]() {
         Serial.println("Start");
       });
       ArduinoOTA.onEnd([]() {
         Serial.println("\nEnd");
       });
       ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
         Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
       });
       ArduinoOTA.onError([](ota_error_t error) {
         Serial.printf("Error[%u]: ", error);
         if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
         else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
         else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed Firewall Issue ?");
         else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
         else if (error == OTA_END_ERROR) Serial.println("End Failed");
       });
       ArduinoOTA.begin();
       Serial.println("OTA Ready");
       Serial.print("IP address: ");
       Serial.println(WiFi.localIP());
    // Print the IP address
  //  Serial.println(WiFi.localIP());
      // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  startUDP();

  if(!WiFi.hostByName(NTPServerName, timeServerIP)) { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);

  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);

}


void lampsON(){
  Serial.print(" Tänder ALLA ");
  for (int s = 0; s < 9; s++){
    Serial.print(" Tänder: ");
    Serial.println(s);
    mySwitch.send(switchesON[s]);
    delay(1000);
    mySwitch.send(switchesON[s]);
    delay(1000);
    valueOfTheLamp[s] = HIGH;
    value = true;

  }
  transmitternew.sendUnit(7, 1);// NEW NEXA Transmitter
  Serial.println(" Lamp 8 NEW ON");
  delay(5000);

}
void lampsOFF(){

    Serial.print(" Släcker ALLA ");
    for (int s = 0; s < 9; s++){
      Serial.print(" Släcker: ");
      Serial.println(s);
      mySwitch.send(switchesOFF[s]);
      delay(1000);
      mySwitch.send(switchesOFF[s]);
      delay(1000);
      valueOfTheLamp[s] = LOW;
      value = false;

    }
    transmitternew.sendUnit(7, 0); // NEW NEXA Transmitter
    Serial.println(" Lamp 8 NEW OFF");
    delay(5000);
}

void webServer(){
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("Webserver read");
  int testraknare = 1;
  while(!client.available()){
    testraknare = testraknare + 1;
    //Serial.print("*");
    if (testraknare > 1000) {
      return;
    }
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request

  //int value = false;

  if (request.indexOf("/LED=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    Serial.println(" All lamps ON");
      //mySwitch.send(switchesON[3]);
    valueOfTheLamp[3] = HIGH;
    lampsON(); // All lamps ON
    value = true;

  }
  if (request.indexOf("/LED=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
    Serial.println(" All lamps OFF");
    //mySwitch.send(switchesOFF[3]);
    valueOfTheLamp[3] = LOW;
    lampsOFF(); // All Lamps switchesOFF
    value = false;
  }

  if (request.indexOf("/LAMP1=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[0]);
    Serial.println(" Lamp 1 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[0] = HIGH;
  }
  if (request.indexOf("/LAMP1=OFF") != -1){
    // digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 1 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[0]);
    //lampsOFF();
    valueOfTheLamp[0] = LOW;
  }
  if (request.indexOf("/LAMP2=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[1]);
    Serial.println(" Lamp 2 ON");
    lightFlag = true;
    //lampsON();
        valueOfTheLamp[1] = HIGH;
  }
  if (request.indexOf("/LAMP2=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 2 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[1]);
    //lampsOFF();
    valueOfTheLamp[1] = LOW;
  }
  if (request.indexOf("/LAMP3=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[2]);
    Serial.println(" Lamp 3 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[2] = HIGH;
  }
  if (request.indexOf("/LAMP3=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 3 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[2]);
    //lampsOFF();
    valueOfTheLamp[2] = LOW;
  }
  if (request.indexOf("/LAMP4=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[3]);
    Serial.println(" Lamp 4 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[3] = HIGH;
  }
  if (request.indexOf("/LAMP4=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 4 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[3]);
    //lampsOFF();
    valueOfTheLamp[3] = LOW;
  }
  if (request.indexOf("/LAMP5=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[4]);
    Serial.println(" Lamp 5 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[4] = HIGH;
  }
  if (request.indexOf("/LAMP5=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 5 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[4]);
    //lampsOFF();
    valueOfTheLamp[4] = LOW;
  }
  if (request.indexOf("/LAMP6=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[5]);
    Serial.println(" Lamp 6 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[5] = HIGH;
  }
  if (request.indexOf("/LAMP6=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 6 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[5]);
    //lampsOFF();
    valueOfTheLamp[5] = LOW;
  }


  if (request.indexOf("/LAMP7=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[6]);
    Serial.println(" Lamp 7 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[6] = HIGH;
  }
  if (request.indexOf("/LAMP7=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 7 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[6]);
    //lampsOFF();
    valueOfTheLamp[6] = LOW;
  }

  if (request.indexOf("/LAMP8=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    mySwitch.send(switchesON[7]);
    Serial.println(" Lamp 8 ON");
    lightFlag = true;
    //lampsON();
    valueOfTheLamp[7] = HIGH;
  }
  if (request.indexOf("/LAMP8=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
      Serial.println(" Lamp 8 OFF");
      lightFlag = false;
    mySwitch.send(switchesOFF[7]);
    //lampsOFF();
    valueOfTheLamp[7] = LOW;
  }

  if (request.indexOf("/LAMP9=ON") != -1) {
    //digitalWrite(ledPin, LOW);
    //mySwitch.send(switchesON[8]);
    //Serial.println(" Lamp 9 ON");
    transmitternew.sendUnit(7, 1);// NEW NEXA Transmitter
    Serial.println(" Lamp 8 NEW ON");

    lightFlag = true;
    //lampsON();
    valueOfTheLamp[8] = HIGH;
  }
  if (request.indexOf("/LAMP9=OFF") != -1){
    //digitalWrite(ledPin, HIGH);
    transmitternew.sendUnit(7, 0); // NEW NEXA Transmitter
    Serial.println(" Lamp 8 NEW OFF");
    lightFlag = false;
  //  mySwitch.send(switchesOFF[8]);
    //lampsOFF();
    valueOfTheLamp[8] = LOW;
  }

 if (request.indexOf("/STARTTIME20+") != -1){
    StartTimeMin = StartTimeMin + 20;
    if (StartTimeMin > 59){
      StartTimeHour = StartTimeHour + 1;
      StartTimeMin = StartTimeMin -60;
    }
  }
  if (request.indexOf("/STARTTIME20-") != -1){
    StartTimeMin = StartTimeMin - 20;
    if (StartTimeMin < 0){
      StartTimeHour = StartTimeHour - 1;
      StartTimeMin = StartTimeMin + 60;
      if (StartTimeHour < 0 && StartTimeMin < 0) {
        StartTimeHour = 0;
        StartTimeMin = 0;
      }
      //StartTimeMin = 0;
    }
  }


  if (request.indexOf("/STARTTIME+") != -1){
    StartTimeMin = StartTimeMin + 2;
    if (StartTimeMin > 58){
      StartTimeHour = StartTimeHour + 1;
      StartTimeMin = 0;
    }
  }
  if (request.indexOf("/STARTTIME-") != -1){
    StartTimeMin = StartTimeMin - 2;
    if (StartTimeMin < 0){
      StartTimeHour = StartTimeHour - 1;
      StartTimeMin = 58;
      if (StartTimeHour < 0 && StartTimeMin < 0) {
        StartTimeHour = 0;
        StartTimeMin = 0;
      }
      //StartTimeMin = 0;
    }
  }

  if (request.indexOf("/STOPTIME20+") != -1){
    StopTimeMin = StopTimeMin + 20;
    if (StopTimeMin > 59){
      StopTimeHour = StopTimeHour + 1;
      StopTimeMin = StopTimeMin - 60;
    }
  }
  if (request.indexOf("/STOPTIME20-") != -1){
    StopTimeMin = StopTimeMin - 20;
    if (StopTimeMin < 0){
      StopTimeHour = StopTimeHour - 1;
      StopTimeMin = StopTimeMin +60;
      if (StopTimeHour < 0 && StopTimeMin < 0) {
        StopTimeHour = 0;
        StopTimeMin = 0;
      }
      //StartTimeMin = 0;
    }
  }


  if (request.indexOf("/STOPTIME+") != -1){
    StopTimeMin = StopTimeMin + 2;
    if (StopTimeMin > 58){
      StopTimeHour = StopTimeHour + 1;
      StopTimeMin = 0;
    }
  }
  if (request.indexOf("/STOPTIME-") != -1){
    StopTimeMin = StopTimeMin - 2;
    if (StopTimeMin < 0){
      StopTimeHour = StopTimeHour - 1;
      StopTimeMin = 58;
      if (StopTimeHour < 0 && StopTimeMin < 0) {
        StopTimeHour = 0;
        StopTimeMin = 0;
      }
      //StartTimeMin = 0;
    }
  }

  if (request.indexOf("/STARTTIMER") != -1){
    //digitalWrite(ledPin, HIGH);
    timerStartStop = true;
  }
  if (request.indexOf("/STOPTIMER") != -1){
    //digitalWrite(ledPin, HIGH);
    timerStartStop = false;
  }


  // Return the response
  // åäöÅÄÖ: &aring; &auml; &ouml; &Aring; &Auml; &Ouml;
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<head>");
  client.println("<h1>S&aumltt p&aring lamporna 209. <br>");
  client.print(" Klockan:");
  client.print(timeGetHour);
  client.print(":");

  if (timeGetMinutes < 10 ) {
    if (timeGetMinutes == 0 ) {
      client.println("00");
    } else {
      client.print("0");
      client.println(timeGetMinutes);
      }
    } else {
      client.println(timeGetMinutes);
    }
  client.print("  Ljus:");
  client.println(lightValue);
  client.print("  LjusDIGITALT:");
  client.println(digitalLightValue);
  client.print("<br>  Start ljusv&aumlrde:");
  client.println(startLightValue);
  client.print("  Ljuset startat:");
  client.print(lampsStartedHour);
  client.print(":");
  client.print(lampsStartedMin);


  client.println("</h1>");
//  client.println("<meta http-equiv=\"refresh\" content=\"60\">");
  client.println("</head>");
  client.println("<html>");
  client.println("<body style=\"background-color:lightblue;\">");
  if(value == true) {
    client.print("<font color=\"FF3333\">All ON </font>");
  } 
  if (value == false) {
    client.print("<font color=\"0000FF\">All OFF </font>");
  }

  client.println("<br><br>");

  if(valueOfTheLamp[0] == HIGH) {
    client.print("<font color=\"FF3333\">No 1 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 1 OFF </font>");
  }
  //client.println("<br>");
  if(valueOfTheLamp[1] == HIGH) {
    client.print("<font color=\"FF3333\">No 2 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 2 OFF </font>");
  }
  client.println("<br>");
  if(valueOfTheLamp[2] == HIGH) {
    client.print("<font color=\"FF3333\">No 3 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 3 OFF </font>");
  }
//  client.println("<br>");

  if(valueOfTheLamp[3] == HIGH) {
    client.print("<font color=\"FF3333\">No 4 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 4 OFF </font>");
  }
  client.println("<br>");

  if(valueOfTheLamp[4] == HIGH) {
    client.print("<font color=\"FF3333\">No 5 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 5 OFF </font>");
  }
  //client.println("<br>");

  if(valueOfTheLamp[5] == HIGH) {
    client.print("<font color=\"FF3333\">No 6 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 6 OFF </font>");
  }
  client.println("<br>");

  if(valueOfTheLamp[6] == HIGH) {
    client.print("<font color=\"FF3333\">No 7 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 7 OFF </font>");
  }
  //client.println("<br>");

  if(valueOfTheLamp[7] == HIGH) {
    client.print("<font color=\"FF3333\">No 8 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 8 OFF </font>");
  }
  //client.println("<br>");

  if(valueOfTheLamp[8] == HIGH) {
    client.print("<font color=\"FF3333\">No 9 ON </font>");
  } else {
    client.print("<font color=\"0000FF\">No 9 OFF </font>");
  }
  client.println("<br><br>");

  client.println("<a href=\"/LED=ON\"><button type=\"button\"><font color=\"FF3333\">ALL ON!</font></button></a>");
  client.println("<a href=\"/LED=OFF\"><button type=\"button\"><font color=\"0000FF\">ALL OFF!</font></button></a><br><br>");
  client.println("<a href=\"/LAMP1=ON\"><button type=\"button\"><font color=\"FF3333\">No 1 ON!</font></button></a>");
  client.println("<a href=\"/LAMP1=OFF\"><button type=\"button\"><font color=\"0000FF\">No 1 OFF!</font></button></a>");
  client.println("<a href=\"/LAMP2=ON\"><button type=\"button\"><font color=\"FF3333\">No 2 ON!</font></button></a>");
  client.println("<a href=\"/LAMP2=OFF\"><button type=\"button\"><font color=\"0000FF\">No 2 OFF!</font></button></a><br><br>");
  client.println("<a href=\"/LAMP3=ON\"><button type=\"button\"><font color=\"FF3333\">No 3 ON!</font></button></a>");
  client.println("<a href=\"/LAMP3=OFF\"><button type=\"button\"><font color=\"0000FF\">No 3 OFF!</font></button></a>");
  client.println("<a href=\"/LAMP4=ON\"><button type=\"button\"><font color=\"FF3333\">No 4 ON!</font></button></a>");
  client.println("<a href=\"/LAMP4=OFF\"><button type=\"button\"><font color=\"0000FF\">No 4 OFF!</font></button></a><br><br>");
  client.println("<a href=\"/LAMP5=ON\"><button type=\"button\"><font color=\"FF3333\">No 5 ON!</font></button></a>");
  client.println("<a href=\"/LAMP5=OFF\"><button type=\"button\"><font color=\"0000FF\">No 5 OFF!</font></button></a>");
  client.println("<a href=\"/LAMP6=ON\"><button type=\"button\"><font color=\"FF3333\">No 6 ON!</font></button></a>");
  client.println("<a href=\"/LAMP6=OFF\"><button type=\"button\"><font color=\"0000FF\">No 6 OFF!</font></button></a><br><br>");
  client.println("<a href=\"/LAMP7=ON\"><button type=\"button\"><font color=\"FF3333\">No 7 ON!</font></button></a>");
  client.println("<a href=\"/LAMP7=OFF\"><button type=\"button\"><font color=\"0000FF\">No 7 OFF!</font></button></a>");
  client.println("<a href=\"/LAMP8=ON\"><button type=\"button\"><font color=\"FF3333\">No 8 ON!</font></button></a>");
  client.println("<a href=\"/LAMP8=OFF\"><button type=\"button\"><font color=\"0000FF\">No 8 OFF!</font></button><br><br>");
  client.println("<a href=\"/LAMP9=ON\"><button type=\"button\"><font color=\"FF3333\">No 9 ON!</font></button></a>");
  client.println("<a href=\"/LAMP9=OFF\"><button type=\"button\"><font color=\"0000FF\">No 9 OFF!</font></button></b><br><br>");
  client.println("<a><h2>- SET START/STOP-TIMER - </h2></a><br>");
  client.println("<a href=\"/STARTTIME+\"><button type=\"button\"><font color=\"0000FF\">STARTTIME + 2min</font></button></a>");
  client.println("<a href=\"/STARTTIME-\"><button type=\"button\"><font color=\"0000FF\">STARTTIME - 2min</font></button></a>");

  client.println("<a href=\"/STARTTIME20+\"><button type=\"button\"><font color=\"0000FF\">STARTTIME + 20min</font></button></a>");
  client.println("<a href=\"/STARTTIME20-\"><button type=\"button\"><font color=\"0000FF\">STARTTIME - 20min</font></button></a>");

  //client.println("<button type=\"button\"><button color=\"FF3333\">Time Up</button></a><br><br>");
  client.println("<a><b> Start time:</a>");
  client.println(StartTimeHour);
  client.println(":");
  client.println(StartTimeMin);
  client.println("<br><br>");
  //client.println(sensorValue);

  client.println("<a href=\"/STOPTIME+\"><button type=\"button\"><font color=\"0000FF\">STOPTIME + 2min</font></button></a>");
  client.println("<a href=\"/STOPTIME-\"><button type=\"button\"><font color=\"0000FF\">STOPTIME - 2min</font></button></a>");

  client.println("<a href=\"/STOPTIME20+\"><button type=\"button\"><font color=\"0000FF\">STOPTIME + 20min</font></button></a>");

  client.println("<a href=\"/STOPTIME20-\"><button type=\"button\"><font color=\"0000FF\">STOPTIME - 20min</font></button></a>");
 

 //client.println("<button type=\"button\"><button color=\"FF3333\">Time Up</button></a><br><br>");
  client.println("<a><b> Stop time:</a>");
  client.println(StopTimeHour);
  client.println(":");
  client.println(StopTimeMin);
  client.println("</b><br><br>");
  //client.println(sensorValue);
if (timerStartStop == false){
  client.println("<a href=\"/STARTTIMER\"><button type=\"button\"><font color=\"0000FF\">START THE TIMER(OFF)</font></button></a>");
} else {
    client.println("<a href=\"/STOPTIMER\"><button type=\"button\"><font color=\"FF3333\">STOP THE TIMER(ON)</font></button></a>");
}
  client.println("</body>");
  client.println("</html>");
  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}

unsigned long intervalNTP = 300000; // Request NTP time every 18 minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;

unsigned long prevActualTime = 0;

void loop() {

  unsigned long currentMillis = millis();
ArduinoOTA.handle();
  if (currentMillis - prevNTP > intervalNTP) { // If a minute has passed since last NTP request
    prevNTP = currentMillis;
    Serial.println("\r\nSending NTP request ...");
    sendNTPpacket(timeServerIP);               // Send an NTP request
  }
  else {
    Serial.print("No NTP answer");
  }

  uint32_t time = getTime();                   // Check if an NTP response has arrived and get the (UNIX) time
  if (time) {                                  // If a new timestamp has been received
    timeUNIX = time;
    Serial.print("NTP response:\t");
    Serial.println(timeUNIX);
    lastNTPResponse = currentMillis;
  } else if ((currentMillis - lastNTPResponse) > 3600000) {
    Serial.println("More than 1 hour since last NTP response. Rebooting.");
    Serial.flush();
    ESP.reset();
  }

  uint32_t actualTime = timeUNIX + (currentMillis - lastNTPResponse)/1000;
  if (actualTime != prevActualTime && timeUNIX != 0) { // If a second has passed since last print
    prevActualTime = actualTime;
    //Serial.printf("\rUTC time:\t%d:%d:%d   ", getHours(actualTime), getMinutes(actualTime), getSeconds(actualTime));
    timeGetMinutes = getMinutes(actualTime);
    timeGetHour = getHours(actualTime) + 2;  // Sommartid + 2, Vintertid + 1
    if (timeGetHour == 23){
      timeGetHour = 01;
    }
    if (timeGetHour == 24){
      timeGetHour = 02;
    }
   // Serial.print(timeGetHour);
    //Serial.print(":");
    //Serial.println(timeGetMinutes);
   }

//getTheTime();

//lampsON();

Serial.print("Lampor= ");
for (int k = 0; k < 9;k++){

  Serial.print(k + 1);
  Serial.print("=>");
  Serial.print(valueOfTheLamp[k]);
  Serial.print(" ");
}
digitalLightValue = digitalRead(digitalLight);
Serial.print("   LjusetDIGTALT:");
Serial.print(digitalLightValue);
lightValue = analogRead(sensorPin);


Serial.println(lightValue, DEC);
lightValue = 1024 - lightValue;// Invert the meassure
Serial.print(" Efter 1024-värde :");
Serial.println(lightValue, DEC); // light intensity
Serial.print(" KLOCKAN TID:");
Serial.print(timeGetHour);
Serial.print(":");
Serial.println(timeGetMinutes);
// ******************* START 
StartTimeHourStop = StartTimeHour + 6;
if (timeGetHour >= StartTimeHour && timeGetHour <= StartTimeHourStop) {
  Serial.print(" Nu kanske lamporna kan TÄNDAS:");
  Serial.print(timeGetHour); 
  Serial.print(" min:");
  Serial.println(timeGetMinutes);

  if (timeGetMinutes >= StartTimeMin){
  startFlag = true; // Now the lights may start
  stopFlag = false;
  Serial.print(" Nu kan lamporna TÄNDAS:");
  Serial.print(timeGetHour);
  Serial.print(":");
  Serial.println(timeGetMinutes);
  Serial.print(" startFlag:");
  Serial.print(startFlag);
  Serial.print(" stopFlag:");
  Serial.println(stopFlag);
  }
}
// ******************* START 
// ******************* STOP
if (timeGetHour >= StopTimeHour) { 
  Serial.print(" Nu kanske lamporna kan SLÄCKAS:");
  Serial.print(timeGetHour);
  Serial.print(" min:");
  Serial.println(timeGetMinutes);

  if (timeGetMinutes >= StopTimeMin){
  stopFlag = true;
  startFlag = false; // Now the lights STOPS
  Serial.print(" Nu kan lamporna SLÄCKAS:");
  Serial.print(" startFlag:");
  Serial.print(startFlag);
  Serial.print(" stopFlag:");
  Serial.println(stopFlag);
  }

}
// ******************* STOP
if(lightValue <= startLightValue && !allLampsOn && startFlag && !stopFlag){ // START the lamps
    Serial.print(" START allLampsCounter:");
    Serial.println(allLampsOnCounter);

  if (allLampsOnCounter <= 3){
    allLampsOnCounter = allLampsOnCounter + 1;
    mySwitch.send(switchesON[3]);
    valueOfTheLamp[3] = HIGH;
    lampsON();
    allLampsOn = true ;
    lampsStartedHour = timeGetHour;
    lampsStartedMin = timeGetMinutes;
    Serial.print(" LAMPS ON !!***");
    Serial.print(" startFlag:");
    Serial.print(startFlag);
    Serial.print(" stopFlag:");
    Serial.print(stopFlag);
    Serial.print(" allLampOn:");
    Serial.print(allLampsOn);
    Serial.print(" allLampsCounter:");
    Serial.println(allLampsOnCounter);

    } 
}

if( allLampsOn && !startFlag && stopFlag){ // STOP the LAMPS
    Serial.print("STOP allLampsCounter:");
    Serial.print(allLampsOnCounter);

  if (allLampsOnCounter >= 1){
    mySwitch.send(switchesOFF[3]);
    valueOfTheLamp[3] = LOW;
    lampsOFF();
    allLampsOn = false;
    startFlag = false;
    stopFlag = false; 
    allLampsOnCounter = allLampsOnCounter - 1;
    Serial.print(" LAMPS OFF ***");
    Serial.print(" startFlag:");
    Serial.print(startFlag);
    Serial.print(" stopFlag:");
    Serial.print(stopFlag);
    Serial.print(" allLampOn:");
    Serial.print(allLampsOn);
    Serial.print(" allLampsCounter:");
    Serial.println(allLampsOnCounter);

  }
}
Serial.print(" Start value:");
Serial.print(startLightValue);
Serial.print(" allLampsOn:");
Serial.print(allLampsOn);
Serial.print(" lightValue:");
Serial.print(lightValue);
Serial.print(" Starttid:");
Serial.print(StartTimeHour);
Serial.print(":");
Serial.print(StartTimeMin);
Serial.print(" Stoptid:");
Serial.print(StopTimeHour);
Serial.print(":");
Serial.print(StopTimeMin);
Serial.print("  Value:");
Serial.println(value); 
Serial.print("  DIGITAL Value:");
Serial.println(digitalLightValue);


delay(1500);

webServer();

}
