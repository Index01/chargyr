
#include "EEPROM.h"
#include <Adafruit_NeoPixel.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Redis.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "otaUpdater.h"
#include "index_html.h"
#include "redisCreds.h"

#define EEPROM_SIZE 128
#define LED_BUILTIN 4
#define SENS 12
#define WS2812 13
#define WSQTY 8


AsyncWebServer server(80);
const char *ssid = "ChargeMeUp";

const char *PARAM_SSID = "inSsid";
const char *PARAM_PASS = "inPass";
const char *PARAM_UNAME = "inUname";
const char *PARAM_LED1 = "inLed1";
const char *PARAM_LED2 = "inLed2";
const char *PARAM_LED3 = "inLed3";
const char *PARAM_SLIDE_AMP = "in_slideAmp";
const char *PARAM_SLIDE_OFFSET = "in_slideOffset";
const char *PARAM_SLIDE_PERIOD = "in_slidePeriod";
const char *PARAM_SLIDE_FREQ_RED = "in_slideFreqRed";
const char *PARAM_SLIDE_FREQ_GREEN = "in_slideFreqGreen";
const char *PARAM_SLIDE_FREQ_BLUE = "in_slideFreqBlue";

enum MemAddrs {
  startSsid = 0,
  startPass = 16,
  startUname = 32,
  startLed1 = 48,
  startLed2 = 64,
  startLed3 = 80,
  startAmp = 96,
  startOffset = 98,
  startFreqRed = 100,
  startFreqGreen = 102,
  startFreqBlue = 104
};

typedef struct FlashIndex {
  int start;
  int size;
};

typedef struct lightState {
  int interval;
  int period;
  int prev_val;
  int last_up;
  int amplitude;
  int offset;
} rgbState;

rgbState rgbArr[3] = {
    {.interval = 30,
     .period = 200,
     .prev_val = 0,
     .last_up = 0,
     .amplitude = 32,
     .offset = 32},
    {.interval = 20,
     .period = 800,
     .prev_val = 0,
     .last_up = 0,
     .amplitude = 32,
     .offset = 32},
    {.interval = 10,
     .period = 60,
     .prev_val = 0,
     .last_up = 0,
     .amplitude = 12,
     .offset = 12},
};

Adafruit_NeoPixel pixels =
    Adafruit_NeoPixel(WSQTY, WS2812, NEO_GRB + NEO_KHZ800);

int timer_now = 0;
int timer_lights = 0;
int stateArr[3] = {1, 1, 1};
unsigned int count_main = 0;
int lSinResp[3] = {0, 0, 0};
String sliderAmp = "";

FlashIndex inSsidFlash {
  .start = startSsid,
  .size = 16
};
FlashIndex inPassFlash {
  .start = startPass,
  .size = 16
};
FlashIndex inUnameFlash {
  .start = startUname,
  .size = 16
};
FlashIndex inLed1Flash {
  .start = startLed1,
  .size = 16
};
FlashIndex inLed2Flash {
  .start = startLed2,
  .size = 16
};
FlashIndex inLed3Flash {
  .start = startLed3,
  .size = 16
};
FlashIndex inAmpFlash {
  .start = startAmp,
  .size = 2
};
FlashIndex inOffFlash {
  .start = startOffset,
  .size = 2
};
FlashIndex inFreqRedFlash {
  .start = startFreqRed,
  .size = 2
};
FlashIndex inFreqGreenFlash {
  .start = startFreqGreen,
  .size = 2
};
FlashIndex inFreqBlueFlash {
  .start = startFreqBlue,
  .size = 2
};


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}
void writeIntIntoEEPROM(int address, int number)
{ 
  byte byte1 = number >> 8;
  byte byte2 = number & 0xFF;
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);

  EEPROM.commit();
  Serial.print("Int read back: ");
  Serial.println(readIntFromEEPROM(address));
}

