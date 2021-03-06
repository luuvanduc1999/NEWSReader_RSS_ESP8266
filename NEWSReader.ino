#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "U8g2_for_Adafruit_GFX.h"
#include <WiFiUdp.h>
char ssid[] = "Ngoc Anh";   // your network SSID (name)
char pass[] = "tuchonmadung";   // your network password
//char ssid[] = "FPT-Unknown";   // your network SSID (name)
//char pass[] = "hustbephot";   // your network password

int idx = 0;
int preIdx = -1;
WiFiClient  client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 25200);
String preTime("");
String preDate("");
String    preStatus("");
String    preTemp("");
String    preHumi("");

#define TFT_CS    D2     // TFT CS  pin is connected to NodeMCU pin D2
#define TFT_RST   D3     // TFT RST pin is connected to NodeMCU pin D3
#define TFT_DC    D4     // TFT DC  pin is connected to NodeMCU pin D4
#define SWITCH    D6     // TFT DC  pin is connected to NodeMCU pin D4
// initialize ILI9341 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char TINHOT[25][9] = {"19180833", "19180832", "19180831", "19180830", "19180829", "19180828", "19180827", "19180826", "19180825", "19180824", "19180823", "19180822", "19180821", "19180820", "19180819", "19180818", "19180817", "19180815", "19180814", "19180813", "19180812", "19180811", "19180810", "19180809", "19180808"};
char T_TINHOT[25][9] = {"19180786", "19180785", "19180784", "19180783", "19180782", "19180781", "19180780", "19180779", "19180778", "19180777", "19180776", "19180775", "19180774", "19180773", "19180772", "19180771", "19180770", "19180769", "19180768", "19180767", "19180766", "19180765", "19180764", "19180763", "19180762"};

int iTitle = 0;
String tTitle[2] = {"", ""};
int iContent = 0;
String tContent[6] = {"", "", "", "", "", ""};

U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

unsigned long CmyTalkBackID = 38959;
const char * CmyTalkBackKey = "9DD7RLNE6Q5YZV90";

unsigned long TmyTalkBackID = 38964;
const char * TmyTalkBackKey = "JFIC48F3RU95H66U";

unsigned long WmyTalkBackID = 39112;
const char * WmyTalkBackKey = "YWLNNTR9Y3OLSQVC";

void setup() {
  display.begin();
  display.setRotation(1);
  display.fillScreen(ILI9341_BLACK);
  u8g2_for_adafruit_gfx.begin(display);
  u8g2_for_adafruit_gfx.setFontMode(0);                 // use u8g2 none transparent mode
  u8g2_for_adafruit_gfx.setFontDirection(0);            // left to right (this is default)

  Serial.begin(115200);          // Initialize serial


  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(String(ssid));
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }
  pinMode(SWITCH, INPUT);
}

void setClock() {
  timeClient.update();
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.println(timeClient.getMinutes());
  String timex(timeClient.getHours());
  timex.concat(":");
  if (timeClient.getMinutes() < 10) {
    timex.concat("0");
    timex.concat(timeClient.getMinutes());
  }
  else
    timex.concat(timeClient.getMinutes());

  display.fillRect(260, 0, 330, 20, ILI9341_BLACK);
  u8g2_for_adafruit_gfx.setCursor(260, 20);
  u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tn);
  u8g2_for_adafruit_gfx.print( timex);

}

// General function to POST to ThingSpeak
String httpGET(String uri, String postMessage) {

  bool connectSuccess = false;
  connectSuccess = client.connect("api.thingspeak.com", 80);

  if (!connectSuccess) {
    Serial.println("Khong ket noi");
  }

  postMessage += "&headers=false";

  String Headers =  String("GET ") + uri + String(" HTTP/1.1\r\n") +
                    String("Host: api.thingspeak.com\r\n") +
                    String("Content-Type: application/x-www-form-urlencoded\r\n") +
                    String("Connection: close\r\n") +
                    String("Content-Length: ") + String(postMessage.length()) +
                    String("\r\n\r\n");

  client.print(Headers);
  client.print(postMessage);

  long startWaitForResponseAt = millis();
  while (client.available() == 0 && millis() - startWaitForResponseAt < 5000) {
    delay(100);
  }

  if (client.available() == 0) {
    //return -304; // Didn't get server response in time
  }

  if (!client.find(const_cast<char *>("HTTP/1.1"))) {
    // return -303; // Couldn't parse response (didn't find HTTP/1.1)
  }

  int status = client.parseInt();
  if (status != 200) {
    //return status;
  }

  if (!client.find(const_cast<char *>("\n\r\n"))) {
    //return -303;
  }


  String tempString = String(client.readString());
  return tempString;

}

