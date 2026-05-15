

// Include does not work: you need to hack original library code
// Please enable INCLUDE_uxTaskGetStackHighWaterMark inside 
// FreeRTOSConfig.h of the Arduino_FReeRTOS library
#include "magilamp_config.h"
#include <Arduino_FreeRTOS.h>
#include "wifi-connect.h"
#include "web-server.h"

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

#if INCLUDE_uxTaskGetStackHighWaterMark==0
#error "INCLUDE_uxTaskGetStackHighWaterMark must be set to 1"
#endif
//define task handles
TaskHandle_t taskBlink_Handler;
TaskHandle_t taskConnect_Handler;
TaskHandle_t taskWebServer_Handler;
TaskHandle_t fadeDance_Handler1, fadeDance_Handler2, fadeDance_Handler3; // Very simple fading

TaskHandle_t taskMelodyBase_Handler;
TaskHandle_t textScroll_t;



TaskHandle_t* listOfHandler2Monitor[]={
  &taskBlink_Handler, &taskConnect_Handler, &taskWebServer_Handler
  ,&fadeDance_Handler1, &fadeDance_Handler2, &fadeDance_Handler3 
  //,&taskMelodyBase_Handler
  //,&textScroll_t
};

// define overall tasks
void TaskBlink( void *pvParameters );
void TaskCloudConnect(void* pvParameters);
void TaskSystemStatus(void* pvParameters);

ArduinoLEDMatrix matrix;
void bootText(){

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

}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(57600);
  
  bootText();
  // normally configMAX_PRIORITIES=5
  
  // Now set up two tasks to run independently.
   xTaskCreate(
    TaskBlink
    ,  "Blink"   // A name just for humans
    ,  128-93  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL //Parameters passed to the task function
    ,  0  // Priority, with 2 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &taskBlink_Handler );//Task handle




  xTaskCreate(TaskMelodyBase
    ,"Melody"
    ,256-155
    , (void*) 2 // pin 
    , 2
    ,&taskMelodyBase_Handler);


  // Two fade dancer task to kill
  xTaskCreate(
    TaskFadeDance
    ,"FadeDance9"
    ,156-80
    ,( void * ) 9
    ,1
    ,&fadeDance_Handler1);


  xTaskCreate(
    TaskFadeDance
    ,"FadeDance10"
    ,156-80
    ,( void * ) 10
    , 1 
    ,&fadeDance_Handler2);


  xTaskCreate(
    TaskFadeDance
    ,"FadeDance3"
    ,156-80
    ,( void * ) 6
    , 1
    ,&fadeDance_Handler3);


  // xTaskCreate(textScroll,"TXTS",4000,NULL,configMAX_PRIORITIES-1 ,&textScroll_t);

  // This task can slow down things so put it a lower priority
  xTaskCreate(TaskCloudConnect
    , "CC"
    , 512-310 // Computed super-strictly subtracting the high watermark
    , NULL
    , 0 // This super low priority allow to be sure everything else will start before CloudConnect freeze a lot of guys
    , &taskConnect_Handler);


  if(DEBUG_MODE){
    xTaskCreate(TaskSystemStatus
      , NULL
      , 200
      , NULL
      , configMAX_PRIORITIES-1  // Highest priority to better track down memory
      , NULL);
  }else{
    Serial.println("No Debug active");
  }

  vTaskStartScheduler();
}


    

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

/**
 * This task ensure WiFi+RTC is ok, then fire the WebServer 
 */
void TaskCloudConnect(void* pvParameters){
  (void) pvParameters;
    connectToWiFi();
    syncClock();

    xTaskCreate(TaskWebServer
    , "WebServer"
    , 512-300 // Very strict is 338
    , NULL
    , configMAX_PRIORITIES - 1 /* Give high priority */
    , &taskWebServer_Handler);
    // We cannot 'exit' from a task: we must invoke vTaskDelete
    vTaskDelete( NULL );
}
void TaskSystemStatus(void *pvParameters){
  (void) pvParameters;
  for(;;){
    Serial.println("======== Tasks status ========");
    RTCTime currentTime;
    RTC.getTime(currentTime); 
    Serial.println("CurrentTime " + String(currentTime));  
    Serial.print("Tick count: ");
    Serial.print(xTaskGetTickCount());
    Serial.print(", Task count: ");
    Serial.println(uxTaskGetNumberOfTasks());

    // GG: Lower the Mark, more probable a overflow
    // You can use it to tune stack size
    Serial.println("== High Watermarks ==");    
  
    for(auto handler: listOfHandler2Monitor){
      Serial.print(pcTaskGetName(*handler)); Serial.print("-> ");
      Serial.println(uxTaskGetStackHighWaterMark(*handler));
    }

    Serial.println();
    Serial.println();
    
    vTaskDelay( (23* 1000)/  portTICK_PERIOD_MS);
  }
}


void TaskBlink(void *pvParameters)  // This is a low priority task.
{
  (void) pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Blink!");
  for (;;) // A Task shall never return or exit.
  {
    //Serial.println(11);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //   Invert led Value
    // The Led will stop blinking
    vTaskDelay(  150/portTICK_PERIOD_MS ); 
  }
}

/** Generic Fading procedure
 */
void TaskFadeDance(void *pvParameters){
  const int led = ( uint32_t ) pvParameters;         // the PWM pin the LED is attached to
  int brightness = 0;  // how bright the LED is
  int fadeAmount = 10+led;  // how many points to fade the LED by, parametrized a bit by led number
  pinMode(led, OUTPUT);
  for(;;){
      // set the brightness of pin 9:
      analogWrite(led, brightness);

      // change the brightness for next time through the loop:
      brightness = brightness + fadeAmount;

      // reverse the direction of the fading at the ends of the fade:
      if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
      }
      // wait for 30 milliseconds to see the dimming effect
      vTaskDelay(30/portTICK_PERIOD_MS);
  }

}
////////// Melody
// notes in the melody:
#include "pitches.h"
const int melody[] = {
  NOTE_E6, NOTE_C6, NOTE_E6, NOTE_C6,
  NOTE_G6, NOTE_A6, NOTE_G6, NOTE_E6,
  NOTE_D6, NOTE_B5, NOTE_D6, NOTE_B5,
  NOTE_E6, NOTE_C6
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
const int noteDurations[] = {
  6, 4, 8, 3,
  16, 16, 16, 8,
  8, 4, 8, 3,
  6, 2
};

// Use of the tone() function will interfere with PWM output on pins 3 and 11
void TaskMelodyBase(void *pvParameters){
  const int pin = ( uint32_t ) pvParameters;
    for (int thisNote = 0; thisNote < (int)(sizeof(melody) / sizeof(melody[0])); thisNote++) {
      // to calculate the note duration, take one second divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / noteDurations[thisNote];

      tone(pin, melody[thisNote], noteDuration);

      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);

      // stop the tone playing:
      noTone(pin);
    }
  // complete this task
  vTaskDelete( NULL );
}
