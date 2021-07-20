//4 pins 3 out and 1 in for selecting channel 3bit and reading the analogue common pin of 4051 - up to 8 channels for one analogue pin
#define MUXCOMMON   A2 //aka 20 the output after selecting the channel of the 4051 analogue MUX
#define CHANNELSELECT_A  21
#define CHANNELSELECT_B  0
#define CHANNELSELECT_C  1
// from marcel adc related code I'm playing with

static uint8_t adcChannelValue[8][3]; //y = 0 the currently sample value    
                             //y = 1 the previous sampled value
                             //y = 0 means unitialized
int  nextChannel;                            
struct adc_to_midi_s
{
    uint8_t ch;
    uint8_t cc;
};

struct adc_to_midi_s adcToMidiLookUp[8] =
{
    {0, 0x10},
    {1, 0x10},
    {2, 0x10},
    {3, 0x10},
    {4, 0x10},
    {5, 0x10},
    {6, 0x10},
    {7, 0x10},
};


void AdcMul_Init(void)
{
  for (int i = 0; i < 8; i++)
  {
      adcChannelValue[i][2] = 0; //i didn't end up using average - i need to read and send out midi after start up
  }

  pinMode(MUXCOMMON, INPUT);

  pinMode(CHANNELSELECT_A, OUTPUT);
  digitalWrite(CHANNELSELECT_A, LOW);
  pinMode(CHANNELSELECT_B, OUTPUT);
  digitalWrite(CHANNELSELECT_B, LOW);
  pinMode(CHANNELSELECT_C, OUTPUT);
  digitalWrite(CHANNELSELECT_C, LOW);
  nextChannel = 0;
  setMuxChannel(nextChannel);
}

void setMuxChannel(uint8_t channel){
    digitalWrite(CHANNELSELECT_A, ((channel & (1 << 0)) > 0) ? HIGH : LOW); 
    digitalWrite(CHANNELSELECT_B, ((channel & (1 << 1)) > 0) ? HIGH : LOW);
    digitalWrite(CHANNELSELECT_C, ((channel & (1 << 2)) > 0) ? HIGH : LOW);

}

void rotateChannel()  //because I prefer to rotate once per call than waste cpu power on for 1-8 and delay
{
  nextChannel++;
  if(nextChannel > 7) nextChannel=0;
}

void AdcMul_Process(void)
{
    int readPot;
    bool midiMsg = false;
               //channel pins a,b,c of 4051
    
            /* give some time for transition */
    //delay(1);
    
    
    readPot= analogRead(MUXCOMMON)>>3;
    //the wiring is messed up and 7 and 5 are reversed so switch and back before increment to nextChannel 
    if(nextChannel == 5) nextChannel = 7;
    else if (nextChannel ==7) nextChannel = 5;
    //endkudge pt1
    adcChannelValue[nextChannel][0] = readPot;
    
    if(adcChannelValue[nextChannel][0] != adcChannelValue[nextChannel][1]){ //change in value  adcChannelValue[nextChannel][0] != adcChannelValue[nextChannel][1]
      if(true){ //nextChannel == 0 || nextChannel == 4
        //Serial.print(adcChannelValue[nextChannel][0]); Serial.print(" ");
        Serial.print(nextChannel);
     
        Serial.print("-rval-");
        Serial.println(adcChannelValue[nextChannel][0] );
        midiMsg = true;
      }
        adcChannelValue[nextChannel][1] = adcChannelValue[nextChannel][0];
      
    }
    if(adcChannelValue[nextChannel][2] == 0){
      midiMsg = true;
      adcChannelValue[nextChannel][1] = adcChannelValue[nextChannel][0];
      adcChannelValue[nextChannel][2] = 1;
      Serial.print("init"+String(nextChannel)+"/ ");
    }
    
    // MIDI adoption 
    if (midiMsg )
    {
        uint8_t midiValueU7 = adcChannelValue[nextChannel][0];
        if(midiValueU7 > 127) midiValueU7 = 127; //constrain
        if (nextChannel < 8)
        {
          controlChange(adcToMidiLookUp[nextChannel].ch, adcToMidiLookUp[nextChannel].cc, midiValueU7);
          MidiUSB.flush();
        }
        

    } 
    //undo the switch from above due to reversed wiring
    if(nextChannel == 7) nextChannel = 5;
    else if (nextChannel ==5) nextChannel = 7;
    //end kludge pt2
    rotateChannel();
    setMuxChannel(nextChannel); //set the pins to read next cycle
}
