

#include "noeboat_config.h"
// FreeRTOS@10.5.1-1
#include "src/Arduino_FreeRTOS.h"

/// Wiring
const int potPin = A0;  
/////// Wiring here is imortant
const uint8_t OrderedLeds[]={3,/*5,*/10,9,11,6};

////

//define task handles
TaskHandle_t taskBlink_Handler;
TaskHandle_t fadeDance_Handler1;

TaskHandle_t taskMelodyBase_Handler;

TaskHandle_t* listOfHandler2Monitor[]={
  //&taskBlink_Handler
  &taskMelodyBase_Handler
  ,&fadeDance_Handler1
  // ,&taskMelodyBase_Handler
};

// define overall tasks
void TaskBlink( void *pvParameters );
void TaskSystemStatus(void* pvParameters);

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(57600);

  // normally configMAX_PRIORITIES=5
  Serial.print(F("portTICK_PERIOD_MS=")); Serial.println(portTICK_PERIOD_MS);
  Serial.print(F("configMAX_PRIORITIES=")); Serial.println(configMAX_PRIORITIES);
  Serial.print(F("configMINIMAL_STACK_SIZE=")); Serial.println(configMINIMAL_STACK_SIZE);


  // We have approx 1624 bytes
  // We have 6 task running more or less so we CANNOT allocate 
  // apporx more than 200 bytes per thread.
  // If we keep low the thread count we can have interesting effects on the go
  // Stack allocation is way more safer than heap allocation, if correctly tuned

  // xTaskCreate(TaskMelodyBase
  //   ,"Melody"
  //   ,256-155
  //   , (void*) 2
  //   , 2
  //   ,&taskMelodyBase_Handler);

  const uint16_t faderStackSize=64;

  xTaskCreate(TaskFadeCycle, "DNC", faderStackSize, NULL, 0, &fadeDance_Handler1);
  
  // TODO: Create a task to detect the press of a button, to change the effect taking the next effect in a list
  
  if(DEBUG_MODE){
    xTaskCreate(TaskSystemStatus
      , NULL
      , 200
      , NULL
      , configMAX_PRIORITIES-1  // Highest priority to betetr track down memory
      , NULL);
  }else{
    Serial.println("No Debug active");
  }

  vTaskStartScheduler();
}


    
// configUSE_IDLE_HOOK=0 so we does not need a loop
// void loop()
// {
//   // Empty. Things are done in Tasks.
// }

// Support functions

/** 8bit resolution potentiometer read.
 */
inline uint8_t readPot(){
  return analogRead(potPin) / 4;
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskSystemStatus(void *pvParameters){
  (void) pvParameters;
  for(;;){
    Serial.println(F(__FILE__));
    Serial.println(F("======== Tasks status ========"));
    Serial.print(F("Tick count: "));
    Serial.print(xTaskGetTickCount());
    Serial.print(F(", Task count: "));
    Serial.println(uxTaskGetNumberOfTasks());

    Serial.print("POT:");
    Serial.println(readPot());

    // Lower the Mark, more probable a overflow
    Serial.println(F("== High Watermarks =="));    
  
    for(auto handler: listOfHandler2Monitor){
      Serial.print(pcTaskGetName(*handler)); Serial.print("-> ");
      Serial.println(uxTaskGetStackHighWaterMark(*handler));
    }

    Serial.println();
    Serial.println();
    
    vTaskDelay( (5* 1000)/  portTICK_PERIOD_MS);
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




// lights turn on on a cycle
void TaskFadeCycle(void *pvParameters){

  // Wiring order is important
  int brightness = 0;  // how bright the LED is
  int fadeAmount = 5;  // how many points to fade the LED by
  
  for(auto l: OrderedLeds){ pinMode(l,OUTPUT); }
  uint8_t previousLed=11;
  
  // use potentiometer to drive it
  uint8_t maxValue=255;
  for(;;){
    uint8_t dimValue=readPot();
    // Dim all led
    for(auto currentLed: OrderedLeds ){
      analogWrite(currentLed,dimValue);
    }
    // Emulate a bouncing ball of light
    vTaskDelay(300/portTICK_PERIOD_MS);

    for(auto currentLed: OrderedLeds ){
      analogWrite(currentLed,maxValue);
      analogWrite(previousLed,readPot());
      previousLed=currentLed;
      vTaskDelay(200/portTICK_PERIOD_MS);      
    }
  }

}

void TaskFadeSyncro(void *pvParameters){
  int brightness = 0;  // how bright the LED is
  int fadeAmount = 5;  // how many points to fade the LED by
  for(auto l: OrderedLeds){ pinMode(l,OUTPUT); }

  for(;;){
    for(auto ledx: OrderedLeds ){
      analogWrite(ledx, brightness);

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
}



/** Generic Fading procedure
 */
void TaskFadeDance(void *pvParameters){
  const int led = ( uint32_t ) pvParameters;         // the PWM pin the LED is attached to
  int brightness = 0;  // how bright the LED is
  int fadeAmount = 5;  // how many points to fade the LED by
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
  NOTE_C5, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4, 0, NOTE_B4, NOTE_C5
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
const uint8_t noteDurations[] = {  
  4, 8, 8, 4, 4, 4, 4, 4
};

// Use of the tone() function will interfere with PWM output on pins 3 and 11
void TaskMelodyBase(void *pvParameters){
  const int pin = ( uint32_t ) pvParameters;      
    for (int thisNote = 0; thisNote < 8; thisNote++) {
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
