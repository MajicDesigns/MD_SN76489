// MD_SN74689 Library example program.
//
// Tests different ADSR parameters playing notes.
// Input ADSR parameters on the serial monitor and test their effect.
//
// Library Dependecies
// MusicTable library located at https://github.com/MajicDesigns/MD_MusicTable
//

#include <MD_SN76489.h>
#include <MD_MusicTable.h>

// Hardware Definitions ---------------
// All the pins connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin
const uint8_t TEST_CHAN = 0;  // Channel being exercised

// Miscellaneous
const uint8_t RCV_BUF_SIZE = 50;      // UI character buffer size
const uint16_t MIN_PLAY = 300;        // minimum note time in ms
const uint16_t Vs_PLAY = 200;         // Vs playing time in ms
void(*hwReset) (void) = 0;            // declare reset function @ address 0

// Global Data ------------------------
MD_SN76489 S(D_PIN, WE_PIN, true);
MD_MusicTable T;

char rcvBuf[RCV_BUF_SIZE];  // buffer for characters received from the console

uint16_t timePlay = 1000;   // note playing time in ms
uint8_t midiNote = 60;      // MIDI note to play

MD_SN76489::adsrEnvelope_t adsr = 
{
  false,  // invert
  MD_SN76489::VOL_MAX,   // Vmax
  MD_SN76489::VOL_MAX-3, // Vs
  20,    // Ta
  30,    // Td
  50     // Tr
};

// Code -------------------------------
void help(void)
{
  Serial.print(F("\n"));
  Serial.print(F("\nIb\tSet Invert ADSR (invert b=1, noninvert b=0)"));
  Serial.print(F("\nAt\tSet Attack time to t ms"));
  Serial.print(F("\nMv\tSet Volume Max level to v volume [0..15]"));
  Serial.print(F("\nDt\tSet Decay time to t ms"));
  Serial.print(F("\nSv\tSet Sustain level to v volume [0..15]"));
  Serial.print(F("\nRt\tSet Release time to t ms"));
  Serial.print(F("\n"));
  Serial.print(F("\nNm\tSet MIDI note m as test note"));
  Serial.print(F("\n"));
  Serial.print(F("\nP\tPlay the test MIDI note"));
  Serial.print(F("\nC\tShow current ADSR parameters"));
  Serial.print(F("\nZ\tSoftware reset"));
  Serial.print(F("\nH,?\tShow this help text"));
  Serial.print(F("\n"));
}

void showADSR(void)
{
  Serial.print(F("\n\n"));
  Serial.print(F("\nInv:  "));  Serial.print(adsr.invert);
  Serial.print(F("\nTa:   "));  Serial.print(adsr.Ta);
  Serial.print(F("\nVmax: "));  Serial.print(adsr.Vmax);
  Serial.print(F("\nTd:   "));  Serial.print(adsr.Td);
  Serial.print(F("\nVs:   "));  Serial.print(adsr.Vs);
  Serial.print(F("\nTr:   "));  Serial.print(adsr.Tr);
}

bool recvLine(void) 
{
  const char endMarker = '\n'; // end of the Serial input line
  static byte ndx = 0;
  char c;
  bool b = false;        // true when we have a complete line

  while (Serial.available() && !b) // process all available characters before eoln
  {
    c = Serial.read();
    if (c != endMarker) // is the character not the end of the string terminator?
    {
      if (!isspace(c))  // filter out all the whitespace
      {
        rcvBuf[ndx] = toupper(c); // save the character
        ndx++;
        if (ndx >= RCV_BUF_SIZE) // handle potential buffer overflow
          ndx--;
        rcvBuf[ndx] = '\0';       // always maintain a valid string
      }
    }
    else
    {
      ndx = 0;          // reset buffer to receive the next line
      b = true;         // return this flag
    }
  }
  return(b);
}

uint16_t getNum(uint16_t &idx)
{
  uint16_t n = 0;
  
  while (rcvBuf[idx] >= '0' && rcvBuf[idx] <= '9')
  {
    n = (n * 10) + (rcvBuf[idx] - '0');
    idx++;
  }
  
  if (rcvBuf[idx] != '\0') idx--;
  
  return(n);
}

void processUI(void)
{
  if (!recvLine())
    return;

  // we have a line to process
  uint16_t idx = 0;
  while (rcvBuf[idx] != '\0')
  {
    switch (rcvBuf[idx++])
    {
    case 'H':   // help text
    case '?':
      help();
      break;

    case 'Z':   // resets
      hwReset();
      break;
      
    case 'P':   // Play note
      timePlay = (adsr.Ta + adsr.Td + adsr.Tr) + Vs_PLAY;
      if (timePlay < MIN_PLAY) timePlay = MIN_PLAY;
      if (T.findId(midiNote));
      {
        uint16_t f = (uint16_t)(T.getFrequency() + 0.5);  // round it up
        char buf[10];

        Serial.print(F("\n>Play "));
        Serial.print(midiNote);
        Serial.print(" (");
        Serial.print(T.getName(buf, sizeof(buf)));
        Serial.print(F(") @ "));
        Serial.print(f);
        Serial.print(F("Hz"));
        S.note(TEST_CHAN, f, timePlay);
      }
      break;

    case 'C':   // Show ADSR
      showADSR();
      break;

    case 'N':   // MIDI test note
      midiNote = getNum(idx);
      Serial.print("\n>Note ");
      Serial.print(midiNote);
      break;

    case 'I':   // Invert true/false
      adsr.invert = (getNum(idx) != 0);
      Serial.print("\n>Invert ");
      Serial.print(adsr.invert);
      break;
      
    case 'A':   // Attack ms
      adsr.Ta = getNum(idx);
      Serial.print("\n>Ta ");
      Serial.print(adsr.Ta);
      break;

    case 'M':   // Attack Vmax
      adsr.Vmax = getNum(idx);
      Serial.print("\n>Vmax ");
      Serial.print(adsr.Vmax);
      break;

    case 'D':   // Decay ms
      adsr.Td = getNum(idx);
      Serial.print("\n>Td ");
      Serial.print(adsr.Td);
      break;

    case 'S':   // Sustain Vs
      adsr.Vs = getNum(idx);
      Serial.print("\n>Vs ");
      Serial.print(adsr.Vs);
      break;

    case 'R':   // Release ms
      adsr.Tr = getNum(idx);
      Serial.print("\n>Tr ");
      Serial.print(adsr.Tr);
      break;
    }
  }
}

void setup(void)
{
  Serial.begin(57600);
  S.begin();
  S.setADSR(TEST_CHAN, &adsr);
  
  Serial.print(F("\n[MD_SN76489 ADSR Envelope]"));
  Serial.print(F("\nEnsure serial monitor line ending is set to newline."));
  help();
}

void loop(void)
{
  S.play();     // run the sound machine every time through loop()
  processUI();  // check the user input
}