void jsonFT(String payload)
{
  const size_t bufferSize = JSON_OBJECT_SIZE(4)  + 370;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(payload);
  const char* rs = root["command_string"];

  const size_t bufferSizeArr = JSON_OBJECT_SIZE(2)  + 370;
  DynamicJsonBuffer jsonBufferArr(bufferSizeArr);
  JsonArray& array = jsonBufferArr.parseArray(rs);

  iTitle = array.size();
  const char* tmp1 = array[0];
  const char* tmp2 = array[1];
  tTitle[0] = tmp1;
  tTitle[1] = tmp2;

}

void jsonFC(String payload)
{
  const size_t bufferSize = JSON_OBJECT_SIZE(4)  + 370;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(payload);
  const char* rs = root["command_string"];
  const size_t bufferSizeArr = JSON_OBJECT_SIZE(5)  + 370;
  DynamicJsonBuffer jsonBufferArr(bufferSizeArr);
  JsonArray& array = jsonBufferArr.parseArray(rs);


  iContent = array.size();
  for (int i = 0; i < array.size(); i++)
  {
    const char* tmp = array[i];
    tContent[i] = String(tmp);
  }

}

void getNewsData(int idx)
{
  String titleURI = String("/talkbacks/") + String(TmyTalkBackID) + String("/commands/") + String(T_TINHOT[idx]) + String(".json");
  String postMessage =  String("api_key=") + String(TmyTalkBackKey);
  String x = httpGET(titleURI, postMessage);
  jsonFT(x.substring(x.indexOf('{'), x.indexOf('}') + 1));


  String tbURI = String("/talkbacks/") + String(CmyTalkBackID) +  String("/commands/") + String(TINHOT[idx]) + String(".json");
  String tbpostMessage =  String("api_key=") + String(CmyTalkBackKey);
  String xx = httpGET(tbURI, tbpostMessage);
  jsonFC(xx.substring(xx.indexOf('{'), xx.indexOf('}') + 1));

}


void ShowOnScreen()
{
  display.fillRect(280, 214, 330, 330, ILI9341_BLACK);
  display.fillRect(2, 30, 330, 173, ILI9341_BLACK);

  u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_GREEN);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_arrow_2x_t  );
  u8g2_for_adafruit_gfx.drawGlyph( 2, 47, 78);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_unifont_t_vietnamese2);
  u8g2_for_adafruit_gfx.setCursor(15, 45);
  u8g2_for_adafruit_gfx.print(tTitle[0]); //max=38
  u8g2_for_adafruit_gfx.setCursor(5, 65);
  u8g2_for_adafruit_gfx.print(tTitle[1]); //max=38

  u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_unifont_t_vietnamese2);
  int line = 90;
  for (int i = 0; i < iContent; i++) {
    u8g2_for_adafruit_gfx.setCursor(8, line);
    u8g2_for_adafruit_gfx.print(tContent[i]);
    line += 20;
  }
}

void getMostViewed( )
{
  //Init News
  if (preIdx == -1) {
    display.fillScreen(ILI9341_BLACK);
    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_RED);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_www_2x_t);
    u8g2_for_adafruit_gfx.drawGlyph( 5, 234, 77);
    u8g2_for_adafruit_gfx.setCursor(280, 230);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    u8g2_for_adafruit_gfx.setCursor(25, 232);
    u8g2_for_adafruit_gfx.print("VNEXPRESS.NET"); //max=38
    u8g2_for_adafruit_gfx.setCursor(2, 20);
    u8g2_for_adafruit_gfx.print("GROUP 26"); //max=38
    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_www_2x_t);
    u8g2_for_adafruit_gfx.drawGlyph( 150, 130, 76);
    if (idx == 0)
      getNewsData(0);
  }
  setClock();
  ShowOnScreen();


  String index(String((idx % 25) + 1) + String("/25"));
  u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
  u8g2_for_adafruit_gfx.setCursor(280, 230);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_unifont_t_vietnamese2);
  u8g2_for_adafruit_gfx.print(index);
  preIdx = idx;
  idx++;
  delay(3000);

  //load next news
  int rdxc = digitalRead(SWITCH);
  if (((rdxc == 1) && (idx >= 0)) || ((rdxc == 0) && (idx < 0))) return;
  u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_YELLOW);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_www_2x_t);
  u8g2_for_adafruit_gfx.drawGlyph( 230, 21, 74);
  unsigned long timeg;
  timeg = millis();
  getNewsData(idx % 25);
  display.fillRect(230, 0, 21, 21, ILI9341_BLACK);
  int continued = int(15000 - (millis() - timeg));
  while (continued > 0)
  {
    if (((rdxc == 1) && (idx >= 0)) || ((rdxc == 0) && (idx < 0))) break;
    delay(1000);
    continued -= 1000;
  }

}


