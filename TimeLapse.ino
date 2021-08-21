/*    284D2207D6013CCD Device 0
 *    285B0707D6013C0E Device 1
 */

#include <WiFi.h>
#include "file.h"
#include "camera.h"
#include "lapse.h"
#include <WiFiUDP.h>
#include <TimeLib.h>
#include "ds3231.h"
#include "lapse.h"
#include "SD_MMC.h"
#include <Update.h>
#include <WebServer.h>
#include <EEPROM.h>

#include "StaticPage.h"

#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include <img_converters.h>
//#include <fb_gfx.h>
////#include "dl_lib.h"
//#include "dl_lib_matrix3d.h"

char Toleo[10] = {"Ver 1.0\0"}  ;

#include <OneWire.h>
#include <DallasTemperature.h>

const int MAX_EEPROM = 2000 ;
const byte MAX_WIFI_TRIES = 45 ;
const int PROG_BASE = 256 ;   // where the irrigation valve setup and program information starts in eeprom

#define ONE_WIRE_BUS 3   // this is the RX pin of the UART  - so we can still debug
#define TEMPERATURE_PRECISION 12
#define MYVER 0x12000000     // change this if you change the structures that hold data that way it will force a "backinthebox" to get safe and sane values from eeprom

#define BUILTIN_LED 33

#define BUFF_MAX 32
char buff[BUFF_MAX];

IPAddress MyIP ;
IPAddress MyIPC  ;

WebServer server(82);

const int NTP_PACKET_SIZE = 48;       // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];   //buffer to hold incoming and outgoing packets

typedef struct __attribute__((__packed__)) {     // eeprom stuff
  unsigned int localPort = 2390;          // 2 local port to listen for NTP UDP packets
  unsigned int localPortCtrl = 8666;      // 4 local port to listen for Control UDP packets
  unsigned int RemotePortCtrl = 8664;     // 6 local port to listen for Control UDP packets
  long lNodeAddress ;                     // 22
  float fTimeZone ;                       // 26
  IPAddress RCIP ;                        // (192,168,2,255)  30
  char NodeName[16] ;                     // 46
  char nssid[16] ;                        // 62
  char npassword[16] ;                    // 78
  time_t AutoOff_t ;                      // 82     auto off until time > this date
  uint8_t lDisplayOptions  ;              // 83
  uint8_t lNetworkOptions  ;              // 84
  uint8_t lSpare1  ;                      // 85
  uint8_t lSpare2  ;                      // 86
  char timeServer[24] ;                   // 110   = {"au.pool.ntp.org\0"}
  char cpassword[16] ;                    // 126
  long lVersion  ;                        // 130
  IPAddress IPStatic ;                    // (192,168,0,123)
  IPAddress IPGateway ;                   // (192,168,0,1)
  IPAddress IPMask ;                      // (255,255,255,0)
  IPAddress IPDNS ;                       // (192,168,0,15)
  struct ts tc;            //
  float ha ;
  float sunX ;
  float sunrise ;
  float sunset ;
  float latitude;          //
  float longitude;         //
  float tst ;

  
//  float solar_az_deg;      //
//  float solar_el_deg;      //
//  int iDayNight ;          //
//  float decl ;
//  float eqtime ;
} general_housekeeping_stuff_t ;          // computer says it's  ??? is my maths crap ????
general_housekeeping_stuff_t ghks ;

cam_stuff_t camstuff ;



typedef struct __attribute__((__packed__)) {     // eeprom stuff
  struct ts tc;            //
  float ha ;
  float sunX ;
  float sunrise ;
  float sunset ;
  float tst ;
  float solar_az_deg;      //
  float solar_el_deg;      //
  int   iDayNight ;          //
  float decl ;
  float eqtime ;
} Solar_App_stuff_t ;          

Solar_App_stuff_t SolarApp  ;

typedef struct __attribute__((__packed__)) {             // tempary record
  float     fTemp[4] ;
} internal_t ;

internal_t          esui ;      // internal volitoile states
char cssid[32] = {"Configure_XXXXXXXX\0"} ;
uint64_t chipid;



OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature.

DeviceAddress Thermometer[4];  // arrays to hold device addresses
WiFiUDP ntpudp;

struct ts tc;

long  MyCheckSum ;
long  MyTestSum ;
int bSaveReq = 0 ;
int iUploadPos = 0 ;
long lScanCtr = 0 ;
long lScanLast = 0 ;
long lRebootCode = 0 ;
bool  hasRTC = false ;
bool bManSet = true ;
bool hasSD = false ;
unsigned long lTimeNext = 0 ;     // next network retry

