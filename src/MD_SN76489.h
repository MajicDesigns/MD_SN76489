#pragma once

#include <Arduino.h>

/**
 * \file
 * \brief Main header file for the MD_SN76489 library
 */

/**
\mainpage SN76489 Sound Generator Library
The SN76489 Sound Generator IC
------------------------------

![SN76489 IC] (SN76489_IC.jpg "SN76489 IC")

The SN76489 Digital Complex Sound Generator (DCSG) is a TTL-compatible programmable
sound generator chip from Texas Instruments. It provides:
- 3 programmable square wave tone generators (122Hz to 125kHz)
- 1 noise generator (white noise and periodic noise at 3 different frequencies)
- 16 different volume levels
- Simultaneous sounds

Its main historical application was the generation of music and sound effects in 
microprocessor systems. It was extensively used in early game consoles, arcade games
and home computers.

This library implements functions that manage the sound and noise generation interface
to the SN76489 IC through a clean API encapsulating the basic functionality provided
by the hardware.

Additionally, the library provides programmable ADSR envelope management of the sounds
produced, allowing a more versatile sound output with minimal programming effort.

Topics
------
- \subpage pageHardware
- \subpage pageLibrary
- \subpage pageADSR
- \subpage pageCompileSwitch
- \subpage pageRevisionHistory
- \subpage pageCopyright
- \subpage pageDonation

References
----------
- On-line IC datasheet at http://members.casema.nl/hhaydn/howel/parts/76489.htm
- Additional technical information from http://www.smspower.org/Development/SN76489

\page pageRevisionHistory Revision History
Nov 2019 version 1.1.0
- Added sanity check for volume settings
- Changed examples to use MD_cmdProcessor library
- Added write() method and VGM player example

Sep 2019 version 1.0.0
- Initial implementation.

\page pageHardware Hardware Connections
SN76489 IC Description
----------------------
The SN76489 Digital Complex Sound Generator (DCSG) is a TTL-compatible programmable
sound generator chip from Texas Instruments. It provides:
- 3 programmable square wave tone generators (122Hz to 125kHz)
- 1 noise generator (white noise and periodic noise at 3 different frequencies)
- 16 different volume levels
- Simultaneous sounds

The IC has 16 pins with the following pinout:

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
|CLK    |4MHz clock signal (see below)
|/CE    |Active low Chip Enable (connect to GND or MCU output if more than one IC shares D0-D7)
|RDY    |Ready signal (unused).

Note: If multiple ICs are interfaced, then the ICs CE line must also be used
to select the right device and share the data lines. This is not managed
by the library and needs to be part of the user application.

Tone Generators
---------------
The frequency of the square waves produced by the tone generators on
each channel is derived from two factors:
- The speed of the external clock.
- A value provided in a control register for that channel (called N).

Each channel's frequency is arrived at by dividing the external clock by 32
and then dividing the result by N. Thus the overall divider range is from 32
to 32768. This gives a frequency range at maximum input clock rate of 122Hz
to 125kHz (a range from roughly A2, two octaves below middle A, to 5-6 times
the generally accepted limits of human audio perception).

The 4MHz Clock Signal
---------------------
The clock signal may be supplied from external hardware or can be created by the
MCU. This can be specified class initializer parameters.

The MCU may have a timer capable of generating the 4MHz clock. This is a hardware
dependent function implemented in the private startClock() method. The output is
usually restricted to a processor specific pin which can be connected to the IC
CLK input. The currently supported hardware is

| CPU Type   | Example Board        | Resource | Pin
|------------|----------------------|----------|-------
| ATmega328P | Uno, Nano, Mini      | Timer 2  | Pin 3
| ATmega32U4 | Teensy 2.0, Leonardo | Timer 1  | Pin 14

If the library does not have code for the particular MCU, a compiler warning is
generated and the private startClock() method is compiled with no code.
Obviously, this results in no clock signal generated by the MCU and an external
clock signal must be provided.

If an external hardware clock is used, then the library can be prevented from
starting the MCU clock in the class initialization parameters.

Hardware Direct Connection to the MCU
-------------------------------------
The derived class SN_SN76489_Direct uses 8 digital output data lines 
from the Arduino MCU and an additional Write Enable digital output to 
load the data into the SN76489 IC.

The data pins used are for the application to specify as part of the 
object initialization parameters. The D array parameter has pin numbers 
arranged to correspond to the IC pins (ie, pin D[0] is connected to IC 
pin D0, D[1] to D1, etc). The WE pin can be any arbitrary pin.

Connections between the MCU and SN76489 are mapped as shown below.
The Arduino pins are arbitrary except the 4Mhz clock - the pins shown are 
used in the library examples.

| Arduino Pin | SN76489                |
|-------------|------------------------|
| D0   [A0]   | D0    [ 3]             |
| D1   [A1]   | D1    [ 2]             |
| D2   [A2]   | D2    [ 1]             |
| D3   [A3]   | D3    [15]             |
| D4   [ 4]   | D4    [13]             |
| D5   [ 5]   | D5    [12]             |
| D6   [ 6]   | D6    [11]             |
| D7   [ 7]   | D7    [10]             |
| WE   [ 8]   | /WE   [ 5]             |
| 4Mhz [ 3]   | CLK   [14]             |
|             | /OE   [ 6] (GND)       |
|             | AUDIO [ 7] (Amplifier) |


SPI Connection the the MCU
--------------------------
The derived class SN_SN76489_SPI uses 3 SPI pins (LD, CLK and DAT)
from the Arduino MCU to a 74595 Serial to Parallel buffer IC and 
an additional Write Enable digital output to load the data from the 
595 outputs to into the SN76489 IC.

This uses less pins than a direct connection but requires an additional 
IC. The pins used are arbitrary and specified by the application as part
of the object initialization parameters.

Connections between the MCU, 74595 and SN76489 are mapped as shown below.
The Arduino pins are arbitrary except the 4Mhz clock - the pins shown are 
used in the library examples.

| Arduino Pin | 74595 buffer   | SN76489                |
|-------------|----------------|------------------------|
|             | D0  [15]       | D0    [ 3]             |
|             | D1  [ 1]       | D1    [ 2]             |
|             | D2  [ 2]       | D2    [ 1]             |
|             | D3  [ 3]       | D3    [15]             |
|             | D4  [ 4]       | D4    [13]             |
|             | D5  [ 5]       | D5    [12]             |
|             | D6  [ 6]       | D6    [11]             |
|             | D7  [ 7]       | D7    [10]             |
| Data  [11]  | DAT [14]       |                        |
| Load  [10]  | LD  [12]       |                        |
| Clock [13]  | CLK [11]       |                        |
|             | /OE [13] (GND) |                        |
|             | /MR [10] (+5V) |                        |
| WE    [ 8]  |                | /WE   [ 5]             |
| 4Mhz  [ 3]  |                | CLK   [14]             |
|             |                | /OE   [ 6] (GND)       |
|             |                | AUDIO [ 7] (Amplifier) |

Audio Output
------------
The Audio output from pin 7 of the IC is a mono signal that can be heard 
directly fed into earbuds. Connection to external speaker must be through
an amplifier.

![SN76489 Audio Output] (SN76489_Audio_Out.png "SN76489 Audio Output")

\page pageLibrary Using the Library
Defining the object
-------------------
The object definition include a list of all the I/O pins that are used
to connect the MCU to the SN76489 IC.

setup()
-------
The setup() function must include the begin() method. All the I/O pins are
initialized at this time.

loop()
------
ADSR envelopes and/or automatic note off events (see below) are managed by the
library. For this to happen in a timely manner, the loop() function must invoke
play() every iteration through loop(). The play() method executes very quickly 
if the library has nothing to process, imposing minimal overheads on the user
application.

Playing a Note
--------------
A note starts with the __note on__ event and ends with a __note off__ event.
If an ADSR envelope is active, the Release phase starts at the note off event.
The note on event is generated when the note(), tone() or noise() method is 
invoked in the application code.

Note On and Off Events
----------------------
The library provides flexibility on how the note on and note off events are
generated.

Invoking the tone(), note() or noise() methods __without a duration parameter__
means the user code needs to generate the note off (this depends on the channel 
type and is explained in the specific method reference). This method is suited 
to applications that directly link a physical event to playing the note (eg, 
switch on for note on and switch off for note off), or where the music being 
played includes its own note on and off events (eg, a MIDI score).

Invoking the the tone(), note() or noise() methods __with a duration parameter__
causes the library to generate a note off event at the end of the specified
total duration. If an ADSR envelope is active, the note duration encompasses
the time between initial Attack phase (note on) to the end of the Release phase
(ie, the Sustain time is calculated from the given duration). This method suits
applications where the sound duration is defined by the music being played (eg, 
RTTTL tunes). In this case the user code can determine if the sound has completed 
playing by using the isIdle() method.

\page pageADSR ADSR Envelope
Attack, Decay, Sustain, Release (ADSR) Envelope
-----------------------------------------------
An ADSR generated sound envelope is a component of many synthesizers and other electronic musical 
instruments. The sound envelope modulates the sound, often its loudness, over time. 
In this library the envelope is implemented in software to control the volume of the sound.

![ADSR Envelope] (ADSR_Envelope.png "ADSR Envelope")

Electronic instruments can also implement an inverted ADSR envelope, resulting in the opposite 
behavior: during the attack phase, the sound fades out from the maximum amplitude to zero,
rises up to the value specified by the sustain parameter in the decay phase and continues to 
rise from the sustain amplitude back to maximum amplitude.

The ADSR envelope can be specified using the following parameters:
- **Attack**: The time interval (Ta) between activation and full loudness (Vmax).
- **Decay**: The time interval (Td) for Vmax to drop to the sustain level (Vs).
- **Sustain**: The constant sound volume (Vs) for the note until it is released.
- **Release**: The time interval (Tr) for the sound to fade from Vs to 0 when a note ends.

An ADSR note cycle is controlled by 2 events - an *note on* event that triggers the attack 
phase of the cycle and a *note off* event that triggers the release phase of the cycle. On
a keyboard instrument these would correspond to a key being pressed and then released.

The library initially defines supplies one default ADSR envelope for all channels. These 
can be changed per channel in real time using the setADSR() method to specify the new 
envelope parameters. The nomenclature for the parameters follows the labels and explanation 
above. A flag can be set to invert the specified envelope.

The SN74689 volume controls are limited to 15 steps, so the Attack, Decay or Release phases
are implemented as a linear progression changing the sound volume over time.

\page pageCompileSwitch Compiler Switches

LIBDEBUG
--------
Controls debugging output to the serial monitor from the library. If set to
1 debugging is enabled and the main program must open the Serial port for output

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

*/

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))  ///< Standard method to work out array size