//const char *readMem(int *addrStart) {
const char *readMem(int addrStart, int size) {
  // Serial.println("[+] Reading mem.");
  String ret = "";
  //for (int i = *addrStart; i < *addrStart + 16; i++) {
  //Serial.println(addrStart);
  //Serial.println(size);
  for (int i = addrStart; i <= addrStart + size; i++) {

    byte readVal = EEPROM.read(i);
    ret += char(readVal);

    //Serial.println(ret);
    //if (ret == 0x0) {
    //  break;
    //}
  }
  //Serial.println(ret);
  return ret.c_str();
}

//void writeMem(int addr, const char *val) {
void writeMem(int addr, int size, const char *val) {
  Serial.print ("writemem: ");
  Serial.println(val);
  int addr_og = addr;
  if (strlen(val) > 1) {
    //for (int i = 0; i < 16; i++) {
    for (int i = 0; i < size; i++) {
      Serial.println(addr);
      Serial.println(val[i]);
      EEPROM.write(addr, val[i]);
      addr += 1;
    }
    EEPROM.commit();
  } else {
    Serial.println("[+] Decline empty eeprom fields");
  }

  Serial.print ("Read it back: ");
  Serial.print (readMem(addr_og, size));
  
}

void redisPost(Redis *redis, const char *key, char *state) {
  String euname = "";
  for (int i = startUname; i < startUname + 16; i++) {
    byte readVal = EEPROM.read(i);
    if (readVal == '00000000') {
      break;
    }
    euname += char(readVal);
  }

  if (redis->set(euname.c_str(), state)) {
    Serial.println("[+] State published.");
    redis->pexpire(euname.c_str(), 30000); // set key expire in 30000ms
  }
}

bool redisKeyStateHigh(Redis *redis, FlashIndex *key) {
  int keyVal = redis->get(readMem(key->start, key->size)).toInt();
  if (keyVal == 0) {
    Serial.println("[+] LED is off .");
    return false;
  } else {
    Serial.println("[+] LED is on!");
  }
  return true;
}

void APMode() {
  Serial.println("[+] Entering APMode");
  for (int i = 0; i < 3; i++) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(20);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
    }
    delay(2000);
  }

  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  server.onNotFound(notFound);
  server.begin();

  if(myIP && MDNS.begin("chargy")) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("http", "tcp", 53);
    Serial.println("[+] DNS begin success");
  }
  else{
    Serial.println("[-] DNS Failed!");
  }
}

void ClientMode(){

    WiFi.mode(WIFI_STA);

    String essid = "";
    for (int i = startSsid; i < startSsid + 16; i++) {
      essid += char(EEPROM.read(i));
      if (essid[i] == '\x00') {
        break;
      }
    }

    String epass = "";
    for (int i = startPass; i < startPass + 16; i++) {
      byte readVal = EEPROM.read(i);
      if (readVal == 0) {
        break;
      }
      epass += char(readVal);
    }

    String eled2 = "";
    for (int i = startLed2; i < startLed2 + 16; i++) {
      byte readVal = EEPROM.read(i);
      if (readVal == '00000000') {
        break;
      }
      eled2 += char(readVal);
    }

    Serial.println(eled2);
    Serial.println(essid);
    Serial.println(epass);
    WiFi.begin(essid.c_str(), epass.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      APMode();
    } else {
      Serial.printf("WiFi Success!\n");
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
      }
    }

    Serial.println("User settings:");
    rgbArr[0].period = readIntFromEEPROM(startFreqRed); 
    rgbArr[1].period = readIntFromEEPROM(startFreqGreen); 
    rgbArr[2].period = readIntFromEEPROM(startFreqBlue); 

    for(int i=0; i<3; i++){
      rgbArr[i].amplitude = readIntFromEEPROM(startAmp); 
      rgbArr[i].offset = readIntFromEEPROM(startOffset); 
    }

    server.onNotFound(notFound);
    server.begin();

    if(WiFi.localIP() && MDNS.begin("chargy")) {
      MDNS.addService("_http", "tcp", 80);
      Serial.println("[*] DNS begin success");
    }
    Serial.println("Server started");
    Serial.println(WiFi.localIP());
}

