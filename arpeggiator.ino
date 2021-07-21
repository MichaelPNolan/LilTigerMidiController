/*
 * I want some arpeggios I watched some project and their arpeggio idea was irrelevant. We have midi - just need to have intervals
 * So we have Arpeggio On and Off per note and we have minor and major (for a start)
 */
#define arpLength 5 //aka 0 .. 4

#define UDPATTERN 0  //Arpeggio Pattern codes for use in struct arp, patternType
#define PATTERNSARRAY1 1

#define CHORD_M 0  //chordType major
#define CHORD_min 1  //chordType minor
#define DEFAULTNOTELENGTH 100 //in milliseconds
#define MAXARPNOTES 5 //if this many notes are used notes will get stopped

uint8_t notesPlayingArp[MAXARPNOTES];
unsigned long int notesPlayingArpEnd[MAXARPNOTES];  //set a timer in mills and notes expire and get turned off
uint8_t noteArpIndex = 0;
static int notesArpOpen = 0;

uint8_t majorArp[arpLength] = {0,4,7,11,16}; //root 3rd 5th 7th 10th semitones or notes above note starting or stopping it
uint8_t minorArp[arpLength] = {0,3,7,10,15}; // root 3rd 5th 7th 10th

struct Chord{
uint8_t numberOfNotes;
int  chord[12]; };//up to 12 notes and they can be negative so you could have other octaves like -12,0,12,7,19 - which is only 2 notes but 3 octaves of root and 2 of 5th

bool majorOrMinor[12] = {0,1,1,1,1,0,1,0,1,1,1,1};               // relative to whatever is key what type of arp would be most appropriate 0 is major, 1 is minor expand later
uint8_t arpPatternUpDown[8] = {0,1,2,3,4,3,2,1};   // UDPATTERN I guess we can have a variety of patterns to use later start with just this eg reference the majorArp[x] 
uint8_t arpPatterns[8][8] = { //assumes a 5 length
  {0,1,2,3,4,3,2,1},
  {0,2,1,3,2,4,3,0},
  {0,3,1,4,2,0,3,1},
  {0,4,1,0,2,1,3,2},
  {0,1,2,1,2,3,2,3},
  {4,3,4,0,4,0,1,0},
  {2,1,2,3,2,3,4,0},
  {3,1,4,0,2,2,4,1}   };
  
bool arpStatus = LOW;

struct Arp{
uint8_t root;  // where to extend to play arpeggio from (if we are too high just play high notes back an octave)
uint8_t seq;   //where in the pattern index are we
uint8_t vol;  
uint8_t chordType;
uint8_t patternType;
uint8_t patternSeq; //refers to the second dimention of the pattern 
uint16_t noteLen;
uint16_t numberOfplays;
};

static Arp arp;
void startArpMajor(uint8_t root, uint8_t vol, uint16_t len){
  #ifdef OCTAVE_DOWN
  arp.root = root-12;
  #else
  arp.root = root;
  #endif
  
  arp.seq = 0;
  arp.vol = vol;
  arp.chordType = CHORD_M;
  arp.patternType = UDPATTERN;
  arp.patternSeq = 0;
  if(len == 0)
    arp.noteLen = DEFAULTNOTELENGTH;
  else 
    arp.noteLen = len;
  arp.numberOfplays = 0;
  arpStatus = HIGH; //will play on high
}

void startArpMinor(uint8_t root, uint8_t vol, uint16_t len){
  startArpMajor(root,vol,len);
  arp.chordType = CHORD_min;
}

void startArpRelative(uint8_t root, uint8_t vol, uint16_t len, uint8_t key){
  if(majorOrMinor[(key+root)%12])
    startArpMinor(root,vol,len);
  else
    startArpMajor(root,vol,len);
 // Serial.println();
 // Serial.println(" offset="+String((key+root)%12)+" "+String(majorOrMinor[12]));
}

void changeArpPattern(uint8_t patternCode)
{
  arp.patternType = patternCode;
  arp.patternSeq = 0;
}

void checkforExpiredNotes(){
  if(notesArpOpen > 0){  // don't bother processing notes that need to be turned off if none are open
    for(int i=0; i<MAXARPNOTES; i++)
    {
      if(notesPlayingArp[i] != 0)
        if(notesPlayingArpEnd[i] < millis()){ //if the note is at its end time zero the arrays related to notesPlayingArp and their endtime 
          notesPlayingArpEnd[i] = 0;
          noteOff(0, notesPlayingArp[i], 0);
          //Serial.println(String(notesPlayingArp[i])+"-off / "+String(notesArpOpen));
          notesPlayingArp[i] = 0;
          decrementNotesArpOpen();
          MidiUSB.flush();
          }
       }
    }
    
}

void incrementNoteArpIndex(){
  noteArpIndex++;
  if(noteArpIndex >= MAXARPNOTES)
    noteArpIndex = 0;  //start back at first note
}

void decrementNotesArpOpen(){
 notesArpOpen=notesArpOpen-1;
  if(notesArpOpen < 0){
    notesArpOpen = 0;
     //Serial.print("notesArpOpen calculation error");
  }
}

void incrementNoteSeq(){
  arp.seq++;
  if(arp.seq > 7){
    arp.seq = 0;
    Serial.println(); //"size: "+String(sizeof(majorArp))
  }
}

//the timing for note starts (process) will be in the loop that calls this - processing will play the next note and look for notes that expired to stop
void processArp(){ //this can get called endlessly and it will just either not play or play according to arpStatus
  
  if(arpStatus){  //ok to play the current arp setting
    unsigned long int timer = millis();
    uint8_t newNote;
    switch(arp.chordType){ //adjust for the arpeggio (the whole point based on the type of chord
       case CHORD_M :
         newNote = arp.root+majorArp[arpPatternUpDown[arp.seq]];
         break;
       case CHORD_min :
         newNote = arp.root+minorArp[arpPatternUpDown[arp.seq]];
         break;
       default: 
         newNote = arp.root+majorArp[arpPatternUpDown[arp.seq]];
    }
   
    

    //doesn't matter if its the same note ...
    if(notesPlayingArp[noteArpIndex] != 0){ //if a note is there still playing then note off and reduce notesArpOpen
      noteOff(0, notesPlayingArp[noteArpIndex], 0);
      //MidiUSB.flush();
      decrementNotesArpOpen();
      //Serial.println(String(notesPlayingArp[noteArpIndex]+"off-taken /"));
    }
    notesPlayingArp[noteArpIndex] = newNote;
    noteOn(0, newNote, arp.vol);
    Serial.print(String(notesPlayingArp[noteArpIndex])+"on ");
    //if(arp.chordType == CHORD_M) Serial.print("Maj/"); else Serial.print("Min/");
    notesPlayingArpEnd[noteArpIndex] = timer+arp.noteLen;
    //prepare for the next note
    incrementNoteArpIndex();
    notesArpOpen=notesArpOpen+1; 
    incrementNoteSeq(); 
    MidiUSB.flush();
  
  }
}

void stopArp(){
  arpStatus = LOW;
 

}