/**
 * Base class for the MD_SN76489 library
 */
class MD_SN76489
{
  public:
    static const uint8_t MAX_CHANNELS = 4;  ///< Number of available sound channels
    static const uint8_t NOISE_CHANNEL = 3; ///< The channel for periodic/white noise
    static const uint8_t VOL_OFF = 0x0;     ///< Convenience constant for volume off
    static const uint8_t VOL_MAX = 0xf;     ///< Convenience constant for volume on

   /**
    * Noise type enumerated definitions
    * The NOISE_CHANNEL can produce either white or periodic noise. This enumerated
    * type is used to define the setting for the channel.
    */
    typedef enum 
    { 
      PERIODIC_0 = 0x0, ///< Periodic noise, output/512
      PERIODIC_1 = 0x1, ///< Periodic noise, output/1024
      PERIODIC_2 = 0x2, ///< Periodic noise, output/2048
      PERIODIC_3 = 0x3, ///< Periodic noise, output/(Channel 2 freq)
      WHITE_0    = 0x4, ///< White noise, output/512
      WHITE_1    = 0x5, ///< White noise, output/1024
      WHITE_2    = 0x6, ///< White noise, output/2048
      WHITE_3    = 0x7, ///< White noise, output/(Channel 2 freq)
      NOISE_OFF  = 0xf, ///< Special indicator to turn noise off if not timed
    } noiseType_t;

