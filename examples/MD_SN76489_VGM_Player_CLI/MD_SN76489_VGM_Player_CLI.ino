// Command Line Interface (Serial) to play VGM files from the SD card.
// 
// Enter commands on the serial monitor to control the application
//
// Dependencies
// SDFat at https://github.com/greiman?tab=repositories
// MD_cmdProcessor at https://github.com/MajicDesigns/MD_cmdProcessor
//
// VGM file format at https://vgmrips.net/wiki/VGM_Specification
// VGM sound files at https://vgmrips.net/packs/chip/sn76489

#include <SdFat.h>
#include <MD_SN76489.h>
#include <MD_cmdProcessor.h>

// Define if we are using a direct or SPI interface to the sound IC
// 1 = use direct, 0 = use SPI
#ifndef USE_DIRECT
#define USE_DIRECT 1
#endif

// Hardware Definitions ---------------
#if USE_DIRECT
// All the pins directly connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
#else
// Define the SPI related pins
const uint8_t LD_PIN = 10;
const uint8_t DAT_PIN = 11;
const uint8_t CLK_PIN = 13;
#endif
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin

// SD chip select pin for SPI comms.
const uint8_t SD_SELECT = 10;

// Miscellaneous
void(*hwReset) (void) = 0;            // declare reset function @ address 0
const uint16_t SAMPLE_uS = 23; // 1e6 us/sec at 44100 samples/sec = 22.67us per sample

// Global Data ------------------------
SdFat SD;
SdFile FD;    // file descriptor
#if USE_DIRECT
MD_SN76489_Direct S(D_PIN, WE_PIN, true);
#else
MD_SN76489_SPI S(LD_PIN, DAT_PIN, CLK_PIN, WE_PIN, true);
#endif

uint32_t dataOffset = 0;    // offset to start of music data in VGM file
bool playingVGM = false;    // flag true when in playing mode


void playVGM(void)
{
  uint32_t samples = 0;
  int cmd = FD.read();

  if (cmd == -1) cmd = 0x66;    // make this stop
  
  switch (cmd)
  {
    case 0x50: // 0x50 dd : PSG (SN76489/SN76496) write value dd
    {
      uint8_t data = FD.read();
      S.write(data);
    }
    break;

    case 0x61: // 0x61 nn nn : Wait n samples, n can range from 0 to 65535
      samples = FD.read() & 0x00FF;
      samples |= (FD.read() << 8) & 0xFF00;
      delay((samples * SAMPLE_uS) / 1000);
      break;

    case 0x62: // wait 735 samples (60th of a second)
      delay(17);  // SAMPLE_uS * 735 = 16.905ms
      break;

    case 0x63: // wait 882 samples (50th of a second)
      delay(20);  // SAMPLE_uS * 882 = 20.286ms
      break;

    case 0x70 ... 0x7f: // 0x7n : wait n+1 samples, n can range from 0 to 15
      samples = cmd + 0x70 + 1;
      delayMicroseconds(samples * SAMPLE_uS);
      break;

    case 0x66: // 0x66 : end of sound data
      handlerS(nullptr);
      break;

    default:
      break;
  } //end switch
}

uint32_t readVGMDword(uint16_t fileOffset)
// All integer values are *unsigned* and written in 
// "Intel" byte order (Little Endian), so for example 
// 0x12345678 is written as 0x78 0x56 0x34 0x12.
{
  uint8_t data[4];
  uint32_t result;

  FD.seekSet(fileOffset);
  for (uint8_t i = 0; i < ARRAY_SIZE(data); i++)
    data[i] = FD.read() & 0xff;

  result = 0;
  for (int8_t i = ARRAY_SIZE(data) - 1; i >= 0; i--)
    result = (result << 8) | data[i];

  return(result);
}

uint16_t checkVGMHeader(char* file)
{
  uint32_t offset = 0;
  uint32_t x, version;

  // try to open the file
  if (file[0] == '\0')
    return(0);

  if (!FD.open(file, O_READ))
  {
    Serial.print(F("\nFile not found."));
    return(0);
  }

  // read the header for VGM signature (0x00)
  x = readVGMDword(0x00);
  if (x != 0x206d6756) // " mgV"
  {
    Serial.print("\nNot a VGM file: 0x");
    Serial.print(x, HEX);
    return(0);
  }

  // get the VGM version (0x08)
  version = readVGMDword(0x08);
  Serial.print(F("\nVGM version: 0x"));
  Serial.print(version, HEX);

  // verify SN76489 clock speed (0x0c)
  x = readVGMDword(0x0c);
  Serial.print(F("\nSN76489 clock: "));
  Serial.print(x);

  // get the relative data offset value (0x34)
  if (version < 0x150)
    offset = 0x40;
  else
  {
    offset = readVGMDword(0x34);
    offset += 0x34;  // make it absolute
  }
  //Serial.print(F("\nData offset: 0x"));
  //Serial.print(offset, HEX);

  // set up the next character read at the data offset
  FD.seekSet(offset);

  return(offset);
}

void handlerHelp(char* param); // function prototype only

void handlerZ(char *param) { hwReset(); }

void handlerS(char* param)
// Stop play
{ 
  S.setVolume(MD_SN76489::VOL_OFF);
  playingVGM = false;
  FD.close();
}

void handlerP(char *param)
// Play
{
  Serial.print(F("\n\nVGM file: "));
  Serial.print(param);
  dataOffset = checkVGMHeader(param); // load the new file
  playingVGM = (dataOffset != 0);
}

void handlerF(char *param)
// set the current folder for MIDI files
{
  Serial.print(F("\nFolder: "));
  Serial.print(param);
  if (!SD.chdir(param, true)) // set folder
    Serial.print(" unable to set.");
}

void handlerL(char *param)
// list the files in the current folder
{
  SdFile file;    // iterated file

  SD.vwd()->rewind();
  while (file.openNext(SD.vwd(), O_READ))
  {
    char buf[20];

    file.getName(buf, ARRAY_SIZE(buf));
    Serial.print(F("\n"));
    Serial.print(buf);
    file.close();
  }
  Serial.print(F("\n"));
}

const MD_cmdProcessor::cmdItem_t PROGMEM cmdTable[] =
{
  { "h", handlerHelp, "",    "help" },
  { "?", handlerHelp, "",    "help" },
  { "f", handlerF,    "fldr", "set current folder to fldr" },
  { "l", handlerL,    "",     "list files in current folder" },
  { "p", handlerP,    "file", "play the named file" },
  { "s", handlerS,    "",     "stop playing current file" },
  { "z", handlerZ,    "",     "software reset" },
};

MD_cmdProcessor CP(Serial, cmdTable, ARRAY_SIZE(cmdTable));

void handlerHelp(char* param) { CP.help(); }

void setup(void) // This is run once at power on
{
  Serial.begin(57600); // For Console I/O
  Serial.print(F("\n[MD_SN74689 VGM Player CLI]"));
  Serial.print(F("\nEnsure serial monitor line ending is set to newline."));

  // Initialise SN74689
  S.begin();
  S.setVolume(MD_SN76489::VOL_OFF);

  // Initialize SD
  if (!SD.begin(SD_SELECT, SPI_FULL_SPEED))
  {
    Serial.print(F("\nSD init fail!"));
    while (true);
  }

  // initialise command processor
  CP.begin();

  // show the available commands
  handlerHelp(nullptr);
}

void loop(void)
{
  if (playingVGM) 
    playVGM();
  //else
    CP.run();  // process the User Interface
}