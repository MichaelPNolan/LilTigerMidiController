
/*
 * MIDIUSB_tiger pup
 * Hacked toy keys - wired up to Arduino Pro Micro board which gives USB over midi device recognition
 * Created: Wed Jul14 2021 -
 * Author: Michael Nolan - using USB library
 * 
 * Pins free 0,1,
 */ 

#include "MIDIUSB.h"
//#define ARP_PLAYER_MODE
#define OCTAVE_DOWN

#define LED_FLASHER 19   //this is the reset pin so maybe this is messed up

//In arpeggiator mode notes are tracked so they can be turned off because they are turned on/off auto
//I brought this rotating queue of notes idea back here for a different reason. Tracking the length of time
//since a note was triggered before it has been noteOff to trigger a mode, command or arpeggio chord
#define MAXNOTES 8 //if this many notes are used notes will get stopped

uint8_t notesPlaying[MAXNOTES];
unsigned long int notesPlayingEnd[MAXNOTES];  //set a timer in mills and notes expire and get turned off
uint8_t noteIndex = 0;
int notesOpen = 0;

// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

bool keyboardInputStates[16][2];   //first iteration i had 4 states and I wrote a really bad, slow debounce - then i copied nerd musician way
int  keboardNotes[16] = {50,49,52,51,54,53,56,55,58,57,60,59,62,61,64,63};
int  keyboardInputPins[16] = {2,-1,9,15,8,14,7,16,6,10,5,18,4,-1,3,-1};
unsigned long lastDebounceTime[16];
unsigned long int  debounceWait = 0;
unsigned long int  lastProcess = 0;
const int debounceTime = 8;
uint8_t debouncePin;
bool toggleArp = LOW;
uint8_t arpNote;

void incrementNoteIndex(){ //it only goes up and resets back to start of queue re-using
  noteIndex++;
  if(noteIndex >= MAXNOTES)
    noteIndex = 0;  //start back at first note
  Serial.println(" idx: "+String(noteIndex)+" Op:"+String(notesOpen));
}

void decrementNotesOpen(){
 notesOpen=notesOpen-1;
  if(notesOpen < 0){
    notesOpen = 0;
     //Serial.print("notesArpOpen calculation error");
  }
}

// carefull not to put stuff inside here
void noteOn(byte channel, byte pitch, byte velocity) {
  if(notesOpen < MAXNOTES){ //stop adding notes
    midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
    MidiUSB.sendMIDI(noteOn);
    for(int i=0; i<MAXNOTES; i++){
      if(notesPlaying[i] == 0){
        notesPlaying[noteIndex] = uint8_t(pitch);
        incrementNoteIndex();
        notesOpen++;
        break;
      }
    }
  }
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  for(int i=0; i<MAXNOTES; i++){
    if(notesPlaying[i] == pitch){
      notesPlaying[i] = 0;
      decrementNotesOpen();
      Serial.print(notesPlaying[i]);
      Serial.print("/");
    }
  }
}

void setup() {
  Serial.begin(115200);
  #ifdef LED_FLASHER
  pinMode(LED_FLASHER, OUTPUT);
  #endif
  //setup button reading pins
  for(int i=0; i<16; i++)
  {
      if(keyboardInputPins[i] >= 0)
         pinMode(keyboardInputPins[i], INPUT); //pins defined in initialization array above
      keyboardInputStates[i][0] = LOW; //checked state
      keyboardInputStates[i][1] = LOW; //previous state

  }
  for(int i=0; i<MAXNOTES; i++){
    notesPlaying[i] = 0;
  }
  notesOpen = 0;
  
  AdcMul_Init(); // multiplexer is 4051 chip analogue channel switching from 3 pins defined in the Multipots4051 file
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).
void readInputPins(){
  for(int i=0; i<16; i++)
  {   
      if(keyboardInputPins[i] >= 0){
         keyboardInputStates[i][1] = keyboardInputStates[i][0]; //copy last read state to previous state holder [1]
         keyboardInputStates[i][0] = digitalRead(keyboardInputPins[i]); //pins defined in initialization array above
         
      }
  }
}
//----------------------------------------
void buttons() {

  for (int i = 0; i < 16;i++) {

    keyboardInputStates[i][0] = digitalRead(keyboardInputPins[i]);
    /*
        // Comment this if you are not using pin 13...
        if (i == pin13index) {
          buttonCState[i] = !buttonCState[i]; //inverts pin 13 because it has a pull down resistor instead of a pull up
        }
        // ...until here
    */
    if ((millis() - lastDebounceTime[i]) > debounceTime) {

      if (keyboardInputStates[i][0] != keyboardInputStates[i][1]) {
        lastDebounceTime[i] = millis();

        if (keyboardInputStates[i][0]  == LOW) {
          #ifdef ARP_PLAYER_MODE
                #ifdef LED_FLASHER
                digitalWrite(LED_FLASHER, HIGH);
                #endif
                if(!toggleArp || (arpNote != keboardNotes[i])){
                  Serial.print(String(i)+":ARP_START ");
                  startArpRelative(keboardNotes[i], 80, 500,52);  //startArpRelative(uint8_t root, uint8_t vol, uint16_t len, uint8_t key)
                  arpNote = keboardNotes[i];
                  toggleArp = HIGH;
                } else {
                  Serial.println(String(i)+":ARP_STOP ");
                  stopArp();
                  arpNote = 0;
                  toggleArp = LOW;
                }
              #else
                //Serial.print(String(i)+":ON ");
                #ifdef LED_FLASHER
                digitalWrite(LED_FLASHER, HIGH);
                #endif
                noteOn(0, keboardNotes[i], 64);
                MidiUSB.flush();
                
              #endif
          
          
          
          /* original
          noteOn(0, keboardNotes[i], 64);  // channel, note, velocity
          MidiUSB.flush();
          */
          //          Serial.print("button on  >> ");
          //          Serial.println(i);
        }
        else {
          #ifdef ARP_PLAYER_MODE
              #ifdef LED_FLASHER
              digitalWrite(LED_FLASHER, LOW);
              #endif
             
          #else
            //Serial.println(String(i)+": OFF");
            #ifdef LED_FLASHER
            digitalWrite(LED_FLASHER, LOW); 
            #endif
            noteOff(0, keboardNotes[i], 0);
            MidiUSB.flush();
            
          #endif

          /*
          noteOn(0, keboardNotes[i], 0);  // channel, note, velocity
          MidiUSB.flush();
          */
          //          Serial.print("button off >> ");
          //          Serial.println(i);
        }
        keyboardInputStates[i][1] = keyboardInputStates[i][0];
      }
    }
  }
}




