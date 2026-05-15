

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


const char bootTextLogo[] = "S@f";

ArduinoLEDMatrix matrix;


/***
 * Important note:
 * Stack memory is very tight
 * If you allocate too much memory, the system will crash and will be unable to proceed
 * You can detect it because the debug monitor will not print the task name
 * 
 */
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(57600);
  matrix.begin();

  // normally configMAX_PRIORITIES=5
  
  // Now set up two tasks to run independently.
   xTaskCreate(
    TaskBlink
    ,  "Blink"   // A name just for humans
    ,  128-93  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL //Parameters passed to the task function
    ,  0  // Priority, with 2 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &taskBlink_Handler );//Task handle


/*
  xTaskCreate(TaskMelodyBase
    ,"Melody"
    ,256-155
    , (void*) 2 // pin 
    , 2
    ,&taskMelodyBase_Handler);
*/
  const int stack=190;
  // Two fade dancer task to kill
  xTaskCreate(
    TaskFadeDance
    ,"FadeDance9"
    ,stack
    ,( void * ) 9
    ,1
    ,&fadeDance_Handler1);


  xTaskCreate(
    TaskFadeDance
    ,"FadeDance10"
    ,stack
    ,( void * ) 10
    , 1 
    ,&fadeDance_Handler2);


  xTaskCreate(
    TaskFadeDance
    ,"FadeDance3"
    ,stack
    ,( void * ) 6
    , 1
    ,&fadeDance_Handler3);

  

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
 * This task control the Matrix
 */
void TaskCloudConnect(void* pvParameters){
  (void) pvParameters;

    matrix.loadFrame(heart);
    connectToWiFi();
    syncClock();

    xTaskCreate(TaskWebServer
    , "WebServer"
    , 512-300 // Very strict is 338
    , NULL
    , configMAX_PRIORITIES - 1 /* Give high priority */
    , &taskWebServer_Handler);

     matrix.loadFrame(happy);
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
 * The general idea is to stop at some times (like quarters)
 * Also the led will go faster near the end of the hour, and will be slower otherwise
 */
void TaskFadeDance(void *pvParameters){  
  const int led = ( uint32_t ) pvParameters;         // the PWM pin the LED is attached to
  int brightness = 0;  // how bright the LED is
  int fadeAmount = 10+led;  // how many points to fade the LED by, parametrized a bit by led number
  pinMode(led, OUTPUT);
  for(;;){

      RTCTime currentTime;
      RTC.getTime(currentTime); 
      //Serial.println("Fade Time " + String(currentTime));  
      auto current_minute=currentTime.getMinutes();
      switch( current_minute ){

        // Before clock setup its value is zero: we do not want to crash
        case 1:        
        case 15:
        case 30:
        case 45:
          analogWrite(led,255);
          //Serial.println("** DONG **");
          // Wait for 1 minute
          vTaskDelay(60 *(1000/portTICK_PERIOD_MS));
          continue;
          break;
        default:
          break;

      }

      // Magically move the limit and we are bright when we are around end of hour
      int magi_limit=(255*current_minute)/59;



      // set the brightness of pin 9:
      analogWrite(led, brightness);

      // change the brightness for next time through the loop:
      brightness = brightness + fadeAmount;

      // reverse the direction of the fading at the ends of the fade:
      // if (brightness <= 0 || brightness >= magi_limit) {
      if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
      }
      // wait for 30 milliseconds to see the dimming effect
      // We adjust it based on the current minute: it is faster during the end of the hour
      vTaskDelay( ((59-current_minute)/2)/portTICK_PERIOD_MS);
  }

}
////////// Melody
// notes in the melody:
#include "pitches.h"

const int melody[] = {
  NOTE_C5, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, 0, NOTE_B4, NOTE_C5
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
const int noteDurations[] = {  
  4, 8, 8, 4, 4, 4, 4, 4
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
