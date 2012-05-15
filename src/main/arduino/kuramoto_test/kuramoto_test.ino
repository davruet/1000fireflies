/*

Test code for The Kuramoto Model hardware. Blinks the LED at startup, to test the LED connections.
After that, it sends out a radio broadcast once per second, and blinks at the same time. Each 
time it hears another radio broadcast, it also blinks. If it cannot send, it holds the LED on
for a long period of time. 

*/
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <JeeLib.h>

#define LED_PIN 3
#define SWITCH_PIN 8

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))  
#endif

MilliTimer ledTimer;
MilliTimer broadcastTimer;

int payload = 5;

//power button variables
unsigned int buttonState = 0;
boolean waitingForSleep = false;
boolean buttonIsDown = false;
boolean shutdownEnabled = false;

/** Gets the raw (un-debounced) value from the button.
Returns 1 if button is pressed, 0 otherwise. **/
unsigned int getRawButtonValue(){
  return digitalRead( SWITCH_PIN) == LOW;
}

/** Non-standard things to do before sleeping (e.g. shut off peripherals, etc.)
*/
void getReadyForBed(){
  waitingForSleep = false;
  rf12_sleep(RF12_SLEEP);
  digitalWrite(LED_PIN, LOW);
  MCUSR = 0;
  wdt_disable();
}

/** Non-standard things to do after awakening (e.g. turn on peripherals, etc.)
*/
void startTheDay(){
  
  reboot(); // do a reset
  //rf12_sleep(RF12_WAKEUP);

  //digitalWrite(SWITCH_PIN, HIGH);
  //digitalWrite(LED_PIN, HIGH);
}

void reboot(){
  MCUSR = 0;
  wdt_enable(WDTO_15MS);
  for (;;){} // INFINNITE LOOP, causes watchdog restart.
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
  shutdownEnabled = shutdownEnabled | (buttonState == 0); 
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




void setup() {
      MCUSR = 0;
      wdt_enable(WDTO_8S); // enable the watchdog - for safety!
       pinMode(SWITCH_PIN,INPUT);
      digitalWrite(SWITCH_PIN, HIGH);
      pinMode(LED_PIN,OUTPUT);
      for (int i = 0; i < 10; i++){
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
        delay(50);
      }
      digitalWrite(LED_PIN, LOW);
      
      broadcastTimer.set(1000);
      
       // power stuff
      sbi(GIMSK,PCIE0); // Enable PCI
      sbi(PCMSK0,PCINT2); // Set PCInt2 as the PCI pin  
      
       rf12_initialize(4, RF12_433MHZ, 20);
        rf12_control(0xC040); // low battery detect set to 2.1
     
}

void loop() {
   checkPowerToggle();
    if (ledTimer.poll()){
      digitalWrite(LED_PIN, LOW);  
    }
    
    if (rf12_recvDone()) {// && rf12_crc == 0 && rf12_len == sizeof payload;    
      ledTimer.set(50);  
      digitalWrite(LED_PIN, HIGH);
    }
    

    if (broadcastTimer.poll()){
      ledTimer.set(50);
      digitalWrite(LED_PIN, HIGH);
      broadcastTimer.set(1000);
      
      if (rf12_canSend()){
        rf12_sendStart(0, &payload, sizeof payload); 
        wdt_reset();
        
      } else {
          ledTimer.set(1000);
      }
    }
    
    //delay(5);

}

ISR(PCINT0_vect) {} //no-op interrupt


