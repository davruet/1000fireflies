/*
  THE KURAMOTO MODEL
  
*/
 
#include <Ports.h>
#include <RF12.h>
#include <avr/sleep.h>
#include <avr/wdt.h>


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

#define SYNC_PERIOD 1000000 // Duration in microseconds of the blink cycle

#define ALONE_PERIOD 300000
//#define HALF_PERIOD 500000 
#define LED_DURATION 70000 // Duration in microseconds of the LED blink
#define PHASE_SHIFT_FACTOR 4 // Strength coefficient of adjustment made when receiving other signals

#define SEND_FAIL_LIMIT 20

int payload = 5;


unsigned long ledTriggerTime = 0;
unsigned long lastReportTime = 0;
boolean ledOn = false;

int consecutiveSendFailures = 0;

//unsigned long period = SYNC_PERIOD;
#define period 1000000



//power button variables
unsigned int buttonState = 0;
boolean waitingForSleep = false;
boolean buttonIsDown = false;
boolean shutdownEnabled = false;

void startupBlinks(){
  for (int i = 0; i < 10; i++){
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    digitalWrite(LED_PIN, LOW);
    delay(50);
  }  
}
void setup() {     
  MCUSR = 0;
  wdt_enable(WDTO_8S); // enable the watchdog - for safety!
  pinMode(SWITCH_PIN,INPUT);
  digitalWrite(SWITCH_PIN, HIGH);
  sbi(GIMSK,PCIE0); // Enable PCI
  sbi(PCMSK0,PCINT2); // Set PCInt2 as the PCI pin  

  pinMode(LED_PIN, OUTPUT);  
  


  rf12_initialize(4, RF12_433MHZ, 20);
  
  startupBlinks();
  ledTriggerTime = micros();

}

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
  boolean received = rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof payload;  //-disable CRC and len check - noise is ok.
  if (received) lastReportTime = micros();
  return received;
  //return rf12_recvDone();
  //return false;
}

 boolean broadcast(){
    boolean canSend = rf12_canSend();
    if (canSend){
      rf12_sendStart(0, &payload, sizeof payload);
      consecutiveSendFailures = 0;
    }
    else {
      consecutiveSendFailures++;  
    }
    return canSend;
}


void loop() {
  checkPowerToggle();
  
   /*if (micros() - lastReportTime > 5000000) period = ALONE_PERIOD;
   else period = SYNC_PERIOD;
  */
  if (listen()) {
    //p("pulse %i avg %i val %i\n ", irDiff, irFilteredValue, val);
    long timediff = micros() - ledTriggerTime;
    long adjustment;
    if (timediff < period / 2){ // we are too early. 
      adjustment = timediff / PHASE_SHIFT_FACTOR;  
      //p("early %li +- %li\n", timediff, adjustment);
     } else { // we are too late
      adjustment = (timediff - period) / PHASE_SHIFT_FACTOR;
      //p("late %li +- %li\n", timediff, adjustment);
    }
    ledTriggerTime += adjustment;
    
  }
  
  unsigned long phase = getPhase();
  
  if (!ledOn && phase > period){
    if (consecutiveSendFailures < SEND_FAIL_LIMIT) wdt_reset(); // reset the watchdog only when we turn on the led and reset the timer.
    ledOn = true;
    digitalWrite(LED_PIN, HIGH);

    
    ledTriggerTime = micros();
    if (!broadcast()) ledTriggerTime += LED_DURATION / 4; // Transmit a radio beacon.
     phase = getPhase();
  } 

  if (ledOn && phase > LED_DURATION){

    ledOn = false;
    //analogWrite(LED_PIN, 0);
    digitalWrite(LED_PIN, LOW);
  }


  //delay(4);
  
  
  
}



ISR(PCINT0_vect) {} //no-op interrupt




