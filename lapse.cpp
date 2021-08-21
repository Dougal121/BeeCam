#include "Arduino.h"
#include "camera.h"
#include <stdio.h>
#include "file.h"
#include <TimeLib.h>
#include <fb_gfx.h>

extern cam_stuff_t camstuff ;

unsigned long currentSamples = 0;
unsigned long lapseIndex = 0;
unsigned long frameInterval = 60;
long postSunrise = 0 ;


bool mjpeg = true;
bool lapseRunning = false;
unsigned long lastFrameTime = 0;
unsigned long numberSamples = 240 ;

void setInterval(unsigned long delta){
    frameInterval = delta;
}

void setNumerSamples(unsigned long Samples){
    numberSamples = Samples;
}
void setPostSunrise(unsigned long Sunrise){
    postSunrise = Sunrise;
}

bool startLapse(){
  if(camstuff.lapseRunning) 
    return true;
  camstuff.currentSample = 0;
  camstuff.lastFrameTime = 0;
  camstuff.lapseRunning = true;
	return false;
}

bool stopLapse(){
    camstuff.lapseRunning = false;
}

bool processLapse(){   // called once a second
bool bRet = false;  
//    if(!camstuff.lapseRunning) return false;
//    lastFrameTime++;
//    if ( lastFrameTime >= frameInterval ){
      lastFrameTime = 0 ;
      camera_fb_t *fb = NULL;
      esp_err_t res = ESP_OK;
 //     sensor_t *s = esp_camera_sensor_get();
 //     s->set_framesize(s, FRAMESIZE_VGA);
      
      fb = esp_camera_fb_get();
      
  //        dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);        
  //        fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  //        rgb_print(image_matrix, 0x000000FF, "test");
  //    fb_gfx_printf(fb,10,10,0xffffff,"test" );
      
      if (!fb){
        Serial.println("Camera capture failed");
        return false;
      }
  
      char path[32];
      sprintf(path, "/day%02d%02d", month(), day());
      if (!fileExists(path)){
          createDir(path);
      }
      
      sprintf(path, "/day%02d%02d/bp%02d%02d%02d.jpg", month(), day(),hour(),minute(),second() );
      Serial.println(path);
      if(!writeFile(path, (const unsigned char *)fb->buf, fb->len)) {
          lapseRunning = false;
          return false;
      }
      esp_camera_fb_return(fb);
      
//      camstuff.currentSample++ ;
//      if (currentSample > camstuff.numberSamples ) {
//        camstuff.lapseRunning = false;
//      }
      bRet = true ;
//    }
    return(bRet);
}