float rtc_temp = 0 ;
int rtc_hour = -1 ;
int rtc_min = -1 ;
int rtc_sec = - 1 ;
int rtc_status = -1 ;
long lMinUpTime = 0 ; 
bool bDoTimeUpdate = false ;
const char *ssid = "TP-LINK_52FC8C\0";
const char *password = "0052FC8C\0";
bool bPrevConnectionStatus = false;

long lcct ;
long lsrt ;

void startCameraServer();

void handleNotFound() {
  String message = F("Seriously - No way DUDE !!!\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
  //  Serial.print(message);
}


void setup()
{
int i = 0  ;  
int j = 0 ;
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println();
  pinMode(BUILTIN_LED, OUTPUT); // GPIO33

  EEPROM.begin(MAX_EEPROM);
  LoadParamsFromEEPROM(true);
  if ( MYVER != ghks.lVersion ) {
    BackInTheBoxMemory();
  }
  
  initFileSystem();
  initCamera();

  sensors.begin(); // Start up the Dallas library

  Serial.print("Parasite power is: ");// report parasite power requirements
  if (sensors.isParasitePowerMode()) 
    Serial.println("ON");
  else 
    Serial.println("OFF");

  for (i = 0 ; i < 3 ; i++){
    if (sensors.getAddress(Thermometer[i], i)) {
        printAddress(Thermometer[i]);
        sensors.setResolution(Thermometer[i], TEMPERATURE_PRECISION);
        Serial.print(" Device "+String(i)+" Resolution: ");
        Serial.print(sensors.getResolution(Thermometer[i]), DEC);
        Serial.print(" ");
        sensors.requestTemperatures();
        printTemperature(Thermometer[i]);
        Serial.println();
    }
    else
      Serial.println("Unable to find address for Device "+String(i));
  }  
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" Temperature devices found.");

  GetTempLogs() ; // grab the data if there is any
  Serial.println(String(esui.fTemp[0])+" "+String(esui.fTemp[1]));
  GetTempLogs() ; // grab the data if there is any
  Serial.println(String(esui.fTemp[0])+" "+String(esui.fTemp[1]));

 
  WiFi.disconnect(false);
  WiFi.mode(WIFI_AP_STA);  // we are having our cake and eating it eee har
  WiFi.setAutoReconnect(true);
//  WiFi.setTxPower(WIFI_POWER_19_5dBm);
//  WiFi.setTxPower(WIFI_POWER_2dBm);
  chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf(cssid, "Configure_%08X\0", chipid);
  Serial.print("SSID: ");
  Serial.println(cssid);
  WiFi.softAP((char*)cssid);                   // no passowrd

  MyIPC = WiFi.softAPIP();  // get back the address to verify what happened
  Serial.print("Soft AP IP address: ");
  snprintf(buff, BUFF_MAX, ">> IP %03u.%03u.%03u.%03u <<", MyIPC[0], MyIPC[1], MyIPC[2], MyIPC[3]);
  Serial.println(buff);
  
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: >");
  Serial.print(password);
  Serial.println("<");
    
	WiFi.begin(ssid, password);
	while ((WiFi.status() != WL_CONNECTED) && ( j < 120 ))
	{
		delay(250);
   if (i++ > 20){
      Serial.println(".");
      i = 0 ;    
   }else{
		  Serial.print(".");
   }
   j++ ; 
   digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));    
   
	}
	Serial.println("");
	Serial.println("WiFi connected");
	startCameraServer();
	Serial.print("Camera Ready! Use 'http://");
	Serial.print(WiFi.localIP());
  MyIP = WiFi.localIP() ;
  
	Serial.println("' to connect");
/*
  if (SD.begin(SS)) {
    Serial.println("SD Card initialized.");
    hasSD = true;
  }else{
    Serial.println("SD Card failed on startup.");
    hasSD = false;
  }*/
  hasSD = true ;

  server.on("/", handleRoot);
  server.on("/setup", handleRoot);
  server.on("/stime", handleRoot);
  server.on("/info", handleInfo);
  server.on("/eeprom", DisplayEEPROM);
  server.on("/backup", HTTP_GET , handleBackup);
  server.on("/backup.txt", HTTP_GET , handleBackup);
  server.on("/backup.txt", HTTP_POST,  handleRoot, handleFileUpload);
  server.on("/list", HTTP_GET, indexSDCard);
  server.on("/download", HTTP_GET, downloadFile);
