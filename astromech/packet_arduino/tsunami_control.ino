#include "tsunami_control.h"
#include <Wire.h>

// The Time at which the Currently Playing Song Ends
int song_end_time = 0;

// The Index of the Currently Playing Song
int song_index = 0;

// Is Playing Variable
extern bool is_playing;

// Tsunami user's guide:  
// https://cdn.sparkfun.com/assets/e/9/9/8/e/Tsunami_UserGuide_20230114.pdf

// Send Command to Tsunami to Play a Sound
void playTsunamiSound(byte index, int volume) {
    // Set the Volume of the Track, if Volume is not greater than 10
    if (volume < 11) {
        unsigned short truncated_volume = (unsigned short)volume;
        Wire.beginTransmission(TSUNAMI_ADDRESS);      // Begin Tsunami Transmission
        Wire.write(0x08);                             // Send Command to Set Volume of Sound
        Wire.write(index);                            // Least Signifigant Byte for File Index
        Wire.write(0x00);                             // No need for most significant bit
        Wire.write((uint8_t)(truncated_volume));      // Send Least Signifigant Bit for Volume
        Wire.write((uint8_t)(truncated_volume >> 8)); // Send Most Signifigant Bit for Volume
        Wire.endTransmission();                       // End Tsunami Transmission
    }

    // Play the Sound
    Wire.beginTransmission(TSUNAMI_ADDRESS); // Begin Tsunami Transmission
    Wire.write(0x03);                        // Send Command to Play a Sound
    Wire.write(0x01);                        // Send Control Code to Play a Polyphonic Sound
    Wire.write(index);                       // Least Signifigant Byte for File Index
    Wire.write(0x00);                        // No need for most signifigant byte
    Wire.write(0x00);                        // Stereo Output
    Wire.endTransmission();                  // End Tsunami Transmission

    // The List of Song Lengths
    const int SONG_LENGTHS[] = { 
        MAIN_THEME_LENGTH,         // Index = 1
        CANTINA_BAND_LENGTH,       // Index = 2
        THRONE_ROOM_LENGTH,        // Index = 3
        DUEL_OF_THE_FATES_LENGTH,  // Index = 7
        IMPERIAL_MARCH_LENGTH,     // Index = 8
        MANDALORIAN_THEME_LENGTH   // Index = 9
    };

    // If Tsunami is Now Playing a Song, Set the Song Time to the Length of the Song Plus Current Time
    if ((index > 0 && index < 4) || (index > 6 && index < 10))
    {
        // Light Side
        if (index < 4)
            song_end_time = SONG_LENGTHS[index - 1];

        // Dark Side
        else
            song_end_time = SONG_LENGTHS[index - 4];

        // Add the Current Time to Song End Time
        song_end_time += millis();

        // Store Song Index
        song_index = index;
    }
}

// Send Command to Tsunami to Set Master Volume
void setTsunamiMasterVolume(int volume) {
    unsigned short truncated_volume = (unsigned short)volume;

    // Send Command Twice for Each Output
    for (int i = 0; i < 2; i++) {
        Wire.beginTransmission(TSUNAMI_ADDRESS);      // Begin Tsunami Transmission
        Wire.write(0x05);                             // Send Command to Set Master Volume
        Wire.write(i + 0x00);                         // Write To Left and Right Outputs
        Wire.write((uint8_t)(truncated_volume & 0xff));      // Send Least Signifigant Byte for Volume
        Wire.write((uint8_t)((truncated_volume >> 8) & 0xff)); // Send Most Signifigant Byte for Volume
        Wire.endTransmission();                       // End Tsunami Transmission
    }
}

inline void stopSingleTrack(int index)
{
    // Stop Specified Song at the Index
    Wire.beginTransmission(TSUNAMI_ADDRESS); // Begin Tsunami Transmission
    Wire.write(0x03);                        // Send Command to Play a Sound
    Wire.write(0x04);                        // Send Control Code to Stop the Track
    Wire.write(index);                       // Least Signifigant Byte for File Index
    Wire.write(0x00);                        // No need for most signifigant byte
    Wire.write(0x00);                        // Stereo Output
    Wire.endTransmission();                  // End Tsunami Transmission
}

void stopTracks() {
    Wire.beginTransmission(TSUNAMI_ADDRESS);      // Begin Tsunami Transmission
    Wire.write(0x04);                             // Send Command to Stop All Tracks
    Wire.endTransmission();                       // End Tsunami Transmission
}

// Read a Register from the Amplifier
// Sending Data to Amplifier Changes The Rest of the Register,
// So We Need to Read it's Current State to Change What We Want
// Without Changing The Rest of the Register
byte readAmplifierRegister(byte reg) {
    // Send Message to Amplifier Asking to Load Specified Register for Reading
    Wire.beginTransmission(AMPLIFIER_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission();

    // Read the Value Stored in the Register
    Wire.requestFrom(AMPLIFIER_ADDRESS, 1);
    return (byte)Wire.read();
}

// Write a Modified Register to the Amplifier
void writeAmplifierRegister(byte reg, byte data) {
    // Send Value to Amplifier
    Wire.beginTransmission(AMPLIFIER_ADDRESS);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

// Reset Values in the Amplifier
void resetAmplifier() {
    // Set Compression Ratio to 0
    writeAmplifierRegister(7, readAmplifierRegister(7) & 252);

    // Disable the Limiter
    writeAmplifierRegister(6, readAmplifierRegister(6) & 128);

    // Disable the Noise Gate
    writeAmplifierRegister(1, readAmplifierRegister(1) & 254);

    // Set the Attack to 1
    writeAmplifierRegister(2, 1);

    // Set the Release to 1
    writeAmplifierRegister(3, 1);
}

// Set the Gain of the Amplifier
void setAmplifierGain(byte gain) { writeAmplifierRegister(5, gain & 63); }

inline void testEndOfSong()
{
    // If Song is Not Playing, Early Return
    if (!is_playing)
        return;

    // Get the Current Runtime
    int current_time = millis();

    // If Song is Not Finished, Early Return
    if  (current_time < song_end_time)
        return;

    // Set Flag That Song is Not Playing
    is_playing = false;

    // Stop the Current Song
    stopSingleTrack(song_index);
}

// Debug Function for Tsunami
void update_tsunami_debug(int value)
{
    // Send Command Based on Value With Volume of 10
    playTsunamiSound(value, 10);
}