//-----------------------------------
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
unsigned long int timer;
bool timerSignal = 0;
int i; //for loop
void loop() {
  //delay(1);
  timer = millis();
  buttons();
  // check state changes of buttons
 /* 
  
  if(false){
    readInputPins();
    checkforExpiredNotes();
    for(i=0; i<16; i++)
    {
      if(keyboardInputStates[i][0] != keyboardInputStates[i][1]) {//if state has changed since last read
        
        if(keyboardInputStates[i][0]) //keyboardInputStates[i][0]
          if(keyboardInputStates[i][3]){
            #ifdef ARP_PLAYER_MODE
                #ifdef LED_FLASHER
                digitalWrite(LED_FLASHER, LOW);
                #endif
                keyboardInputStates[i][3] = LOW;
            #else
              //Serial.println(String(i)+": OFF");
              #ifdef LED_FLASHER
              digitalWrite(LED_FLASHER, LOW); 
              #endif
              noteOff(0, keboardNotes[i], 0);
              MidiUSB.flush();
              keyboardInputStates[i][3] = LOW;
            #endif
          }
        else {     
           keyboardInputStates[i][2] = HIGH; //candidate for debounce
           lastDebounceTime[i] = timer; //wait time to check for really on 
           
           }
       }
    }
   
  }
  // if a state changed now check if its time to confirm for debounce
  //have a feeling the hardware is debounced - some buttons need debouncing though
  
  if(false) //debounceWait > 0
    
    if(debounceWait < timer){
      readInputPins();
      debounceWait = 0;
      for(i=0; i<16; i++){
         if(keyboardInputStates[i][2]) //only check a key that has been triggered for debounce
           if(keyboardInputStates[i][0] == LOW){ //low means still pressed
              #ifdef ARP_PLAYER_MODE
                #ifdef LED_FLASHER
                digitalWrite(LED_FLASHER, HIGH);
                #endif
                if(!toggleArp || (arpNote != keboardNotes[i])){
                  Serial.print(String(i)+":ARP_START ");
                  startArpRelative(keboardNotes[i], 80, 500,52);  //startArpRelative(uint8_t root, uint8_t vol, uint16_t len, uint8_t key)
                  arpNote = keboardNotes[i];
                  toggleArp = HIGH;
                } else {
                  Serial.println(String(i)+":ARP_STOP ");
                  stopArp();
                  arpNote = 0;
                  toggleArp = LOW;
                }
              #else
                //Serial.print(String(i)+":ON ");
                #ifdef LED_FLASHER
                digitalWrite(LED_FLASHER, HIGH);
                #endif
                noteOn(0, keboardNotes[i], 64);
                MidiUSB.flush();
                
              #endif
              keyboardInputStates[i][3] = HIGH;
              keyboardInputStates[i][2] = LOW; // debounce check only once
           }
         
      }  //for
  } */
  
  if((timer - lastProcess) >200){
    processArp();
    lastProcess = timer;
  }

  if(timer%5 == 0)
  {
    AdcMul_Process();
    checkforExpiredNotes();
  }

  
}