void showWeather()
{
  String WURI = String("/talkbacks/") + String(WmyTalkBackID) + String("/commands/") + String("18906639") + String(".json");
  String postMessage =  String("api_key=") + String(WmyTalkBackKey);
  String payload = httpGET(WURI, postMessage);
  payload = payload.substring(payload.indexOf('{'), payload.indexOf('}') + 1);

  const size_t bufferSize = JSON_OBJECT_SIZE(4)  + 370;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(payload);
  const char* rs = root["command_string"];
  const size_t bufferSizeArr = JSON_OBJECT_SIZE(5)  + 370;
  DynamicJsonBuffer jsonBufferArr(bufferSizeArr);
  JsonArray& array = jsonBufferArr.parseArray(rs);

  const char* wStatus = array[0];
  const char* wTemp = array[1];
  const char* wHumi = array[2];

  if (String(wStatus) != preStatus)
  {
    display.fillRect(0, 165, 340, 22, ILI9341_BLACK);

    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_ORANGE);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_www_2x_t);
    u8g2_for_adafruit_gfx.drawGlyph( 2, 180, 80);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    u8g2_for_adafruit_gfx.setCursor(25, 180);
    u8g2_for_adafruit_gfx.print(wStatus);
    //10+20+8*14
    u8g2_for_adafruit_gfx.print( " ");
    for (int i = 0; i < 30; i++)
      u8g2_for_adafruit_gfx.print( "_");
    preStatus = wStatus;
  }

  if (String(wTemp) != preTemp)
  {
    display.fillRect(60, 212, 120, 30, ILI9341_BLACK);
    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    u8g2_for_adafruit_gfx.setCursor(97, 230);
    u8g2_for_adafruit_gfx.print( wTemp);
    preTemp = wTemp;
  }

  if (String(wHumi) != preHumi)
  {
    display.fillRect(200, 212, 120, 30, ILI9341_BLACK);
    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    u8g2_for_adafruit_gfx.setCursor(215, 230);
    u8g2_for_adafruit_gfx.print(wHumi);
    preHumi = wHumi;
  }



}

void showClock( )
{
  //Init_Clock
  if (idx == -1) {
    display.fillScreen(ILI9341_BLACK);
    preDate = "";
    preTime = "";
    preStatus = "";
    preTemp = "";
    preHumi = "";
    u8g2_for_adafruit_gfx.setFontMode(0);                 // use u8g2 none transparent mode
    u8g2_for_adafruit_gfx.setFontDirection(0);            // left to right (this is default)
    display.drawRect(0, 0, 320, 45, ILI9341_WHITE);
    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_RED);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR18_tf   );
    u8g2_for_adafruit_gfx.setCursor(50, 31);
    u8g2_for_adafruit_gfx.print( "GROUP 26 #KTVXL");
    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_ORANGE);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_open_iconic_www_2x_t);
    u8g2_for_adafruit_gfx.drawGlyph( 2, 180, 80);

    u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_WHITE);
    u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf);
    u8g2_for_adafruit_gfx.setCursor(60, 210);
    u8g2_for_adafruit_gfx.print( "Temperature");


    u8g2_for_adafruit_gfx.setCursor(200, 210);
    u8g2_for_adafruit_gfx.print( "Humidity");
    showWeather();

  }


  timeClient.update();
  String timex("");
  if (timeClient.getHours() < 10) {
    timex.concat("0");
    timex.concat(timeClient.getHours());
  }
  else
    timex.concat(timeClient.getHours());
  timex.concat(":");
  if (timeClient.getMinutes() < 10) {
    timex.concat("0");
    timex.concat(timeClient.getMinutes());
  }
  else
    timex.concat(timeClient.getMinutes());
  u8g2_for_adafruit_gfx.setForegroundColor(ILI9341_CYAN);
  u8g2_for_adafruit_gfx.setCursor(118, 107);
  u8g2_for_adafruit_gfx.setFont(u8g2_font_helvB24_tf   );
  if (preTime != timex) {
    display.fillRect(0, 50, 340, 58, ILI9341_BLACK);
    u8g2_for_adafruit_gfx.print(timex);
  }
  preTime = timex;

  String datex("");
  datex.concat(daysOfTheWeek[timeClient.getDay()]);
  datex.concat(", ");
  datex.concat(timeClient.getFormattedDate());
  u8g2_for_adafruit_gfx.setFont(u8g2_font_helvR14_tf   );
  u8g2_for_adafruit_gfx.setCursor(70, 135);
  if (preDate != datex) {
    display.fillRect(0, 108, 340, 50, ILI9341_BLACK);
    u8g2_for_adafruit_gfx.print(datex);
  }
  preDate = datex;

  if (abs(idx)%10==0) showWeather();

  Serial.println(timeClient.getFormattedDate());
  idx--;

  delay(1000);
}


void loop() {
  int rdxc = digitalRead(SWITCH);
  if ((rdxc == 1) && (idx >= 0)) {
    idx = -1;
  }
  if ((rdxc == 0) && (idx < 0))
    if (preIdx != -1)
    {
      idx = preIdx;
      preIdx = -1;
    }
    else idx = 0;
  Serial.println(idx);
  Serial.println(rdxc);
  if (idx >= 0) getMostViewed();
  else showClock();
}
