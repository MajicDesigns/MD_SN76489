#pragma once

#include <Arduino.h>

/**
* \file
* \brief Main header file for the MD_MAX72xx library
*/

/**
\mainpage SN76489 Sound Generator Library
The SN76489 Sound Generator IC
------------------------------

This library implements functions that allow manages the interface to the 
SN76489 and provides a clean interface for applications.

The library encapsulates the functionality of the SN76489 IC into a few
class methods that allow an application to control the IC.

This work is informed by and borrows from a number of sources:
- On-line IC datasheet at http://members.casema.nl/hhaydn/howel/parts/76489.htm
- Additional technical information from http://www.smspower.org/Development/SN76489

Topics
------
- \subpage pageHardware
- \subpage pageRevisionHistory
- \subpage pageCopyright
- \subpage pageDonation

\page pageDonation Support the Library
If you like and use this library please consider making a small donation 
using [PayPal](https://paypal.me/MajicDesigns/4USD)

\page pageCopyright Copyright
Copyright (C) 2019 Marco Colli. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\page pageRevisionHistory Revision History
Sep 2019 version 0.1.0
- Initial implementation.

\page pageHardware Hardware Connections
The Hardware
------------
The SN76489 uses 8 digital output lines from the Arduino MCU and an additional 
digital output to set toggle loading the data into the IC.


     D5 ->  1 +-----+ 16 <- Vcc
     D6 ->  2 | S N | 15 <- D4
     D7 ->  3 |  7  | 14 <- CLK
    RDY ->  4 |  6  | 13 <- D3
    /WE ->  5 |  4  | 12 <- D2
    /CE ->  6 |  8  | 11 <- D1
    AUD ->  7 |  9  | 10 <- D0
    GND ->  8 +-----+ 9  <- N/C


| Signal| Description
|-------|---------------
|D0-D7  |Command byte inputs
|/WE    |Active low Write Enable (latches data)
|VCC    |5V
|GND    |Ground
|AUD    |Audio output (headphone jack)
|CLK    |4MHz clock signal*
|/CE    |Active low Chip Enable (connect to GND or MCU output if more than one IC shares D0-D7)
|RDY    |Ready signal (unused).


NOTE*: The Arduino MCU may have a timer capable of generating the 4MHz clock. This is a hardware
dependent function implemented in the startClock() method. The output is restricted to a processor 
specific pin which can be connected to the IC CLK input.
*/

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define VOL_OFF 0x0   /// Volume OFF
#define VOL_MAX 0xf   /// Volume MAX

/**
* Core object for the MD_SN76489 library
*/
class MD_SN74689
{
  public:

    /**
    * Number of sound channels available on the device
    */
    static const uint8_t MAX_CHANNELS = 4;   

    /**
    * ADSR definition for a channel
    * ADSR (Attack, Decay, Sustain and Release) defines the sound characteristics
    * for a note being played. See main text for a full explanation.
    */
    typedef struct
    {
      bool    invert; /// Invert the normal curve if true
      uint8_t Vmax;   /// Maximum volume setting for Attack phase
      uint8_t Vs;     /// Sustain volume setting
      uint16_t Ta;    /// Time in ms for the attack curve to reach Vmax
      uint16_t Td;    /// Time in ms for the decay curve to reach Vs
      uint16_t Tr;    /// Time in ms for the Release curve to reach 0 volume.
    } adsrEnvelope_t;
    
    /**
    * Class Constructor - arbitrary digital interface.
    *
    * Instantiate a new instance of the class. The parameters passed are used to
    * connect the software to the hardware. Multiple instances may co-exist.
    *
    * The D array is arranged to correspond to the IC pins (ie, pin D[0] is connected
    * to IC pin D0, D[1] to D1, etc). D0 is the MSB in the data byte, D7 the LSB.
    *
    * \param D       pointer to array of 8 pins number used to interface to IC pins D0 to D7 in that order.
    * \param WE      pin number used as write enable for the IC
    * \param clock   if true the 4MHz clock signal is generated using MCU timers (hardware dependency)
    */
    MD_SN74689(const uint8_t *D, uint8_t WE, bool clock);

    /**
    * Class Destructor.
    *
    * Does the necessary to clean up once the object is no longer required.
    */
    ~MD_SN74689(void);

    /**
    * Initialize the object.
    *
    * Initialize the object data. This needs to be called during setup() to initialize
    * new data for the class that cannot be done during the object creation.
    *
    * The I/O pins are initialized and the 4MHz clock signal is started.
    */
    void begin(void);

    /**
    * Play a frequency on a channel
    *
    * Output a sound with frequency freq on the specified channel. 
    * If a freq is 0, the channel is turned off (equivalent to 
    * setVolume(chan, VOL_OFF). 
    *
    * \param chan    channel number on which to play this note [0..MAX_CHANNELS-1].
    * \param freq    frequency to play.
    */
    void play(uint8_t chan, uint32_t freq);

