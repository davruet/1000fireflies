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

#define PERIOD 1600000 // Duration in microseconds of the blink cycle
#define HALF_PERIOD 800000 
#define LED_DURATION 60000 // Duration in microseconds of the LED blink
#define PHASE_SHIFT_FACTOR 4 // Strength coefficient of adjustment made when receiving other signals

int payload = 5;


unsigned long ledTriggerTime = 0;
boolean ledOn = false;




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
  return rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof payload;  //-disable CRC and len check - noise is ok.
  //return false;
}

 boolean broadcast(){
    boolean canSend = rf12_canSend();
    if (canSend) rf12_sendStart(0, &payload, sizeof payload); 
    return canSend;
}


void loop() {
  
  checkPowerToggle();
  
  if (listen()) {
    //p("pulse %i avg %i val %i\n ", irDiff, irFilteredValue, val);
    long timediff = micros() - ledTriggerTime;
    long adjustment;
    if (timediff < HALF_PERIOD){ // we are too early. 
      adjustment = timediff / PHASE_SHIFT_FACTOR;  
      //p("early %li +- %li\n", timediff, adjustment);
     } else { // we are too late
      adjustment = (timediff - PERIOD) / PHASE_SHIFT_FACTOR;
      //p("late %li +- %li\n", timediff, adjustment);
    }
    ledTriggerTime += adjustment;
    
  }
  
  unsigned long phase = getPhase();
  
  if (!ledOn && phase > PERIOD){
    ledOn = true;
    analogWrite(LED_PIN, 255);
    
    
    
    ledTriggerTime = micros();
    if (!broadcast()) ledTriggerTime += LED_DURATION / 4; // Transmit a radio beacon.
     phase = getPhase();
  } 

  if (ledOn && phase <= LED_DURATION){
   // p("maintaining led");
    digitalWrite(LED_PIN, HIGH);
       //analogWrite(LED_PIN, (LED_DURATION - phase) * 255 / LED_DURATION);
  } else if (ledOn && phase > LED_DURATION){

    ledOn = false;
    //analogWrite(LED_PIN, 0);
    digitalWrite(LED_PIN, LOW);
  }

  
  //delay(4);
  
  
}



ISR(PCINT0_vect) {} //no-op interrupt




