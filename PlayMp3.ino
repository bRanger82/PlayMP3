// it expects the sd card to contain these three mp3 files but doesn't care whats in them
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
class Mp3Notify
{
public:
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print(F("Com Error "));
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

SoftwareSerial swSerial(D1, D2); // RX, TX
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(swSerial);

#define BUSY_PIN   D5  // BUSY pin of the DF MP3 player to WEMOS
#define BLTIN_LED  D4  // WEMOS built-in LED

void setup() 
{
  Serial.begin(115200);

  pinMode(BUSY_PIN, INPUT);
  
  pinMode(BLTIN_LED, OUTPUT);
  digitalWrite(BLTIN_LED, HIGH);

  Serial.println(F("Initializing MP3 module..."));
  mp3.begin();
  Serial.println(F("done."));
  
  SetupWLAN();
}

void SetupWLAN(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WIFI ..."));

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    digitalWrite(BLTIN_LED, LOW);
    waitMilliseconds(50);
    digitalWrite(BLTIN_LED, HIGH);
    Serial.print(F("."));
    waitMilliseconds(50);
  }
  
  Serial.println(F(" connection successful!"));

  server.on("/", handleRoot);
  server.on("/VolUp", handleVolUp);
  server.on("/VolDown", handleVolDown);
  server.on("/Play", handlePlay);
  server.on("/Stop", handleStop);
  server.on("/Pause", handlePause);
  server.on("/PrevTrack", handlePrevTrack);
  server.on("/NextTrack", handleNextTrack);  
  server.on("/ChangeEq", handleSetEq);

  //server.on("/inline", []() {
  //  server.send(200, "text/plain", "this works as well");
  //});

  server.onNotFound(handleNotFound);
  server.begin();

  waitMilliseconds(10);
  
  Serial.println("");
  Serial.print(F("Connected to "));
  Serial.println(ssid);
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  // calling mp3.loop() periodically allows for notifications 
  // to be handled without interrupts
  while ((millis() - start) < msWait)
  {
    mp3.loop(); 
    delay(1);
  }
}

void HTTPRcvLED(void)
{
  digitalWrite(BLTIN_LED, LOW);
  waitMilliseconds(10);
  digitalWrite(BLTIN_LED, HIGH);
}

void ShowOK(void)
{
  // show OK, two times 'slow' blinking
  digitalWrite(BLTIN_LED, LOW);
  waitMilliseconds(500);
  digitalWrite(BLTIN_LED, HIGH);
  waitMilliseconds(100);
  digitalWrite(BLTIN_LED, LOW);
  waitMilliseconds(500);
  digitalWrite(BLTIN_LED, HIGH);
  waitMilliseconds(10);
}

void ShowError(void)
{
  // show Error, fast blinking 3 times
  digitalWrite(BLTIN_LED, LOW);
  waitMilliseconds(50);
  digitalWrite(BLTIN_LED, HIGH);
  waitMilliseconds(50);
  digitalWrite(BLTIN_LED, LOW);
  waitMilliseconds(50);
  digitalWrite(BLTIN_LED, HIGH);
  waitMilliseconds(50);  
  digitalWrite(BLTIN_LED, LOW);
  waitMilliseconds(50);
  digitalWrite(BLTIN_LED, HIGH);
}

void redirectHTTP()
{
  HTTPRcvLED();
  waitMilliseconds(50);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
  waitMilliseconds(1);  
}

void handlePlay()
{
  mp3.playMp3FolderTrack(1);
  redirectHTTP();
}

void handlePrevTrack()
{
  mp3.prevTrack();
  redirectHTTP();
}

void handleNextTrack()
{
  mp3.nextTrack();
  redirectHTTP();
}

void handleStop()
{
  mp3.stop();
  redirectHTTP();
}

void handlePause()
{
  mp3.pause();
  redirectHTTP();
}


void handleVolUp()
{
  mp3.increaseVolume();
  redirectHTTP();
}

void handleVolDown()
{
  mp3.decreaseVolume();
  redirectHTTP();
}

volatile byte EQ_IDX = 1;

void handleSetEq(void)
{
  DfMp3_Eq eq = DfMp3_Eq_Normal;
  
  EQ_IDX++;
  
  if (EQ_IDX > 6)
  {
    EQ_IDX = 1;
  }
  
  switch(EQ_IDX)
  {
    case 1:  eq = DfMp3_Eq_Normal;  break;
    case 2:  eq = DfMp3_Eq_Pop;     break;
    case 3:  eq = DfMp3_Eq_Rock;    break;
    case 4:  eq = DfMp3_Eq_Jazz;    break;
    case 5:  eq = DfMp3_Eq_Classic; break;
    case 6:  eq = DfMp3_Eq_Bass;    break;
    default: Serial.println(F("EQ_IDX is not defined!")); break;
  }

  mp3.setEq(eq);
  redirectHTTP();
}

void handleRoot() 
{
  HTTPRcvLED();

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
                <p><a href=\"/ChangeEq\"><button class=\"btn\">CHANGE EQ</button></a></p>\
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
  /*
  s += "<p><h1>EQ used:</h1><p>";
  DfMp3_Eq eq = mp3.getEq();

  
  switch(eq)
  {
    case DfMp3_Eq_Normal:  s += "Normal";  break;
    case DfMp3_Eq_Pop:     s += "Pop";     break;
    case DfMp3_Eq_Rock:    s += "Rock";    break;
    case DfMp3_Eq_Jazz:    s += "Jazz";    break;
    case DfMp3_Eq_Classic: s += "Classic"; break;
    case DfMp3_Eq_Bass:    s += "Bass";    break;
    default: Serial.println(F("EQ_IDX is not defined!")); break;
  }
  */
  s += "</body></html>";
  char CharBuffer[s.length() + 1];
  s.toCharArray(CharBuffer, s.length());          
  
  server.send(200, "text/html", CharBuffer);
}

void handleNotFound() 
{
  HTTPRcvLED();
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) 
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void loop() 
{
  mp3.loop(); 
  waitMilliseconds(1);
  server.handleClient();
  waitMilliseconds(1);
}
