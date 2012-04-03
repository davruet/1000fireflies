/*
 * Pin Change Interrupt example - toggle power down mode.
 * Includes software switch debouncing.
 */
#include <avr/sleep.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define SWITCH_PIN 8

/* LED defines */
#define LED_PIN 3
#define BLINK_INTERVAL 100

/* RF12 stuff */
#include <JeeLib.h>
#include <Ports.h>

#define RF12_SLEEP 0
#define RF12_WAKEUP -1

void setup(){
  pinMode(SWITCH_PIN,INPUT);
  digitalWrite(SWITCH_PIN, HIGH);
  sbi(GIMSK,PCIE0); // Turn on Pin Change interrupt
  sbi(PCMSK0,PCINT2); // Which pins are affected by the interrupt
  //sbi(MCUCR, ISC00);
  
  pinMode(LED_PIN,OUTPUT);
}

unsigned long lastBlink = 0;
byte ledState = HIGH;

unsigned int buttonState = 0;
boolean waitingForSleep = false;
boolean buttonIsDown = false;
boolean shutdownEnabled = false;


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

void loop(){
  
  if (millis() - lastBlink > BLINK_INTERVAL){   
    lastBlink = millis(); 
    digitalWrite(LED_PIN,ledState);
   
    if (ledState == HIGH) ledState = LOW;
    else ledState = HIGH;
  }
  
  checkPowerToggle();
 
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