void lightSweep(Adafruit_NeoPixel *pixels, int position) {
  int posArr[3] = {0, 0, 0};
  posArr[position] = 1;

  for (int amp = 0; amp < 128; amp++) {
    for (int pix = 0; pix < WSQTY; pix++) {
      pixels->setPixelColor(pix, posArr[0] * amp, posArr[1] * amp,
                            posArr[2] * amp);
      pixels->show();
      delay(1);
    }
  }
  for (int amp = 128; amp >= 0; amp--) {
    for (int pix = 0; pix < WSQTY; pix++) {
      pixels->setPixelColor(pix, posArr[0] * amp, posArr[1] * amp,
                            posArr[2] * amp);
      pixels->show();
      delay(1);
    }
  }
}

int lightSine(rgbState read_rgbArr) {
  /* sine: y= (offset + amplitude) * cos(2*pi/period*tao)+phi */

  int phi = 90;
  float ang_freq = 2 * 3.14159 / read_rgbArr.period;

  int dt = count_main + read_rgbArr.interval;
  int val = read_rgbArr.offset +
            //read_rgbArr.amplitude * cos((ang_freq * millis()) + phi);
            read_rgbArr.amplitude * cos((ang_freq *dt) + phi);
  /* Bit of smoothing */          
  if (val == 0 && read_rgbArr.prev_val > 10) {
    val = read_rgbArr.prev_val;
  }
  if(val < 4){
    val = 0;
  }
  if(val < 13){
    val = val / 2;
  }
  int valDelta = abs(val - read_rgbArr.prev_val) / 2;

  if(read_rgbArr.prev_val > val){
    val = val + valDelta;
  }else{
    val = val - valDelta;
  }
  read_rgbArr.prev_val = val;
  read_rgbArr.last_up = count_main;

  count_main ++;
  return val;
}


void lightSteppa(int stateArr[3], Adafruit_NeoPixel *pixels) {
  float delta = 1 / 44100;
  int r = 0;
  int g = 0;
  int b = 0;
  
  for (int i = 0; i < 3; i++) {
      lSinResp[i] = lightSine(rgbArr[i]);
  }
  //Serial.println(lSinResp[2]);

  for (int pix = 0; pix < WSQTY; pix++) {
    pixels->setPixelColor(pix, stateArr[0] * lSinResp[0],
                              stateArr[1] * lSinResp[1],
                              stateArr[2] * lSinResp[2]);
    pixels->show();

    int del_log10[15] = {10, 12, 14, 16, 19, 23, 27, 31,
                        37, 44, 52, 61, 72, 84, 100};

    delay(del_log10[pix]);
    //delay(2);
 }
}

String processor(const String& var){
  //Note: this thing is whack
  //Serial.println(var);
  if (var == "SLIDEAMP"){
    return sliderAmp;
  }
  return String();
}
 
