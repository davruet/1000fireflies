/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */  
 
#include <Ports.h>
#include <RF12.h>

/* For ATTINY */


int ledPin = 3;
/*int irXmitPin = 1;
int irReceivePin = 3;*/

/* For ARDUINO */

//int ledPin = 9;

long period = 1600000;
long halfPeriod = period / 2;
unsigned long ledDuration = 600000;
unsigned long irDuration = 50000;

int payload = 5;


unsigned long ledTriggerTime = 0;
boolean ledOn = false;

int phaseShiftFactor = 4;
int irRecoverTime = 400;

/*
void p(char *fmt, ... ){
        char tmp[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(tmp, 128, fmt, args);
        va_end (args);
        //Serial.print(tmp);
}
  */  


void setup() {                
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(ledPin, OUTPUT);  

  rf12_initialize(4, RF12_433MHZ, 20);
  /*
  for (int i = 1; i <= 5; i++){
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);  
  }*/
  // troubleshooting
 // Serial.begin(19200);
}

unsigned long getPhase(){
    unsigned long now = micros();
    if (now < ledTriggerTime) return 0;
    unsigned long phase = now - ledTriggerTime;
    return phase;
}

boolean listen(){
  return rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof payload;  
  //return false;
}

 void broadcast(){

  if (rf12_canSend()){

    rf12_sendStart(0, &payload, sizeof payload, 2); 
  }
}


void loop() {
  
  if (listen()) {
    //p("pulse %i avg %i val %i\n ", irDiff, irFilteredValue, val);
    long timediff = micros() - ledTriggerTime;
    long adjustment;
    if (timediff < halfPeriod){ // we are too early. 
      adjustment = timediff / phaseShiftFactor;  
      //p("early %li +- %li\n", timediff, adjustment);
     } else { // we are too late
      adjustment = (timediff - period) / phaseShiftFactor;
      //p("late %li +- %li\n", timediff, adjustment);
    }
    ledTriggerTime += adjustment;
    
  }
  
  unsigned long phase = getPhase();
  // a bit of a hack, but phase might have had an overflow, if so, just use zero.
  if (!ledOn && phase > period){
    ledOn = true;
    analogWrite(ledPin, 255);
    
    broadcast();
    
    ledTriggerTime = micros();

     phase = getPhase();
  } 

  if (ledOn && phase <= ledDuration){
   // p("maintaining led");
    //digitalWrite(ledPin, HIGH);
       analogWrite(ledPin, (ledDuration - phase) * 255 / ledDuration);
  } else if (ledOn && phase > ledDuration){

    ledOn = false;
    analogWrite(ledPin, 0);
    //digitalWrite(ledPin, LOW);
  }

  
  //delay(4);
  
  
}



