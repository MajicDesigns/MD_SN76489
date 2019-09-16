// MD_SN74689 Library example program.
//
// Tests different ADSR parameters playing notes.
// Input ADSR parameters on the serial monitor and test their effect.
//
// Library Dependencies
// MusicTable library located at https://github.com/MajicDesigns/MD_MusicTable
//

#include <MD_SN76489.h>
#include <MD_MusicTable.h>

// Hardware Definitions ---------------
// All the pins connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin

// Miscellaneous
const uint8_t RCV_BUF_SIZE = 50;      // UI character buffer size
void(*hwReset) (void) = 0;            // declare reset function @ address 0

// Global Data ------------------------
MD_SN76489 S(D_PIN, WE_PIN, true);
MD_MusicTable T;

char rcvBuf[RCV_BUF_SIZE];  // buffer for characters received from the console

uint16_t timePlay = 200;    // note playing time in ms
uint8_t channel = 0;        // Channel being exercised

MD_SN76489::adsrEnvelope_t adsr[MD_SN76489::MAX_CHANNELS];

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
  Serial.print(F("\nCn\tSet channel to n ([0..2] for note, 3 for noise)"));
  Serial.print(F("\nTt\tSet play time duration to t ms"));
  Serial.print(F("\nNm\tPlay MIDI Note m"));
  Serial.print(F("\nOtf\tPlay Noise sound type t [P,W] freq f [0..2]"));
  Serial.print(F("\n"));
  Serial.print(F("\nP\tShow current ADSR parameters"));
  Serial.print(F("\nZ\tSoftware reset"));
  Serial.print(F("\nH,?\tShow this help text"));
  Serial.print(F("\n"));
}

void showADSR(void)
{
  Serial.print(F("\n\nChan\tInv\tTa\tVmax\tTd\tVs\tTr\n"));
  for (uint8_t i = 0; i < MD_SN76489::MAX_CHANNELS; i++)
  {
    Serial.print(i);              Serial.print(F("\t"));
    Serial.print(adsr[i].invert); Serial.print(F("\t"));
    Serial.print(adsr[i].Ta);     Serial.print(F("\t"));
    Serial.print(adsr[i].Vmax);   Serial.print(F("\t"));
    Serial.print(adsr[i].Td);     Serial.print(F("\t"));
    Serial.print(adsr[i].Vs);     Serial.print(F("\t"));
    Serial.print(adsr[i].Tr);     Serial.print(F("\t"));
    Serial.print(F("\n"));
  }
  Serial.print(F("\n"));
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
      
    case 'N':   // Play note
      {
        uint8_t midiNote = getNum(idx);

        if (timePlay <= adsr[channel].Ta + adsr[channel].Td + adsr[channel].Tr)
          timePlay = adsr[channel].Ta + adsr[channel].Td + adsr[channel].Tr;
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
          S.note(channel, f, timePlay);
        }
      }
      break;

    case 'O': 
    {
      MD_SN76489::noiseType_t n = MD_SN76489::PERIODIC_0;

      switch (rcvBuf[idx++])
      {
        case 'P':
          switch (rcvBuf[idx++])
          {
            case '0': n = MD_SN76489::PERIODIC_0; break;
            case '1': n = MD_SN76489::PERIODIC_1; break;
            case '2': n = MD_SN76489::PERIODIC_2; break;
          }
          break;

        case 'W':
          switch (rcvBuf[idx++])
          {
            case '0': n = MD_SN76489::WHITE_0; break;
            case '1': n = MD_SN76489::WHITE_1; break;
            case '2': n = MD_SN76489::WHITE_2; break;
          }
          break;
      }

      if (timePlay <= adsr[channel].Ta + adsr[channel].Td + adsr[channel].Tr)
        timePlay = adsr[channel].Ta + adsr[channel].Td + adsr[channel].Tr;

      Serial.print(F("\n>Noise "));
      Serial.print(n);
      S.noise(n, timePlay);
    }
    break;

    case 'C':   // set channel
      {
        uint8_t n = getNum(idx);

        if (n < MD_SN76489::MAX_CHANNELS) channel = n;
        Serial.print("\n>Channel ");
        Serial.print(channel);
      }
      break;

    case 'T':   // time duration
    {
      timePlay = getNum(idx);
      Serial.print("\n>Time ");
      Serial.print(timePlay);
    }
    break;

    case 'P':   // Print ADSR
      showADSR();
      break;

    case 'I':   // Invert true/false
      adsr[channel].invert = (getNum(idx) != 0);
      Serial.print("\n>Invert ");
      Serial.print(adsr[channel].invert);
      break;
      
    case 'A':   // Attack ms
      adsr[channel].Ta = getNum(idx);
      Serial.print("\n>Ta ");
      Serial.print(adsr[channel].Ta);
      break;

    case 'M':   // Attack Vmax
      adsr[channel].Vmax = getNum(idx);
      Serial.print("\n>Vmax ");
      Serial.print(adsr[channel].Vmax);
      break;

    case 'D':   // Decay ms
      adsr[channel].Td = getNum(idx);
      Serial.print("\n>Td ");
      Serial.print(adsr[channel].Td);
      break;

    case 'S':   // Sustain Vs
      adsr[channel].Vs = getNum(idx);
      Serial.print("\n>Vs ");
      Serial.print(adsr[channel].Vs);
      break;

    case 'R':   // Release ms
      adsr[channel].Tr = getNum(idx);
      Serial.print("\n>Tr ");
      Serial.print(adsr[channel].Tr);
      break;
    }
  }
}

void setup(void)
{
  Serial.begin(57600);
  S.begin();
  for (uint8_t i = 0; i < MD_SN76489::MAX_CHANNELS; i++)
  {
    adsr[i].invert = false;
    adsr[i].Vmax = MD_SN76489::VOL_MAX;
    adsr[i].Vs = MD_SN76489::VOL_MAX - 3;
    adsr[i].Ta = 20;
    adsr[i].Td = 30;
    adsr[i].Tr = 50;
    S.setADSR(i, &adsr[i]);
  };

  Serial.print(F("\n[MD_SN76489 ADSR Envelope]"));
  Serial.print(F("\nEnsure serial monitor line ending is set to newline."));
  help();
}

void loop(void)
{
  S.play();     // run the sound machine every time through loop()
  processUI();  // check the user input
}