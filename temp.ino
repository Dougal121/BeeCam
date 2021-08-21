/*
For reference - probe addresses




*/
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

void GetTempLogs(void){
  float fTemp;
  sensors.requestTemperatures();  
  sensors.requestTemperatures();  
  for (int i = 0 ; i < 2 ; i++ ) {
    fTemp = sensors.getTempC(Thermometer[i]) ;
    if ( fTemp > -50.0 ){
      esui.fTemp[i]= fTemp ;
    }
  }
}

void DataLog(){
bool bHeader = false ;
  sprintf(buff, "/day%02d%02d/temp.csv", month(), day());
  
  Serial.println(buff);
  if ( hasSD ){
    if (!SD_MMC.exists(buff)){    // ok if its the first write include a header
      bHeader = true ;
    }
    File dataFile = SD_MMC.open(buff, FILE_WRITE);
    if (dataFile) {
      Serial.print(".");
      if ( bHeader) {
        dataFile.println("Time,Temp1,Temp2,RSSI");
        Serial.print("*");
      }else{
        dataFile.seek(dataFile.size());
      }
      Serial.print(".");
      snprintf(buff, BUFF_MAX, "%4d-%02d-%02dT%02d:%02d:%02d", year(), month(), day() , hour(), minute(), second() );
      dataFile.print(String(buff)+",");
      dataFile.println(String(esui.fTemp[0],2)+","+String(esui.fTemp[1],2)+","+String(abs(WiFi.RSSI())));
      dataFile.close();
    }    
  }
  Serial.println("");
}


void SerialOutParams() {
  String message ;

  message = "Web Request URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  Serial.println(message);
  message = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  Serial.println(message);
}

void SendHTTPHeader() {
  String message ;
  String strOnOff ;
  
  server.sendHeader(F("Server"), F("ESP32-tinfoil-hats-on-please"), false);
  server.sendHeader(F("X-Powered-by"), F("Dougal-filament-7"), false);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(F("<!DOCTYPE HTML>"));
  server.sendContent("<head><title>ESP Bee CAM " + String(Toleo) + "</title>");
  server.sendContent(F("<meta name=viewport content='width=320, auto inital-scale=1'>"));
//  server.sendContent(F("<link rel='icon' type='image/png' href='download?file=/favorite.ico'>"));
  server.sendContent(F("</head><body><html lang='en'><center><h3>"));
  server.sendContent("<a title='click for home / refresh' href='/'>" + String(ghks.NodeName) + "</a></h3>");
}


