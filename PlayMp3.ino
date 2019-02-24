// this example will play a track and then 
// every five seconds play another track
//
// it expects the sd card to contain these three mp3 files
// but doesn't care whats in them
//
// sd:/mp3/0001.mp3
// sd:/mp3/0002.mp3
// sd:/mp3/0003.mp3

#include <SoftwareSerial.h>
#include <DFMiniMp3.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>


const char *ssid = "****";
const char *password = "****";


ESP8266WebServer server(80);

// implement a notification class,
// its member methods will get called 
//
class Mp3Notify
{
public:
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }

  static void OnPlayFinished(uint16_t globalTrack)
  {
    Serial.println();
    Serial.print(F("Play finished for #"));
    Serial.println(globalTrack);   
  }

  static void OnCardOnline(uint16_t code)
  {
    Serial.println();
    Serial.print(F("Card online "));
    Serial.println(code);     
  }

  static void OnCardInserted(uint16_t code)
  {
    Serial.println();
    Serial.print(F("Card inserted "));
    Serial.println(code); 
  }

  static void OnCardRemoved(uint16_t code)
  {
    Serial.println();
    Serial.print(F("Card removed "));
    Serial.println(code);  
  }
};

// instance a DFMiniMp3 object, 
// defined with the above notification class and the hardware serial class
//
//DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial1);

// Some arduino boards only have one hardware serial port, so a software serial port is needed instead.
// comment out the above definition and uncomment these lines
SoftwareSerial secondarySerial(D1, D2); // RX, TX
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(secondarySerial);

#define BUSY_PIN D5

void setup() 
{
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  Serial.begin(115200);
  digitalWrite(D4, LOW);
  delay(150);
  digitalWrite(D4, HIGH);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WIFI ..."));
  digitalWrite(D4, LOW);
  delay(150);
  digitalWrite(D4, HIGH);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    digitalWrite(D4, LOW);
    delay(100);
    digitalWrite(D4, HIGH);
    Serial.print(".");
  }
  Serial.println(F(" connection successful!"));

  
  server.on("/", handleRoot);
  server.on("/VolUp", handleVolUp);
  server.on("/VolDown", handleVolDown);
  server.on("/Play", handlePlay);
  server.on("/Stop", handleStop);
  server.on("/CheckPlaying", handleCheckPlaying);
  server.on("/Pause", handlePause);
  server.on("/PrevTrack", handlePrevTrack);
  server.on("/NextTrack", handleNextTrack);
  
  //server.on("/inline", []() {
  //  server.send(200, "text/plain", "this works as well");
  //});
  
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  pinMode(BUSY_PIN, INPUT);
  Serial.println(F("Initializing MP3 module..."));
  
  mp3.begin();

  // show OK
  digitalWrite(D4, LOW);
  delay(500);
  digitalWrite(D4, HIGH);
  delay(100);
  digitalWrite(D4, LOW);
  delay(500);
  digitalWrite(D4, HIGH);
  delay(10);
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    mp3.loop(); 
    delay(1);
  }
}

void HTTP(void)
{
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);
}

void handlePlay()
{
  HTTP();
  mp3.playMp3FolderTrack(1);
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}

void handlePrevTrack()
{
  HTTP();
  mp3.prevTrack();
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}

void handleNextTrack()
{
  HTTP();
  mp3.nextTrack();
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}

void handleStop()
{
  HTTP();
  mp3.stop();
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}

void handlePause()
{
  HTTP();
  mp3.pause();
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}


void handleVolUp()
{
  HTTP();
  uint16_t volume = mp3.getVolume();
  volume += 5;
  if (volume > 40)
  {
    volume = 40;
  }
  Serial.print("volume ");
  Serial.println(volume);
  mp3.setVolume(volume);
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}

void handleVolDown()
{
  HTTP();
  uint16_t volume = mp3.getVolume();
  volume -= 5;
  if (volume < 5)
  {
    volume = 0;
  }
  Serial.print("volume ");
  Serial.println(volume);
  mp3.setVolume(volume);
  delay(100);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  delay(1);
}

void handleRoot() 
{
  String s = "<html>\
              <head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\
              <link rel=\"icon\" href=\"data:,\">\
              <style>html {font-family: Helvetica;display:inline-block;margin:0px auto;text-align:center;}\
              .btn {background-color:#195B6A;color:white;padding:16px 40px;\
              text-decoration:none;font-size:30px;margin:2px;cursor:pointer;}\
              </style></head>\
              <body>\
                <p><h1>Control</h1></p>\
                <p><a href=\"/Play\"><button class=\"btn\">PLAY</button></a></p>\
                <p><a href=\"/Stop\"><button class=\"btn\">STOP</button></a></p>\
                <p><a href=\"/Pause\"><button class=\"btn\">PAUSE</button></a></p>\
                <p><a href=\"/PrevTrack\"><button class=\"btn\">PREV. TRACK</button></a></p>\
                <p><a href=\"/NextTrack\"><button class=\"btn\">NEXT TRACK</button></a></p>\
                <p><h1>Volume</h1></p>\
                <p><a href=\"/VolUp\"><button class=\"btn\">Volume UP</button></a></p>\
                <p><a href=\"/VolDown\"><button class=\"btn\">Volume DOWN</button></a></p>\
        <p><h1>Current state:</h1></p>";
  if (digitalRead(BUSY_PIN) == LOW)
  {     
    s += "<p>Playing music ... enjoy!</p>";
  } else
  {
    s += "<p>Not playing ... something is wrong?</p>";
  }
  s += "<p><h1>Number of tracks:</h1><p>";
  uint16_t count = mp3.getTotalTrackCount();
  s += String(count);
  s += "</body></html>";
  char CharBuffer[s.length() + 1];
  s.toCharArray(CharBuffer, s.length());          
  
  server.send(200, "text/html", CharBuffer);
}

void handleCheckPlaying() 
{
  HTTP();
  char temp[500];
  if (digitalRead(BUSY_PIN) == HIGH)
  {
    sprintf(temp, 
           "<html>\
              <head>\
                <meta http-equiv='refresh' content='5'/>\
                <title>ESP8266 Demo</title>\
                <style>\
                  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                </style>\
              </head>\
              <body>\
                <h1>Hello from ESP8266!</h1>\
                <h2>MP3 Player is NOT playing!</h2>\
              </body>\
            </html>");
  } else
  {
    sprintf(temp, 
           "<html>\
              <head>\
                <meta http-equiv='refresh' content='5'/>\
                <title>ESP8266 Demo</title>\
                <style>\
                  body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
                </style>\
              </head>\
              <body>\
                <h1>Hello from ESP8266!</h1>\
                <h2>MP3 Player is playing!</h2>\
              </body>\
            </html>");
  }
  
  server.send(200, "text/html", temp);
}

void handleNotFound() 
{
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);
  delay(50);
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);
  delay(50);
  digitalWrite(D4, LOW);
  delay(50);
  digitalWrite(D4, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void loop() 
{
  mp3.loop(); 
  delay(1);
  server.handleClient();
  delay(1);
}