void serverSetup() {
  // Setup the OTA stuff. no touchy.
  serverOtaRoutes(&server);

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[+] / hit");
    request->send_P(200, "text/html", index_html);
  });

  server.on("/slider", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[+] /slider hit");

    if (request->hasParam(PARAM_SLIDE_AMP)) {
      sliderAmp = request->getParam(PARAM_SLIDE_AMP)->value().c_str();
      for(int i=0; i<3; i++){
          rgbArr[i].amplitude = sliderAmp.toInt();
          Serial.println(rgbArr[i].amplitude);
      }
      writeIntIntoEEPROM(startAmp, sliderAmp.toInt());
    } 
    else if (request->hasParam(PARAM_SLIDE_OFFSET)) {
      String sliderOff = request->getParam(PARAM_SLIDE_OFFSET)->value().c_str();
      for(int i=0; i<3; i++){
          rgbArr[i].offset = sliderOff.toInt();
          Serial.println(rgbArr[i].offset);
      }
      writeIntIntoEEPROM(startOffset, sliderOff.toInt());
    } 
    else if (request->hasParam(PARAM_SLIDE_FREQ_RED)) {
      String sliderFreqRed = request->getParam(PARAM_SLIDE_FREQ_RED)->value().c_str();
      rgbArr[0].period = sliderFreqRed.toInt();
      writeIntIntoEEPROM(startFreqRed, sliderFreqRed.toInt());
      Serial.println(rgbArr[0].period);
    } 
    else if (request->hasParam(PARAM_SLIDE_FREQ_GREEN)) {
      String sliderFreqGreen = request->getParam(PARAM_SLIDE_FREQ_GREEN)->value().c_str();
      rgbArr[1].period = sliderFreqGreen.toInt();
      Serial.println(rgbArr[1].period);
      writeIntIntoEEPROM(startFreqGreen, sliderFreqGreen.toInt());
    } 
    else if (request->hasParam(PARAM_SLIDE_FREQ_BLUE)) {
      String sliderFreqBlue = request->getParam(PARAM_SLIDE_FREQ_BLUE)->value().c_str();
      rgbArr[2].period = sliderFreqBlue.toInt();
      Serial.println(rgbArr[2].period);
      writeIntIntoEEPROM(startFreqBlue, sliderFreqBlue.toInt());
    } 
    else{
      Serial.println("[-] Nothin on the slide");
    }


    request->send(200);
  });

  server.on("/data-red", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[+] /data-red hit");
    String redPoints = "";
    for(int i=0; i<60; i++){
       int redResp = lightSine(rgbArr[0]);
       rgbArr[0].prev_val = redResp;
       redPoints += redResp;
       redPoints += ", ";

    }
    request->send_P(200, "text/html", redPoints.c_str());
    count_main -= 60;

  });
  server.on("/data-green", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[+] /data-green hit");
    String greenPoints = "";
    for(int i=0; i<60; i++){
       int greenResp = lightSine(rgbArr[1]);
       rgbArr[1].prev_val = greenResp;
       greenPoints += greenResp;
       greenPoints += ", ";
    }
    request->send_P(200, "text/html", greenPoints.c_str());
    count_main -= 60;

  });
  server.on("/data-blue", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[+] /data-blue hit");
    String bluePoints = "";
    for(int i=0; i<60; i++){
       int blueResp = lightSine(rgbArr[2]);
       rgbArr[2].prev_val = blueResp;
       bluePoints += blueResp;
       bluePoints += ", ";
    }
    request->send_P(200, "text/html", bluePoints.c_str());
    count_main -= 60;

  });

  // Send a GET request to <ESP_IP>/get?inSsid=<inputMessage>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[+] /get hit");
    String inputMessage;
    const char *inputSsid;
    const char *inputPassword;
    const char *inputUname;
    const char *inputLed1;
    const char *inputLed2;
    const char *inputLed3;
    inputSsid = request->getParam(PARAM_SSID)->value().c_str();
    inputPassword = request->getParam(PARAM_PASS)->value().c_str();
    inputUname = request->getParam(PARAM_UNAME)->value().c_str();
    inputLed1 = request->getParam(PARAM_LED1)->value().c_str();
    inputLed2 = request->getParam(PARAM_LED2)->value().c_str();
    inputLed3 = request->getParam(PARAM_LED3)->value().c_str();
    Serial.println(inputSsid);
    Serial.println(inputPassword);
    Serial.println(inputUname);
    Serial.println(inputLed1);
    Serial.println(inputLed2);
    Serial.println(inputLed3);

    writeMem(inSsidFlash.start, inSsidFlash.size, inputSsid);
    writeMem(inPassFlash.start, inPassFlash.size, inputPassword);
    writeMem(inUnameFlash.start, inUnameFlash.size, inputUname);
    writeMem(inLed1Flash.start, inLed1Flash.size, inputLed1);
    writeMem(inLed2Flash.start, inLed2Flash.size, inputLed2);
    writeMem(inLed3Flash.start, inLed3Flash.size, inputLed3);

    String ss(inputSsid);
    String un(inputUname);
    String pp(inputPassword);
    String l1(inputLed1);
    String l2(inputLed2);
    String l3(inputLed3);

    if (strlen(inputUname) > 10 || strlen(inputLed1) > 10 ||
        strlen(inputLed2) > 10 || strlen(inputLed3) > 10) {
      request->send(200, "text/html",
                    "FAILURE! <br>No fields greater than 10 characters allowed."
                    "Risc architecture is going to change everything, brah.<br>"
                    "<br><a href=\"/\"><br>Return to Home Page</a>");
    }

    if (strlen(inputSsid) > 2 || strlen(inputPassword) > 2 ||
        strlen(inputUname) > 2) {
      request->send(200, "text/html",
                    "SUCCESS! <br>saved WiFi network name:" + ss +
                        " <br>with device name: " + un +
                        "setTimeout(function(){window.location='/';}, 2000);"
                        "document.write(<br>You will be redirected shotly.');"     // Todo: fixme
                        "<br><a href=\"/\"><br>Return to Home Page</a>");

      ESP.restart();
    } else {
      request->send(200, "text/html",
                    "SUCCESS! <br>Updated the following fields:" + ss + "<br>" +
                        pp + "<br>" + un + "<br>" + l1 + "<br>" + l2 + "<br>" + l3 + 
                        "setTimeout(function(){window.location='/';}, 2000);"
                        "document.write(<br>You will be redirected shotly.');"
                        "<br><a href=\"/\"><br>Return to Home Page</a>");
    }
  });
}