   /**
    * ADSR definition for a channel.
    * The ADSR envelope defines the sound characteristics for notes being 
    * played on a channel. See \ref pageADSR for more information.
    */
    typedef struct 
    {
      bool invert;    ///< Invert the normal curve if true
      uint16_t Ta;    ///< Time in ms for the attack curve to reach Vmax
      uint16_t Td;    ///< Time in ms for the decay curve to reach Vs
      uint8_t deltaVs;///< Sustain volume setting relative to volume SP (absolute value)
      uint16_t Tr;    ///< Time in ms for the Release curve to reach 0 volume.
    } adsrEnvelope_t;
    
   /**
    * Class Constructor.
    *
    * Instantiate a new instance of the class. The parameters passed are used to
    * connect the software to the hardware. Multiple instances may co-exist.
    *
    * \param clock   if true the 4MHz clock signal is generated using MCU timers (hardware dependency)
    */
    MD_SN76489(bool clock);

   /**
    * Class Destructor.
    *
    * Does the necessary to clean up once the object is no longer required.
    */
    ~MD_SN76489(void) {};

   /**
    * Initialize the object.
    *
    * Initialize the object data. This needs to be called during setup() to initialize
    * new data for the class that cannot be done during the object creation.
    *
    * User code will invoke begin() method from the derived class to initialise,
    * which will in turn call this base class method.
    * 
    * In this base class method, the 4MHz clock signal is started, the ADSR envelope
    * initialzed and sound volume turned off.
    *
    */
    virtual void begin(void);
    
