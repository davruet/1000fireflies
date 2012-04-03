/*
  THE KURAMOTO MODEL
  
*/
 
#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>



#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


#define LED_PIN 3
#define SWITCH_PIN 8

#define RF12_SLEEP 0
#define RF12_WAKEUP -1

long period = 1600000;
long halfPeriod = period / 2;
unsigned long ledDuration = 600000;
unsigned long irDuration = 50000;

int payload = 5;


unsigned long ledTriggerTime = 0;
boolean ledOn = false;

int phaseShiftFactor = 4;
int irRecoverTime = 400;

//power button variables
unsigned int buttonState = 0;
boolean waitingForSleep = false;
boolean buttonIsDown = false;
boolean shutdownEnabled = false;


void setup() {            

  pinMode(SWITCH_PIN,INPUT);
  digitalWrite(SWITCH_PIN, HIGH);
  sbi(GIMSK,PCIE0); // Enable PCI
  sbi(PCMSK0,PCINT2); // Set PCInt2 as the PCI pin  

  pinMode(LED_PIN, OUTPUT);  

  rf12_initialize(4, RF12_433MHZ, 20);

}

void checkPowerToggle(){
  unsigned int currentValue = getRawButtonValue();
  buttonState <<= 1;
  buttonState |= currentValue;
  
  buttonIsDown = (buttonState == 0xffff);
  shutdownEnabled = shutdownEnabled | buttonState == 0; 
  if (buttonIsDown && shutdownEnabled){
    waitingForSleep = true;
    
  } else {
    
    if (waitingForSleep){
      shutdownEnabled = false;
      waitingForSleep = false;
      goToSleep();
      
      shutdownEnabled = false;
      waitingForSleep = false;
    } 
  }  
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
  
  checkPowerToggle();
  
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
    analogWrite(LED_PIN, 255);
    
    broadcast();
    
    ledTriggerTime = micros();

     phase = getPhase();
  } 

  if (ledOn && phase <= ledDuration){
   // p("maintaining led");
    //digitalWrite(LED_PIN, HIGH);
       analogWrite(LED_PIN, (ledDuration - phase) * 255 / ledDuration);
  } else if (ledOn && phase > ledDuration){

    ledOn = false;
    analogWrite(LED_PIN, 0);
    //digitalWrite(LED_PIN, LOW);
  }

  
  //delay(4);
  
  
}

/** Gets the raw (un-debounced) value from the button.
Returns 1 if button is pressed, 0 otherwise. */
unsigned int getRawButtonValue(){
  return digitalRead( SWITCH_PIN) == LOW;
}

/** Non-standard things to do before sleeping (e.g. shut off peripherals, etc.)
*/
void getReadyForBed(){
  waitingForSleep = false;
  rf12_sleep(RF12_SLEEP);
  digitalWrite(LED_PIN, LOW);
}

/** Non-standard things to do after awakening (e.g. turn on peripherals, etc.)
*/
void startTheDay(){
  rf12_sleep(RF12_WAKEUP);
  //digitalWrite(SWITCH_PIN, HIGH);
  //digitalWrite(LED_PIN, HIGH);
}


void goToSleep() {
  getReadyForBed();
  cbi(ADCSRA,ADEN); // Turn off ADC

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Go to sleep
  sleep_mode(); // Sleep starts after this line
  //sleep_cpu();

  sbi(ADCSRA,ADEN);  // Turn on ADC
  startTheDay();
}

ISR(PCINT0_vect) {} //no-op interrupt




