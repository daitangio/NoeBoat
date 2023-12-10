

#include "noeboat_config.h"
// FreeRTOS@10.5.1-1
#include "Arduino_FreeRTOS.h"

/// Wiring
const int potPin = A0;  
const int modeButtonPin=7;
/////// Wiring here is imortant
const uint8_t OrderedLeds[]={6,11,9,10,3 };

//// Internal config do not edit from this point...normally
const uint16_t FaderStackSize=64;

//define task handles
TaskHandle_t majorTaskHandler;


// define overall tasks
void TaskBlink( void *pvParameters );
void TaskSystemStatus(void* pvParameters);
void TaskDebugButton(void *pvParameters);

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(57600);

  // normally configMAX_PRIORITIES=5
  Serial.print(F("portTICK_PERIOD_MS=")); Serial.println(portTICK_PERIOD_MS);
  Serial.print(F("configMAX_PRIORITIES=")); Serial.println(configMAX_PRIORITIES);
  Serial.print(F("configMINIMAL_STACK_SIZE=")); Serial.println(configMINIMAL_STACK_SIZE);

  pinMode(modeButtonPin,INPUT);
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


  xTaskCreate(TaskFadeOneByOne,"FBO", FaderStackSize, NULL,0, &majorTaskHandler);
  xTaskCreate(TaskModeSwitch,"SWC", FaderStackSize, NULL, 3, NULL);  
  // TODO: Create a task to detect the press of a button, to change the effect taking the next effect in a list
  
  if(DEBUG_MODE){
    xTaskCreate(TaskSystemStatus
      // TaskDebugButton // TaskSystemStatus
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

//////////////  Support functions

/** 8bit resolution potentiometer read.
 */
inline uint8_t readPot(){
  return analogRead(potPin) / 4;
}

// Bug it is not working right now
inline boolean buttonPressed(){
  if(digitalRead(modeButtonPin) == HIGH) {
    return true;
  }else{
    return false;
  }
}

/*--------------------------------------------------*/
/*------------------ Tasks Organization ------------*/
/*--------------------------------------------------*/


// Mode switch:
enum BoatMode: uint8_t { CyclePot=0, Fading=1, FadeOneByOne=2};

struct TaskInfo {
  const char * const name;
  TaskFunction_t taskFunction;
};

/**
 * This constant structure can map every Mode to the Function
 */
const TaskInfo Mode2Task[]= {
  {"CP", TaskFadeSyncro},
  {"F",   TaskFadeCycle},
  {"1By1", TaskFadeOneByOne}
};

BoatMode CurrentMode=FadeOneByOne;

/** This is an high priority task. We try to detect a button press every 200 milliseconds, because human are slow.
 * We NEED to delay to give a chance to all the system to run.
 */
void TaskModeSwitch(void *pvParameters){
  Serial.println(F("Task Mode Switch Ready"));
  for(;;){  
    delay(200);
    if(buttonPressed()){

      vTaskDelete(majorTaskHandler);
      Serial.print(F("From ")); Serial.print(Mode2Task[CurrentMode].name);
      Serial.print(F(" to "));
      // Increment mode
      if(CurrentMode==FadeOneByOne){
        CurrentMode=CyclePot;
      }else{
        CurrentMode=CurrentMode+1;
      }
      Serial.println( Mode2Task[CurrentMode].name );
      xTaskCreate( Mode2Task[CurrentMode].taskFunction,  Mode2Task[CurrentMode].name , FaderStackSize, NULL, 0, &majorTaskHandler);
      delay(100);
      
    }
  }
}

/*--------------------------------------------------*/
/*------------------ Led Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskSystemStatus(void *pvParameters){
  (void) pvParameters;
  for(;;){
    // The list printing is a bit bugged, because these handlers are pointers
    // and if a task is destroyed the pointer could be invalidated
    TaskHandle_t* listOfHandler2Monitor[]={
      &majorTaskHandler     
    };
    Serial.println(F(__FILE__));
    Serial.println(F("======== Tasks status ========"));
    Serial.print(F("Tick count: "));
    Serial.print(xTaskGetTickCount());
    Serial.print(F(", Task count: "));
    Serial.println(uxTaskGetNumberOfTasks());


    // Print Mode
    switch(CurrentMode){
      case CyclePot:
        Serial.println(F("Cycle Ball Mode"));
        break;
      case Fading:
        Serial.println(F("Fading Mode"));
    }


    Serial.print(F(" POT:"));
    Serial.println(readPot());

    // Lower the Mark, more probable a overflow
    Serial.println(F("== High Watermarks =="));    
  
    for(auto handler: listOfHandler2Monitor){
      Serial.print(pcTaskGetName(*handler)); Serial.print("-> ");
      Serial.println(uxTaskGetStackHighWaterMark(*handler));
    }

    Serial.println();
    Serial.println();
    
    vTaskDelay( (15* 1000)/  portTICK_PERIOD_MS);
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


void TaskFadeOneByOne(void *pvParmeters){

  const int FADE_AMOUNT=7;
  for(auto l: OrderedLeds){ pinMode(l,OUTPUT); analogWrite(l,0);}

  for(;;){
    for(auto ledx: OrderedLeds ){
      analogWrite(ledx, 0);
      for(int fadeAmount=5; fadeAmount < 255; fadeAmount+=FADE_AMOUNT){
        analogWrite(ledx,fadeAmount);
        vTaskDelay(2);
      }
      // Now turn off in turn
      for(int fadeAmount=255; fadeAmount > 0; fadeAmount-=FADE_AMOUNT){
        analogWrite(ledx,fadeAmount);
        vTaskDelay(2);
      }      
      analogWrite(ledx, 0);
    }
    

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