   //--------------------------------------------------------------
   /** \name Hardware control basics.
    * @{
    */

   /**
    * Set the volume for a channel.
    *
    * Set the volume for a channel to be the value specified.
    * Valid values are all the values in the range [VOL_MIN..VOL_MAX].
    *
    * \param chan  channel number on which to play this note [0..MAX_CHANNELS-1].
    * \param v     volume to set for the specificed channel in range [VOL_MIN..VOL_MAX].
    */
    void setVolume(uint8_t chan, uint8_t v);

   /**
    * Set the volume for all channels.
    *
    * Set the volume for all channels to be the speified value. Valid values are all
    * the values in the range [VOL_MIN..VOL_MAX].
    *
    * \param v     volume to set for all channels in range [VOL_MIN..VOL_MAX].
    */
    void setVolume(uint8_t v);

   /**
    * Set the frequency for a channel.
    *
    * Set the frequency output for a channel to be the value specified.
    *
    * This method is not supported by the NOISE_CHANNEL.
    *
    * \sa setNoise()
    *
    * \param chan  channel number on which to play this note [0..MAX_CHANNELS-2].
    * \param freq  frequency to set for the specified channel.
    */
    void setFrequency(uint8_t chan, uint16_t freq);

    /**
     * Set the noise channel parameters.
     *
     * Set the noise parameters for NOISE_CHANNEL. This channel cannot play notes.
     * Noise may be one of the noiseType_t types.
     *
     * New settings will immediately replace old settings for for NOISE_CHANNEL.
     *
     * \param  noise one of the valid noise types in noiseType_t.
     */
    void setNoise(noiseType_t noise);

    /** @} */

    //--------------------------------------------------------------
    /** \name Methods for notes and tones.
     * @{
     */

