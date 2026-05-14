// To use ArduinoGraphics APIs, please include BEFORE Arduino_LED_Matrix
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
// FreeRTOS@10.5.1-1
#include "Arduino_FreeRTOS.h"

ArduinoLEDMatrix matrix;

const uint8_t OrderedLeds[]={3,5,6 };

/**
* GG refer to https://www.arduino.cc/reference/en/libraries/arduinographics/
for more details
*/
void setup() {
  Serial.begin(115200);
  matrix.begin();

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  // add some static text to signal booting  
  const char text[] = "S@f";
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText();

  matrix.endDraw();

  for(auto l: OrderedLeds){ pinMode(l,OUTPUT); }
  delay(1000);
}

// const int scroll_speed=60;
const int scroll_speed=80;
const int master_delay=40;
void loop() {

  // Make it scroll!
  /*
  matrix.beginDraw();

  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(scroll_speed);

  // add the text
  const char text[] = "Sofia Magilamp";
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();
*/

  uint8_t dimValue=10;
  for(dimValue=10; dimValue<=120; dimValue+=10){
    for(auto currentLed: OrderedLeds ){
      analogWrite(currentLed,dimValue);
    }
    delay(master_delay);
  }
  for(dimValue=200; dimValue>=10; dimValue-=10){
    for(auto currentLed: OrderedLeds ){
      analogWrite(currentLed,dimValue);
    }
    delay(master_delay);
  }
  


}