void SendHTTPPageFooter() {
  server.sendContent(F("<br><a href='/?command=1'>Load Parameters from EEPROM</a><br><br><a href='/?command=667'>Reset Memory to Factory Default</a><br><a href='/?command=665'>Sync UTP Time</a><br><a href='/stime'>Manual Time Set</a><br>")) ;
  server.sendContent("<a href='/?reboot=" + String(lRebootCode) + "'>Reboot</a><br>");
  //  server.sendContent(F("<a href='/?command=668'>Save Fert Current QTY</a><br>"));
  server.sendContent(F("<a href='/eeprom'>EEPROM Memory Contents</a><br>"));
  server.sendContent(F("<a href='/setup'>Node Setup</a><br>"));
  server.sendContent(F("<a href='/list'>SD card file list</a><br>"));
  server.sendContent(F("<a href='/chartsetup'>SD card chart setup</a><br>"));
  server.sendContent(F("<a href='/info'>Node Infomation</a><br>"));  
  server.sendContent(F("<a href='/vsss'>view volatile memory structures</a><br>"));
  if (!WiFi.isConnected()) {
    snprintf(buff, BUFF_MAX, "%u.%u.%u.%u", MyIPC[0], MyIPC[1], MyIPC[2], MyIPC[3]);
  } else {
    snprintf(buff, BUFF_MAX, "%u.%u.%u.%u", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  }
  server.sendContent("<a href='http://" + String(buff) + "/update'>OTA Firmware Update</a><br>");
  server.sendContent("<a href='https://github.com/Dougal121/BeeCAM'>Source at GitHub</a><br>");
  server.sendContent("<a href='http://" + String(buff) + "/backup'>Backup / Restore Settings</a><br>");
/*  if (hasSD){
    server.sendContent("<a href='http://" + String(buff) + "/list'>List log files on SD Card</a><br>");    
  }
*/  
  server.sendContent(F("</body></html>\r\n"));
}



void printDirectory(File dir, int numTabs,  String MyFolder) {
File entry ;  
int i ;
String strLeaf ;
static unsigned long lTB ;
static int iCol ; 

//  Serial.println("Called " + String(numTabs));
  if( numTabs == 0 ) {
    lTB = 0 ;  
    dir.rewindDirectory(); // start at the begining
  }
  while(entry = dir.openNextFile()) {  // single = very subtle assign and test
//    Serial.println(entry.name());
    if (( iCol % 4 ) ==  0 ){
      server.sendContent("<tr>");
    }
    if (entry.isDirectory()) {
        MyFolder = String("/") + entry.name() ;
        MyFolder = entry.name() ;
//        server.sendContent("<tr><td><b>" + MyFolder + "</td></tr>" );
        server.sendContent("<td><b>" + MyFolder + "</td>" );
        printDirectory(entry, numTabs+1, MyFolder);
//        Serial.println("Returned ");
    }else{
//      server.sendContent("<tr><td><A href='download?file="); 
      server.sendContent("<td><A href='download?file="); 
      server.sendContent(entry.name());  //      + "/"
      server.sendContent("'>");
      server.sendContent(entry.name());
      server.sendContent("</a></td><td> </td><td align=right>");
      server.sendContent(String(entry.size(), DEC));      
      lTB += entry.size() ;  
      server.sendContent("</td>");
      i = String(entry.name()).lastIndexOf(".csv");
      if (i != -1) { //
        strLeaf = String("<td><A href='chartfile?file="+String(entry.name())+"'><img src='download?file=/chart.ico' alt='chart' width=16 height=16></a></td>");
        server.sendContent(strLeaf);
      }       
//      server.sendContent("</tr>");
    }
    if (( iCol % 4 ) ==  3 ){
      server.sendContent("</tr>");
    }
    iCol++ ;
  }
  if( numTabs == 0 ) {
     server.sendContent("<tr><td colspan=3> </td></tr>");
     server.sendContent("<tr><td><b>Total Bytes</td><td> </td><td align=right><b>");
     server.sendContent(String(lTB, DEC));      
     server.sendContent("</td></tr>");
  }
}


void indexSDCard()
{
  String message ;
  
  SerialOutParams();
  SendHTTPHeader();   //  ################### START OF THE RESPONSE  ######
  File root = SD_MMC.open("/");                
  server.sendContent("<table border=0>");
  printDirectory(root,0,"/");
  server.sendContent("</table>");      
  root.close();
  SendHTTPPageFooter();  
}

void downloadFile()
{
  String strExt = "" ;
  String dataType = "" ;
  String strFile = "" ;
  String strLeaf = "" ;
  int i = 0 ;

  SerialOutParams();
    
  for (uint8_t j = 0; j < server.args(); j++) {
    //    bSaveReq = 1 ;
    i = String(server.argName(j)).indexOf("file");
    if (i != -1) { //
      i = String(server.arg(j)).lastIndexOf(".");
      if (i != -1) { //
        strExt = String(server.arg(j)).substring(i+1);
        strExt.toLowerCase();
        strFile = String(server.arg(j)) ;
        strLeaf = strFile ; 
        i = String(server.arg(j)).lastIndexOf("/");
        if (i != -1) { //
          strLeaf = String(server.arg(j)).substring(i+1);
        }
      }
    }
  }
  Serial.println("Leaf: "+String(strLeaf));
  Serial.println("File: "+String(strFile));
  Serial.println("Ext: "+String(strExt));

  if (strExt.lastIndexOf("htm")>0) {
    dataType = "text/html";
  } else if (strExt.lastIndexOf("css")>=0) {
    dataType = "text/css";
  } else if (strExt.lastIndexOf("js")>=0) {
    dataType = "application/javascript";
  } else if (strExt.lastIndexOf("png")>=0) {
    dataType = "image/png";
  } else if (strExt.lastIndexOf("gif")>=0) {
    dataType = "image/gif";
  } else if (strExt.lastIndexOf("jpg")>=0) {
    dataType = "image/jpeg";
  } else if (strExt.lastIndexOf("ico")>=0) {
    dataType = "image/x-icon";
  } else if (strExt.lastIndexOf("xml")>=0) {
    dataType = "text/xml";
  } else if (strExt.lastIndexOf("pdf")>=0) {
    dataType = "application/pdf";
  } else if (strExt.lastIndexOf("zip")>=0) {
    dataType = "application/zip";
  } else if (strExt.lastIndexOf("csv")>=0) {
    dataType = "application/csv";
  }
  Serial.println(dataType);
  File dataFile = SD_MMC.open(strFile);
  if (dataFile) {                                 // if the file is available, write to it:
//    while (dataFile.available()) {
      server.sendHeader(F("Content-Disposition"),"inline; filename=" + String(strLeaf),true);
      server.streamFile(dataFile,dataType);
//      server.sendContent(String(dataFile.read()));
//    }
    dataFile.close();
  }else {    // if the file isn't open, pop up an error:
    server.sendContent("Error opening " + strFile);
  }
}