    /**
     * Play a tone without ADSR.
     *
     * Output a sound with frequency freq on the specified channel.
     * If another note or tone is playing on the channel this will be 
     * immediately replaced by the new tone. 
     *
     * If specified, the duration will cause an automatic note off 
     * event when the total time has expired.
     *
     * If a freq is 0, the tone is turned off. If duration is 0, 
     * the tone remains in the sustain phase until it is turned off.
     *
     * This method is not supported by the NOISE_CHANNEL.
     *
     * \sa note()
     *
     * \param chan    channel number on which to play this note [0..MAX_CHANNELS-1].
     * \param freq    frequency to play.
     * \param volume  volume to play, default to VOL_MAX, in the range [0..VOL_MAX]
     * \param duration length of time in ms for the whole note to last.
     */
    void tone(uint8_t chan, uint16_t freq, uint8_t volume, uint16_t duration = 0);

   /**
    * Play a note on using ADSR.
    *
    * Output a sound with frequency freq on the specified channel using the
    * ADSR envelope curently defined for the channel. If another note or tone
    * is playing on the channel this will be immediately replaced by the 
    * new note.
    *
    * If specified, the duration will cause an automatic note off
    * event when the total time has expired. Note that the duration
    * is measured from the start of the A to the end of the R phases
    * of the playing envelope.
    *
    * If a freq is 0, the note is turned off. If duration is 0,
    * the note remains in the sustain phase until it is turned off.
    *
    * This method is not supported by the NOISE_CHANNEL.
    *
    * \sa tone()
    *
    * \param chan    channel number on which to play this note [0..MAX_CHANNELS-2].
    * \param freq    frequency to play.
    * \param volume  volume to play, default to VOL_MAX, in the range [0..VOL_MAX]
    * \param duration length of time in ms for the whole note to last.
    */
    void note(uint8_t chan, uint16_t freq, uint8_t volume, uint16_t duration = 0);

    /**
     * Play a noise using ADSR.
     *
     * Output a noise as specified on NOISE_CHANNEL, using the
     * ADSR envelope curently defined for the channel. If another noise
     * is playing on NOISE_CHANNEL it will be immediately replaced by the
     * new noise.
     *
     * If specified, the duration will cause an automatic note off
     * event when the total time has expired. Note that the duration
     * is measured from the start of the A to the end of the R phases
     * of the playing envelope.
     *
     * This method is ONLY supported by the NOISE_CHANNEL.
     *
     * \param noise      one of the valid noise types in noiseType_t.
     * \param volume  volume to play, default to VOL_MAX, in the range [0..VOL_MAX]
     * \param duration length of time in ms for the whole note to last.
     *
     */
    void noise(noiseType_t noise, uint8_t volume, uint16_t duration = 0);

    /**
    * Set the ADSR envelope for a channel.
    *
    * Sets the ADSR envelope for a channel. The envelope is defined in a structure
    * of type adsrEnvelope_t. The envelope definition is not copied and must remain 
    * in scope while it is in use. The application may change elements of the 
    * envelope defintion and they will be immediately reflected in sound output.
    *
    * A null pointer changes the ADSR definition back to the library default. The 
    * ADSR profile cannot be changed while it is in use (ie, channel not idle).
    *
    * \sa isIdle()
    * 
    * \param chan  channel number on which to play this note [0..MAX_CHANNELS-1].
    * \param padsr pointer to the ADSR structure to be used.
    * \return true if the change was possible, false otherwise.
    */
    bool setADSR(uint8_t chan, adsrEnvelope_t* padsr);

   /**
    * Set the same ADSR envelope for all channels.
    *
    * Sets the same ADSR envelope for all channels by passing in an application 
    * copy of the envelope definition. The application may change values in the 
    * envelope and they will be immediately reflected in the sound.
    *
    * \sa setADSR()
    *
    * \param padsr pointer to the ADSR structure to be used.
    * \return true if all changes were possible, false otherwise.
    */
    bool setADSR(adsrEnvelope_t* padsr);

   /**
    * Return the idle state of a channel.
    *
    * Used to check if a channel is currently idle (ie, not playing a note).
    *
    * \param chan  channel to check [0..MAX_CHANNELS-1]
    * \return true if the channel is idle, false otherwise.
    */
    bool isIdle(uint8_t chan);

