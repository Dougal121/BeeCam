void BackInTheBoxMemory(){
  int i , j ;

//  sprintf(ghks.nssid,"************\0");  // put your default credentials in here if you wish
//  sprintf(ghks.npassword,"********\0");  // put your default credentials in here if you wish



  sprintf(ghks.cpassword,"\0");
  
  sprintf(ghks.NodeName,"Bee CAM\0") ;
  ghks.fTimeZone = 10.0 ;
  ghks.lNodeAddress = (long)chipid & 0xff ;
  sprintf(ghks.timeServer ,"au.pool.ntp.org\0"); 
  ghks.AutoOff_t = 0 ;
  ghks.localPortCtrl = 8088 ;
  ghks.RemotePortCtrl= 8089 ;
  ghks.lVersion = MYVER ;
  
  ghks.RCIP[0] = 192 ;
  ghks.RCIP[1] = 168 ; 
  ghks.RCIP[2] = 2 ;
  ghks.RCIP[3] = 255 ;
  
  ghks.lNetworkOptions = 0 ;     // DHCP 
  ghks.IPStatic[0] = 192 ;
  ghks.IPStatic[1] = 168 ;
  ghks.IPStatic[2] = 2 ;
  ghks.IPStatic[3] = 234 ;

  ghks.IPGateway[0] = 192 ;
  ghks.IPGateway[1] = 168 ;
  ghks.IPGateway[2] = 1 ;
  ghks.IPGateway[3] = 1 ;

  ghks.IPDNS = ghks.IPGateway ;

  ghks.IPMask[0] = 255 ;
  ghks.IPMask[1] = 255 ;
  ghks.IPMask[2] = 255 ;
  ghks.IPMask[3] = 0 ;

  ghks.latitude = -34.051219 ;
  ghks.longitude = 142.013618 ;

  camstuff.frameInterval = 120; // seconds
  camstuff.postSunrise = 1800 ;  // seconds
  camstuff.numberSamples = 120 ;
  camstuff.currentSample = 0 ;
  camstuff.lapseRunning = false ;
  camstuff.lastFrameTime = 0 ;
}
