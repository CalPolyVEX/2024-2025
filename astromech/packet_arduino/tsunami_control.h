#ifndef TSUNAMI_CONTROL_H
#define TSUNAMI_CONTROL_H

// Note: The RC Receiver Will use the Top-Right Levers to
// Select and Play Sounds. The Top Lever Will be Used to Select
// a Sound, Where Pulling the Level Up then Pushing it Back to Center
// Will Increment a Value, and Pushing it Down and Pulling it Back to
// Center Will Decrement a Value. This Value, ranging from 1 to N, Where
// N is the Number of Loaded Tracks, Will Determine Which Track to Play.
// The Bottom Button, When Pulled Up, Will Trigger the Sound at The Index
// of the Value

// Address of the Tsunami
#define TSUNAMI_ADDRESS 0x13

// Address of the Amplifier
#define AMPLIFIER_ADDRESS 0x58

// The Length of Each Song (In Milliseconds)
#define MAIN_THEME_LENGTH         90000   // Index = 1
#define CANTINA_BAND_LENGTH       163500  // Index = 2
#define THRONE_ROOM_LENGTH        298500  // Index = 3
#define DUEL_OF_THE_FATES_LENGTH  115000  // Index = 7
#define IMPERIAL_MARCH_LENGTH     180500  // Index = 8
#define MANDALORIAN_THEME_LENGTH  143500  // Index = 9

// Send Command to Tsunami to Play a Sound
void playTsunamiSound(byte index, int volume);

// Send Command to Tsunami to Set Master Volume
void setTsunamiMasterVolume(int volume);

// Send Command to Tsunami to Stop Playing Specified Track
void stopSingleTrack(int index);

// Send Command to Tsunami to Stop playing all tracks
void stopTracks();

// Reset Values in the Amplifier
void resetAmplifier();

// Set the Gain of the Amplifier
void setAmplifierGain(byte gain);

// Test if a Song is Finished Playing
void testEndOfSong();

// Debug Function for Tsunami
void update_tsunami_debug(int value);

#endif