   /**
    * Play the music machine.
    *
    * Runs the ADSR finite state machine for all channels. This should be called
    * from the main loop() as frequently as possible to allow the library to execute
    * the note required timing for the note envelopes.
    */
    void play(void);

   /**
    * Write a byte directly to the device
    *
    * This method should be used with caution, as it bypasses all the checks
    * and buffering built into the library. It is provided to support applications
    * that are a collection of register setting to be written to hardware at set 
    * time intervals (eg, VGM files).
    *
    * \param data  the 8 bit data value to write to the device.
    */
    inline void write(uint8_t data) { send(data); }

   /** @} */
  protected:
    const uint8_t DATA_BITS = 8;        ///< Number of bits in the byte (for loops)

   /**
    * Send a byte to the SN76489IC.
    *
    * Sending a byte to the SN76489 IC depends on how it is connected to the
    * MCU. This base class method must be replaced by derived class methods that 
    * implements the appropriate data transfer method.
    *
    * \param data  the data byte to transmit
    */
    virtual void send(uint8_t data);

  private:
    // Hardware register definitions
    const uint32_t CLOCK_HZ = 4000000UL;  ///< 4Mhz clock

    // 1CCTDDDD - 1=Latch+Data, CC=Channel, T=Type, DDDD=Data1
    const uint8_t LATCH_CMD = 0x80;  ///< Latch register indicator
    const uint8_t DATA1_MASK = 0x0f; ///< 4-bits LSB of data [DATA2|DATA1]

    const uint8_t TYPE_VOL = 0x10;   ///< Volume type command
    const uint8_t TYPE_TONE = 0x00;  ///< Tone type command

    const uint8_t NOISE_RATE_MASK = 0x03; ///< Noise shift rate

    // 0XDDDDDD - 0=Data, X=Ignored, DDDDDD = Data2
    const uint8_t DATA_CMD = 0x00;   ///< Data register indicator
    const uint8_t DATA2_MASK = 0x3f; ///< 6-bits MSB of data (if needed)

    // Dynamic data held per tone channel
    enum channelState_t 
    {
      IDLE,     ///< doing nothing waiting for play() to turn a note on
      NOTE_ON,  ///< note() has started the play sequence
      TONE_ON,  ///< tone() has started the play sequence
      NOISE_ON, ///< noise() has started the play sequence
      ATTACK,   ///< managing the sound for ATTACK phase
      DECAY,    ///< managing the sound for DECAY phase
      SUSTAIN,  ///< wiat out duration period if specified, or for note() or tone() to turn off
      NOTE_OFF, ///< note() has set the note to be off
      RELEASE   ///< managing the sound for RELEASE phase
    };

    struct channelData_t
    {
      uint8_t volSP;  ///< volume setpoint for this channel, 0-15 (map to attenuator 15-0)
      uint8_t volCV;  ///< volume current value for this channel, 0-15 (map to attenuator 15-0)
      int8_t volumeStep;  ///< the volume step increment (+1/-1) during ADSR

      uint16_t frequency; ///< the frequency being played (or noise settings for NOISE_CHANNEL)
      uint16_t duration;  ///< the total playing duration for the sustain phase
      bool playTone;      ///< true if we are just playing a tone.

      channelState_t  state;  ///< current note playing state

      uint32_t timeBase;  ///< base time for current time operation
      uint32_t timeStep;  ///< time for each volume step up or down

      const adsrEnvelope_t *adsr;  ///< current channel adsr envelope
    };
    
    channelData_t C[MAX_CHANNELS];   ///< real-time tracking data for each channel

    // Variables
    bool _clock;          ///< use MCU as the clock signal generator
    
    // Methods
    void startClock(void);              ///< use the MCU timers to generate 4MHz clock
    void setCVolume(uint8_t chan, uint8_t v);         ///< set current volume level
    uint8_t saneVolume(uint8_t volume); ///< return a volume setting within bounds
    uint16_t calcTs(uint8_t chan, uint16_t duration); ///< work out the Vs time for this note