    /**
    * Set the volume on a channel
    *
    * Set the volume on a channel to be the value specified.
    * Valid values are all the values in the range [VOL_MIN .. VOL_MAX] including the end values.
    *
    * \param chan  channel number on which to play this note [0..MAX_CHANNELS-1].
    * \param v     volume to set for the specificed channel in range [VOL_MIN..VOL_MAX].
    */
    void setVolume(uint8_t chan, uint8_t v);

    /**
    * Set the volume on a channel
    *
    * Set the volume on all channels to be the speified value. Valid values are all 
    * the values in the range [VOL_MIN .. VOL_MAX] including the end values.
    *
    * \param v     volume to set for all channels in range [VOL_MIN..VOL_MAX].
    */
    void setVolume(uint8_t v);

    /**
    * Set the ADSR envelope for a channel
    *
    * Sets the ADSR envelope for a channel. The envelope is defined in a structure
    * of type adsrEnvelope_t. The definition is not copied and must remain in scope 
    * for the durtion of it's use.
    *
    * A null pointer changes the ADSR definition back to the library default. No 
    * change is made if the channel is not idle as the current ADSR definition is 
    * in use.
    * 
    * \param chan  channel number on which to play this note [0..MAX_CHANNELS-1].
    * \param padsr pointer to the ADSR structure to be used.
    */
    void setADSR(uint8_t chan, adsrEnvelope_t* padsr);

    /**
    * Return the idle state of a channel
    *
    * Used to check if a channel is currently idle (ie not playuing a note).
    *
    * \param chan  channel to check [0..MAX_CHANNELS]
    * \return true if the channel is currently idle, false otherwise.
    */
    bool isIdle(uint8_t chan);

    /**
    * Play notes on all channels
    *
    * Runs the ADSR finite state machine for all channels. This should be called
    * from the main loop() as frequently as possible to allow the library to execute
    * the note playing envelope (ADSR).
    */
    void run(void);

  private:
    // Hardware register definitions
    const uint8_t DATA_BITS = 8;          /// Number of bits in the byte (for loops)
    const uint32_t CLOCK_HZ = 4000000UL;  /// 4Mhz clock

    // 1CCTDDDD - 1=Latch+Data, CC=Channel, T=Type, DDDD=Data1
    const uint8_t LATCH_CMD = 0x80;  /// Latch register indicator
    const uint8_t DATA1_MASK = 0x0f; /// 4-bits LSB of data [DATA2|DATA1]

    const uint8_t TYPE_VOL = 0x10;   /// Volume type command

    const uint8_t TYPE_TONE = 0x00;  /// Tone type command
    const uint8_t NOISE_MODE_MASK = 0x04; /// 0=periodic, 1=white
    const uint8_t NOISE_RATE_MASK = 0x03; /// Shift rate

    // 0XDDDDDD - 0=Data, X=Ignored, DDDDDD = Data2
    const uint8_t DATA_CMD = 0x00;   /// Data register indicator
    const uint8_t DATA2_MASK = 0x3f; /// 6-bits MSB of data (if needed)

    // Dynamic data held per tone channel
    enum channelState_t 
    {
      IDLE,     /// doing nothing waiting for play() to turn a note on
      NOTE_ON,  /// play() has set the note to be on
      ATTACK,   /// managing the sound for ATTACK phase
      DECAY,    /// managing the sound for DECAY phase
      SUSTAIN,  /// doing nothying waigint for play() to turn the note off
      NOTE_OFF, /// play() has set the note to be off
      RELEASE   /// managing the sound for RELEASE phase
    };

    struct channelData_t
    {
      uint8_t volume;     /// current volume setting values 0->15 (map to attenuator 15->0)
      uint32_t frequency; /// the frequency being played    

      channelState_t  state;  /// current note playing state

      uint32_t timeBase;  /// base time for current time operation
      uint32_t timeStep;  /// time for each volume step up or down
      int8_t volumeStep;  /// the volume step increment (+/-)
      const adsrEnvelope_t *adsr;  /// current channel adsr envelope
    };
    
    channelData_t C[MAX_CHANNELS];   /// real-time tracking data for each channel

    // Variables
    const uint8_t *_D;    /// IC pins D0-D7 in that order
    uint8_t _WE;          /// Write Enable output pin (active low)
    bool _clock;          /// use MCU as the clock signal generator
    
    // Methods
    void send(uint8_t data);    /// send a byte to the IC 
    void startClock(void);      /// use the MCU timers to generate 4MHz clock

    // Data
    adsrEnvelope_t _adsrDefault;  /// default ADSR envelope, initialize in constructor
};