//  server.on("/chartfile", HTTP_GET, chartFile);

  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", updatePage);
  });
  server.on("/update", HTTP_POST, []() {   //handling uploading firmware file
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {

      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {   // flashing firmware to ESP
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.onNotFound(handleNotFound);
  server.begin();
 
  
  
  Serial.println("Starting UDP");
  ntpudp.begin(ghks.localPort);                      // this is the recieve on NTP port
  Serial.println("NTP Local UDP port: "+String(ghks.localPort));



  
  sendNTPpacket(ghks.timeServer); // send an NTP packet to a time server
}


void loop()
{
	unsigned long t = -1;
	static unsigned long ot = 0;
	unsigned long dt = t - ot;
  unsigned long dtNow ; 

	ot = t;
  long lTD ;
 
  lScanCtr++ ;

  if ( rtc_min != minute()) {
    lMinUpTime++ ;
    if ((bDoTimeUpdate)) {  // not the correct time try to fix every minute
      sendNTPpacket(ghks.timeServer); // send an NTP packet to a time server
      bDoTimeUpdate = false ;
    }
    DoSolarCalcs();
  }
  if (second() > 4 ) {
    if ( ntpudp.parsePacket() ) {
      processNTPpacket();
    }
  }
  if ( rtc_sec != second()){
    GetTempLogs() ; // grab the data if there is any
    GetTempLogs() ; // grab the data if there is any
//    Serial.println(String(esui.fTemp[0])+" "+String(esui.fTemp[1]));
    if (camstuff.lapseRunning) {
      camstuff.lastFrameTime++ ;
      if ( camstuff.lastFrameTime >= camstuff.frameInterval ){
        if (((camstuff.frameInterval % 60 ) != 0 ) || ( rtc_sec == 59 )) { //  if set to minute process on the minute
          camstuff.lastFrameTime = 0 ;
          if ( processLapse() ){
              DataLog() ;             //  if we same take a temperature 
          }
          camstuff.currentSample++ ;
          if (camstuff.currentSample > camstuff.numberSamples ) {
            camstuff.lapseRunning = false;
            Serial.println("Casmera timer to OFF mode");
          }      
        }
      }
    }
    rtc_sec = second() ;
    lScanLast = lScanCtr ;
    lScanCtr = 0 ;    
    digitalWrite(BUILTIN_LED, !digitalRead(BUILTIN_LED));    
   
    dtNow = now() ;
    if ( year(dtNow) > 2020 ){// valid time
      dtNow -= camstuff.postSunrise ;
      lsrt = ( HrsSolarTime(SolarApp.sunrise) * 100 ) + MinSolarTime(SolarApp.sunrise) ;
      lcct = ( hour(dtNow) * 100 ) + minute(dtNow)  ;
//      Serial.println(String(lsrt) + " " +String(lcct) + " " + String(dtNow)+ " "+String(HrsSolarTime(SolarApp.sunrise)) + " " + String(MinSolarTime(SolarApp.sunrise)));
      if (!camstuff.lapseRunning ){
        if (( lcct == lsrt ) && ( camstuff.postSunrise >= 0 )) {   // if we are at sunrise
          camstuff.lapseRunning = true;
          camstuff.currentSample = 0 ; 
          Serial.println("Casmera timer to ON mode");
        }   
      }
    }
  }
  server.handleClient();

  snprintf(buff, BUFF_MAX, "%d/%02d/%02d %02d:%02d:%02d", year(), month(), day() , hour(), minute(), second());
  if ( !bPrevConnectionStatus && WiFi.isConnected() ){
    Serial.println(String(buff )+ " WiFi Reconnected OK...");  
    MyIP =  WiFi.localIP() ;
  }
  if (!WiFi.isConnected())  {
    lTD = (long)lTimeNext-(long) millis() ;
    if (( abs(lTD)>40000)||(bPrevConnectionStatus)){ // trying to get roll over protection and a 30 second retry
      lTimeNext = millis() - 1 ;
/*      Serial.print(millis());
      Serial.print(" ");
      Serial.print(lTimeNext);
      Serial.print(" ");
      Serial.println(abs(lTD));*/
    }
    bPrevConnectionStatus = false;
    if ( lTimeNext < millis() ){
      Serial.println(String(buff )+ " Trying to reconnect WiFi ");
      WiFi.disconnect(false);
//      Serial.println("Connecting to WiFi...");
      WiFi.mode(WIFI_AP_STA);
      if ( ghks.lNetworkOptions != 0 ) {            // use ixed IP
        WiFi.config(ghks.IPStatic, ghks.IPGateway, ghks.IPMask, ghks.IPDNS );
      }
      if ( ghks.npassword[0] == 0 ) {
        WiFi.begin((char*)ghks.nssid);                    // connect to unencrypted access point
      } else {
        WiFi.begin((char*)ghks.nssid, (char*)ghks.npassword);  // connect to access point with encryption
      }
      lTimeNext = millis() + 30000 ;
    }
  }else{
    bPrevConnectionStatus = true ;
  }  


 
}