    // Data
    adsrEnvelope_t _adsrDefault;  ///< default ADSR envelope, initialize in constructor
};

/**
 * Derived class for direct I/O control of the SN76489 IC
 */
class MD_SN76489_Direct: public MD_SN76489
{
public:
  /**
   * Class Constructor.
   *
   * Instantiate a new instance of this derived class. The parameters passed are used to
   * connect the software to the hardware. Multiple instances may co-exist.
   *
   * The D array is arranged to correspond to the IC pins (ie, pin D[0] is connected
   * to IC pin D0, D[1] to D1, etc). D0 is the MSB in the data byte, D7 the LSB.
   *
   * \sa \ref pageHardware
   *
   * \param D       pointer to array of 8 pins number used to interface to SN76489 IC pins D0 to D7 in that order.
   * \param we      pin number used as write enable for the SN76489 IC
   * \param MCUclk  if true the 4MHz clock signal is generated using MCU timers (hardware dependency)
  */
  MD_SN76489_Direct(const uint8_t* D, uint8_t we, bool MCUclk):
    MD_SN76489(MCUclk), _D(D), _we(we)
  {};

  /**
   * Initialize the object.
   *
   * Initialize the object data. This needs to be called during setup() to initialize
   * new data for the class that cannot be done during the object creation.
   * 
   * Initializes the output pins.
   */
  void begin(void);

protected:
 /**
  * Send a byte to the SN76489IC.
  *
  * Send a byte to the SN76489 IC directly connected to the MCU I/O pins.
  * Uses the pin number from the D array to send data D0-D7, where D0 is 
  * the MSB.
  *
  * \param data  the data byte to transmit
  */
  void send(uint8_t data);

private:
  const uint8_t* _D;    ///< SN76489 IC pins D0-D7 in that order
  uint8_t _we;          ///< SN76489 write Enable output pin (active low)
};

/**
 * Derived class for SPI 74595 buffer control of the SN76489 IC
 */
class MD_SN76489_SPI: public MD_SN76489
{
public:
  /**
   * Class Constructor.
   *
   * Instantiate a new instance of this derived class. The parameters passed are used to
   * connect the software to the hardware. Multiple instances may co-exist.
   *
   * The D array is arranged to correspond to the IC pins (ie, pin D[0] is connected
   * to IC pin D0, D[1] to D1, etc). D0 is the MSB in the data byte, D7 the LSB.
   *
   * \sa \ref pageHardware
   *
   * \param ld     the SPI pin to toggle to load the data in the 595 buffer
   * \param dat    the SPI pin to toggle for data transmission to the 595 buffer
   * \param clk    the SPI pin to toggle for clocking data into the 595 buffer
   * \param we     pin number used as write enable for the SN76489 IC
   * \param MCUclk if true the 4MHz clock signal is generated using MCU timers (hardware dependency)
  */
  MD_SN76489_SPI(uint8_t  ld, uint8_t dat, uint8_t clk, uint8_t we, bool MCUclk) :
    MD_SN76489(MCUclk), _dat(dat), _ld(ld), _clk(clk), _we(we)
  {};

  /**
   * Initialize the object.
   *
   * Initialize the object data. This needs to be called during setup() to initialize
   * new data for the class that cannot be done during the object creation.
   *
   * Initializes the output pins.
   */
  void begin(void);

protected:
 /**
  * Send a byte to the SN76489IC.
  *
  * Send a byte to the SN76489 IC using SPI protocol through a 74595 
  * serial to parallel buffer. Uses the pin numbers passed through the 
  * constructor to shift the data out to the buffer IC.
  *
  * \param data  the data byte to transmit
  */
  void send(uint8_t data);

private:
  uint8_t _dat;  ///< SPI data out pin (MOSI)
  uint8_t _ld;   ///< SPI load data pin (LD)
  uint8_t _clk;  ///< SPI clock pin (CLK)
  uint8_t _we;   ///< SN76489 Write Enable output pin (active low)
};