void setup() {

  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  pixels.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENS, INPUT);

  serverSetup();


  if (sizeof(readMem(inSsidFlash.start, inSsidFlash.size)) > 2) {
    Serial.println("[+] Attempting to join ssid... ");
      ClientMode();

    } else {
      Serial.println("[+] Device unconfigured. Booting in APMode");
      APMode();
    }
}


void loop() {
  char val[2];
  if (count_main > 65500) {
    count_main = 0;
    rgbArr[0].last_up = 0;
    rgbArr[1].last_up = 0;
    rgbArr[3].last_up = 0;
  } 

  if (millis() > timer_now + 9000) {
    timer_now = millis();

    WiFiClient redisConn;
    if (!redisConn.connect(REDIS_ADDR, REDIS_PORT)) {
      Serial.println("Failed to connect to the Redis server!");
    }

    Redis redis(redisConn);
    auto connRet = redis.authenticate(REDIS_PASSWORD);
    if (!connRet == RedisSuccess) {
      Serial.printf("Failed to authenticate to the Redis server! Errno: %d\n",
                    (int)connRet);
    }

    /* Read the local state, post remote*/
    if (digitalRead(SENS) == HIGH) {
      sprintf(val, "%d", 1);
      redisPost(&redis, readMem(inUnameFlash.start, inUnameFlash.size), val);
    } else {
      sprintf(val, "%d", 0);
      redisPost(&redis, readMem(inUnameFlash.start, inUnameFlash.size), val);
    }

    lightSteppa(stateArr, &pixels);  // redis takes a while. get in an extra update
    /* Read the remote states*/
    stateArr[0] = redisKeyStateHigh(&redis, &inLed1Flash),
    stateArr[1] = redisKeyStateHigh(&redis, &inLed2Flash),
    stateArr[2] = redisKeyStateHigh(&redis, &inLed3Flash),

    Serial.println("------ in loop complete -------");

    redisConn.stop();
  }

  /* The easy way */
  // for(int pos=0; pos < sizeof stateArr; pos++){
  //  if(stateArr[pos] == 1){
  //    lightSweep(&pixels, pos);
  //  }
  //}

  /* The other way */
  lightSteppa(stateArr, &pixels);

  // Serial.println("------ outer loop complete -------");
}
