/**
 * Tangia Sun (tts47), Emily Speckhals (ems395), Max Klugherz (mck65)
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 16 ---> VGA Hsync
 *  - GPIO 17 ---> VGA Vsync
 *  - GPIO 18 ---> 330 ohm resistor ---> VGA Red
 *  - GPIO 19 ---> 330 ohm resistor ---> VGA Green
 *  - GPIO 20 ---> 330 ohm resistor ---> VGA Blue
 *  - RP2040 GND ---> VGA GND
 *
 */
 // ==========================================
 // === VGA graphics library
 // ==========================================
#include "vga_graphics.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
// // Our assembled programs:
// // Each gets the name <pio_filename.pio.h>
// ==========================================
// === protothreads globals
// ==========================================
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "string.h"
// protothreads header
#include "pt_cornell_rp2040_v1.h"
#include "assets.h"
#include "sound_effects.h"

// ==========================================

// =================================
// ========== SOUND SETUP ==========
// =================================

// Sound effects while music is playing
static volatile uint8_t music_playable = 1;

//DDS parameters
#define two32 4294967296.0 // 2^32
#define FS 40000

// interrupt units
volatile uint32_t increment_0 = 0;
volatile uint32_t array_ptr_0 = 0;
volatile uint32_t length_copy_val_0 = 0;

volatile uint32_t increment_1 = 0;
volatile uint32_t array_ptr_1 = 0;
volatile uint32_t length_copy_val_1 = 0;

// the DDS units:
volatile unsigned int phase_accum_main_0 = 0;
volatile unsigned int phase_accum_main_1 = 0;
volatile unsigned int phase_incr_main_0 = (440 * two32) / FS;
volatile unsigned int phase_incr_main_1 = (440 * two32) / FS;

// Create a repeating timer that calls repeating_timer_callback.
// If the delay is > 0 then this is the delay between the previous callback ending and the next starting.
// If the delay is negative (see below) then the next call to the callback will be exactly x us after the
// start of the call to the last callback
struct repeating_timer timer;

// Hand-calculated frequencies
#define A2 880.0 // 2x A-440
#define Gs2 830.6
#define G2 784.0  // 440.0 * pow(twelth_root_two, 10));
#define Fs2 740.0 // 440.0 * pow(twelth_root_two, 9));
#define F2 698.5  // 440.0 * pow(twelth_root_two, 8));
#define E2 659.3  // 440.0 * pow(twelth_root_two, 7));
#define Ds 622.3  // 440.0 * pow(twelth_root_two, 6));
#define D 587.3   // 440.0 * pow(twelth_root_two, 5));
#define Cs 554.3  // 440.0 * pow(twelth_root_two, 4));
#define C1 523.3  // 440.0 * pow(twelth_root_two, 3));
#define B 493.9   // 440.0 * pow(twelth_root_two, 2));
#define Bb 466.2  // +1
#define A 440.0   // A-440
#define Gs 415.3
#define G1 392.0 // 440.0 * pow(twelth_root_two, -2));
#define Fs 415.3
#define F 349.2
#define E1 329.6 // 440.0 * pow(twelth_root_two, -5));
#define Ds0 311.1
#define D0 293.2  // -7
#define Cs0 277.0 // -8
#define B0 247.0  // -10
#define A0 220.0
#define no_note 0 // no note

//Note sound and length arrays
int note_array_0[] = { A,  E1,  A, A, B, Cs, D, E2, E2, E2, F2, G2, A2, A2, A2, G2, F2, G2, F2, E2, E2, D, D, E2, F2, E2, D, C1, C1, D, E2, D, C1, B, B, Cs, Ds, Fs2, E2, A, E1, Cs0, Cs0, D0, Cs0, B0, A0, Cs0, E1, A, Cs, E2, A2, no_note, E2, E2, E2, E2, F2, F2, F2, E2, D, E2, Cs, A, E1, Cs0, E2, D, Bb, F, Bb, D, C1, D, E2, D, E2, D, C1, A, E1, A, C1, A, C1, D, C1, D, C1, B, Fs, Ds0, B0, Ds0, B0, Fs, Ds0, B, Fs, Ds, B, Fs2, Gs2, Fs2, Gs2, E2, B, Gs, E1 };
int note_length_array_0[] = { 12, 18, 6, 3, 3, 3,  3, 30, 6,  4,  4,  4,  30, 6,  4,  4,  4,  9,  3,  24, 12, 6, 3, 3,  24, 6,  6, 6, 3, 3, 24, 6, 6, 6, 3, 3,  24, 12,  48, 12,18, 6,   3,  3,  3,   3,  6,  3,   3, 3, 3,  3,  6,  3, 6, 4,    4,  4, 30, 6,  4,  4,  4, 9,  3,  9, 3, 12,  12, 3,  3, 3,  3, 3, 3, 3, 3,  12, 6, 6, 3, 3, 3, 3, 3, 3, 3, 3, 12,6, 6, 3,  3,   3,  3,  3,   3,  3,   3, 3,  3,  3, 3,  12, 18,   3,    3,  3, 3, 3,  15 };

int note_array_1[] = { no_note,   A,  E1,  A, A, B, Cs, D, E2, E2, E2, F2, G2, A2, A2, A2, G2, F2, G2, F2, E2, E2, D, D, E2, F2, E2, D, C1, C1, D, E2, D, C1, B, B, Cs, Ds, Fs2, E2, E1, E1, E1, E1, E1, E1, E1, E1 };
int note_length_array_1[] = { 384, 12, 18, 6, 3, 3, 3,  3, 30, 6,  4,  4,  4,  30, 6,  4,  4,  4,  9,  3,  24, 12, 6, 3, 3,  24, 6,  6, 6, 3, 3, 24, 6, 6, 6, 3, 3,  24, 12,  6,  3, 3, 6, 3, 3, 6, 3, 15 };

// Fix15 for note attacks
// Macros for fixed-point arithmetic (faster than floating point)
typedef signed int fix15;
#define multfix15(a, b) ((fix15)((((signed long long)(a)) * ((signed long long)(b))) >> 15))
#define float2fix15(a) ((fix15)((a)*32768.0))
#define fix2float15(a) ((float)(a) / 32768.0)
#define absfix15(a) abs(a)
#define int2fix15(a) ((fix15)(a << 15))
#define fix2int15(a) ((int)(a >> 15))
#define char2fix15(a) (fix15)(((fix15)(a)) << 15)
#define divfix(a, b) (fix15)((((signed long long)(a)) << 15) / (b))

//SPI configurations
#define PIN_MISO 4
#define PIN_CS 5
#define PIN_SCK 6
#define PIN_MOSI 7
#define SPI_PORT spi0

// Amplitude modulation parameters and variables
fix15 max_amplitude = int2fix15(1); // maximum amplitude
fix15 attack_inc;                   // rate at which sound ramps up
fix15 current_amplitude_1 = 0;      // current amplitude (modified in ISR)
fix15 current_amplitude_0 = 0;
int ATTACK_TIME = 1000; // This is tunable
int DECAY_TIME = 1000;

int attack_count_0 = 0;
int attack_count_1 = 0;

// SPI data
uint16_t DAC_data; // output value
uint16_t DAC_data_1;

// DDS lookup table
#define lookup_table_size 256
volatile int lookup_table[lookup_table_size];

//DAC parameters
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000

void buildLookupTable() {
  // === build the ~squareish~ lookup table =======
  // Manual reconstruction of buzzer waves from NES chiptune output
  // scaled to produce values between 0 and 4096
  int ii;
  for (ii = 0; ii < lookup_table_size; ii++) {
    if (ii < 18) {
      lookup_table[ii] = (int)(ii * 2047.0 / 18.0);
    }
    else if (ii < 110) {
      lookup_table[ii] = (int)(-1 * (ii - 18) * 512.0 / 92) + 2047;
    }
    else if (ii < 146) {
      lookup_table[ii] = (int)(-1 * (ii - 110) * 1792.0 / 18) + 1536;
    }
    else if (ii < 238) {
      lookup_table[ii] = (int)((ii - 146) * 512.0 / 92) - 2048;
    }
    else {
      lookup_table[ii] = (int)((ii - 238) * 1536.0 / 18.0) - 1536;
    }
  }

  attack_inc = divfix(max_amplitude, int2fix15(ATTACK_TIME));
  length_copy_val_0 = note_length_array_0[array_ptr_0];
  phase_incr_main_0 = (note_array_0[array_ptr_0] * two32) / FS;

  length_copy_val_1 = note_length_array_1[array_ptr_1];
  phase_incr_main_1 = (note_array_1[array_ptr_1] * two32) / FS;
}

// Timer ISR
bool repeating_timer_callback(struct repeating_timer* t) {

  if (music_playable) {
    // Change phase_incr_main wrt array of notes and lengths
    // each subdivision of a beat occurs at (130 beats/min * 1 min/60 sec)^-1 * 40,000 iterates/sec * 1/12 beats/subdivision
    if (increment_0 > 2500) {

      increment_0 = 0; // Reset until 12th of a beat occurs
      length_copy_val_0 -= 1; // Decrease number of subdivisons remaining

      // No subdivisions remaining; move on to the next note
      // reset attack; move to next note in array(s); change the phase_incr corresponding to new note
      if (length_copy_val_0 <= 0) {
        current_amplitude_0 = 0;
        attack_count_0 = 0;
        array_ptr_0 += 1;
        length_copy_val_0 = note_length_array_0[array_ptr_0];
        phase_incr_main_0 = (note_array_0[array_ptr_0] * two32) / FS;
      }
    }

    // If at end of the array
    // Loop back around
    if (array_ptr_0 >= sizeof(note_array_0) / sizeof(note_array_0[0])) {
      array_ptr_0 = 0;
      length_copy_val_0 = note_length_array_0[0];
      phase_incr_main_0 = (note_array_0[0] * two32) / FS;
    }

    // Scale with attack increment
    // Mimics the attack rampup of a new note
    if (attack_count_0 < ATTACK_TIME) {
      current_amplitude_0 = (current_amplitude_0 + attack_inc);
    }
    else if (attack_count_0 > note_length_array_0[array_ptr_0] * 2500 - DECAY_TIME) {
      current_amplitude_0 = (current_amplitude_0 - attack_inc);
    }

    // DDS phase and sine table lookup
    phase_accum_main_0 += phase_incr_main_0;
    int DAC_output = (fix2int15(multfix15(current_amplitude_0, int2fix15(lookup_table[phase_accum_main_0 >> 24]))) + 2048) / 8;
    // DAC_data = (DAC_config_chan_A | (DAC_output & 0xfff));

    // spi_write16_blocking(SPI_PORT, &DAC_data, 1);

    attack_count_0 += 1;
    increment_0 += 1;

    // =======================================================================================================================
    // Channel 1:
    // These should occur in the same in the same timer isr (it's easiest)

    // Change phase_incr_main wrt array of notes and lengths
    // each subdivision of a beat occurs at (130 beats/min * 1 min/60 sec)^-1 * 40,000 iterates/sec * 1/12 beats/subdivision
    if (increment_1 > 2500) {

      increment_1 = 0; // Reset until 12th of a beat occurs
      length_copy_val_1 -= 1; // Decrease number of subdivisons remaining

        // No subdivisions remaining; move on to the next note
        // reset attack; move to next note in array(s); change the phase_incr corresponding to new note
      if (length_copy_val_1 <= 0) {
        current_amplitude_1 = 0;
        attack_count_1 = 0;
        array_ptr_1 += 1;
        length_copy_val_1 = note_length_array_1[array_ptr_1];
        phase_incr_main_1 = (note_array_1[array_ptr_1] + 1 * two32) / FS;
      }
    }

    // If at end of the array
    // Loop back around
    if (array_ptr_1 >= sizeof(note_array_1) / sizeof(note_array_1[0])) {
      array_ptr_1 = 0;
      length_copy_val_1 = note_length_array_1[0];
      phase_incr_main_1 = (note_array_1[0] + 1 * two32) / FS;
    }

    // Scale with attack and decay
    // Mimics the attack rampup of a new note
    if (attack_count_1 < ATTACK_TIME) {
      current_amplitude_1 = (current_amplitude_1 + attack_inc);
    }
    else if (attack_count_1 > note_length_array_1[array_ptr_1] * 2500 - DECAY_TIME) {
      current_amplitude_1 = (current_amplitude_1 - attack_inc);
    }

    // DDS phase and sine table lookup
    phase_accum_main_1 += phase_incr_main_1;
    int DAC_output_1 = (fix2int15(multfix15(current_amplitude_1, int2fix15(lookup_table[phase_accum_main_1 >> 24]))) + 2048) / 8;
    // DAC_data_1 = (DAC_config_chan_B | (DAC_output_1 & 0xfff));

    DAC_data_1 = (((DAC_output & 0xfff) + (DAC_output_1 & 0xfff)) >> 4) | DAC_config_chan_B;

    spi_write16_blocking(SPI_PORT, &DAC_data_1, 1);


    attack_count_1 += 1;
    increment_1 += 1;

    if (array_ptr_1 > 48 || array_ptr_0 > 111) {
      array_ptr_1 = 0;
      array_ptr_0 = 0;
    }
  }
  return true;
}

// Enum and arrays referencing sound effects
enum sound_effects {
  boulder_sound = 0,
  get_hit_sound,
  get_rupee_sound,
  hit_enemy_sound,
  important_item_sound,
  kill_enemy_sound,
  link_die_sound,
  sword_slash_sound,
  walk_sound,
  wall_bump_sound
};

int sound_effect_sizes[] = { 8545, 6316, 6594, 4922, 9659, 6780, 17399, 6315, 2176, 5666 };
const short* sound_effect_references[] = {
    &boulder_8545[0],
    &get_hit_6316[0],
    &get_rupee_6594[0],
    &hit_enemy_4922[0],
    &important_item_9659[0],
    &kill_enemy_6780[0],
    &link_die_17399[0],
    &sword_slash_6315[0],
    &walk_2176[0],
    &wall_bump_5666[0]
};


static int data_chan = 3;
static dma_channel_config c2;

// ==========================================
// ========== GAME INITIALIZATIONS ==========
// ==========================================

// Initializing for arrays
#define NUM_ENEMIES 8                                           // Maximum Number of Enemies
#define NUM_ENTITIES 20                                         // Maximum Number of Entities (Boulders, Gems, etc)
#define SCREEN_TRAN 6                                           // Maxmium Number of Screen Transitions (Extra for Stairs)
#define NUM_INTERACTABLES (NUM_ENTITIES + NUM_ENEMIES + 1 + SCREEN_TRAN) // Additional 1 for MC

// Defining Times for cooldowns for attacks, periodic timers, etc
#define enemy_move_cd_us 100000
#define death_cloud_cd_us 100000
#define periodic_timer 5000

// Attack sequence cooldowns
#define sword_out_cd 150000
#define input_enable_cd 200000
#define attack_again_cd 350000
#define input_cd 50000

// Semaphores for drawing the game and the game logic
struct pt_sem graphic_go, game_go;

// ===== ENUMS =====
// This enum represents the state of the character.
enum des_state_index {
  FRONT,
  BACK,
  RIGHT,
  LEFT,
  FRONT_2,
  BACK_2,
  RIGHT_2,
  LEFT_2,
  F_ATTACK,
  B_ATTACK,
  R_ATTACK,
  L_ATTACK,
};

// The state of the character
enum main_char_index {
  FULL,
  SWORD,
  SHIELD,
  NEITHER,
};

// The list of enemies that can be drawn
enum enemy_name {
  DEAD,
  GUARDIAN,
  SPIDER
};

// The direction of the sword so it can be written
enum sword_dir {
  SWORD_D,
  SWORD_U,
  SWORD_R,
  SWORD_L
};

// The list of entities that can exist and the screen transition
enum entity {
  NOTHING,
  BOULDER,
  GEM,
  SIGN,
  SC_TR,
  PED_SW,
  PED_NO_SW,
  BOULDER_RED,
  GEM_RED,
  HEART,
  VILLAGER
};

// ========== MAIN CHARACTER ==========

// List of pointers that point to the top designs of the main character
const long long* main_char_full_des[] = { &MCHAR_F_DES[0][0], &MCHAR_B_DES[0][0], &MCHAR_R_DES[0][0], &MCHAR_L_DES[0][0], &MCHAR_F_WALK_DES[0][0], &MCHAR_B_WALK_DES[0][0], &MCHAR_R_WALK_DES[0][0], &MCHAR_L_WALK_DES[0][0], &MCHAR_ATT_F_DES[0][0], &MCHAR_ATT_B_DES[0][0], &MCHAR_ATT_R_DES[0][0], &MCHAR_ATT_L_DES[0][0] };
// List of pointers that point to the mask of the top designs of the main character
const char* main_char_full_mask[] = { &MCHAR_F_MASK[0][0], &MCHAR_B_MASK[0][0], &MCHAR_R_MASK[0][0], &MCHAR_L_MASK[0][0],  &MCHAR_F_MASK[0][0], &MCHAR_B_MASK[0][0], &MCHAR_R_WALK_MASK[0][0], &MCHAR_L_WALK_MASK[0][0], &MCHAR_ATT_F_MASK[0][0], &MCHAR_ATT_B_MASK[0][0], &MCHAR_ATT_R_MASK[0][0], &MCHAR_ATT_L_MASK[0][0] };

// List of pointers to pointers that determine the state of the character
const long long** main_char_index[] = { &main_char_full_des[0] };
// Masks
const char** main_char_mask_index[] = { &main_char_full_mask[0] };

// ========== ENEMIES ==========

// ----- GUARDIAN -----
// List of pointers that point to the top designs of a specific enemy
const long long* guardian_des[] = { &GUARDIAN_F_DES[0][0], &GUARDIAN_B_DES[0][0], &GUARDIAN_R_DES[0][0], &GUARDIAN_L_DES[0][0] };
const long long* spider_des[] = { &SPIDER_F_DES[0][0], &SPIDER_F_DES[0][0], &SPIDER_F_DES[0][0], &SPIDER_F_DES[0][0] };
// CLOUD FOR WHEN ENEMY DIES
const long long* death_anim[] = { &CLOUD_DES[0][0] };

// List of pointers that point to the mask of the top designs of the main character
const char* guardian_mask[] = { &GUARDIAN_F_MASK[0][0], &GUARDIAN_B_MASK[0][0], &GUARDIAN_R_MASK[0][0], &GUARDIAN_L_MASK[0][0] };
const char* spider_mask[] = { &SPIDER_F_MASK[0][0], &SPIDER_F_MASK[0][0], &SPIDER_F_MASK[0][0], &SPIDER_F_MASK[0][0] };
// CLOUD FOR WHEN ENEMY DIES
const char* death_anim_mask[] = { &CLOUD_MASK[0][0] };

// List of all enemies that point to the enemy designs
const long long** enemy_index[] = { &death_anim[0], &guardian_des[0], &spider_des[0] };
// Masks
const char** enemy_mask_index[] = { &death_anim_mask[0], &guardian_mask[0], &spider_mask[0] };

// ========== WEAPON DESIGNS ==========

// List of pointers that point to the top designs of a direction for the sword
const long long* weapon_des[] = { &SW_DOWN_DES[0][0], &SW_UP_DES[0][0], &SW_RIGHT_DES[0][0], &SW_LEFT_DES[0][0] };
// Masks
const char* weapon_mask[] = { &SW_DOWN_MASK[0][0], &SW_UP_MASK[0][0], &SW_RIGHT_MASK[0][0], &SW_LEFT_MASK[0][0] };

// ========== ENVIRONMENT/ENTITIES ==========

// List of pointers that point to the design of tiles that make up the screen
const long long* block_map[] = { &WALL_TILE[0][0], &TREE_TILE[0][0], &GRASS_TILE[0][0], &WATER_TILE[0][0], &BED_TILE[0][0], &STAIRS_DES[0][0], &LADDER_DES[0][0], &CAVE_TILE[0][0], &CAVE_OBJ[0][0], &CAVE_WALL[0][0], &WALL_RED_TILE[0][0], &TREE_2_TILE[0][0], &BOARDWALK_DES[0][0] };
// List of pointers that point to the designs of entities that sit on top of the screen
const long long* entity_map[] = { &NOTHING_TILE[0][0], &BOULDER_TILE[0][0], &GEM_TILE[0][0], &SIGN_DES[0][0], &NOTHING_TILE[0][0], &END_SW_DES[0][0] , &END_PEDESTAL_DES[0][0], &BOULDER_RED_TILE[0][0], &GEM_RED_TILE[0][0] , &HEART_DES[0][0], &VILLAGER_DES[0][0] };
// Masks
const char* entity_mask[] = { &NOTHING_MASK[0][0], &BOULDER_MASK[0][0], &GEM_MASK[0][0], &SIGN_MASK[0][0] ,  &NOTHING_MASK[0][0], &END_SW_MASK[0][0], &END_PEDESTAL_MASK[0][0], &BOULDER_MASK[0][0], &GEM_RED_MASK[0][0], &HEART_MASK[0][0], &VILLAGER_MASK[0][0] };

// ========== LEVEL/SCREEN DESIGN ==========
// A level is a full stage that has 3 x 3 screens

/* Naming Format:
SCREEN(level#)(screen#)
S(level#)(screen#)_IAS     *i nteractables
*/

#define LEVEL_HEIGHT 3
#define LEVEL_WIDTH 3

// ----- Each level -----
// Single Sequence Events
char SSE[LEVEL_HEIGHT * LEVEL_WIDTH] = { 0 };

// List of pointers that point to level background designs.
const long long* LEVEL0[LEVEL_HEIGHT * LEVEL_WIDTH] = {
    &S02_GETSW[0][0], &S01_HALL1[0][0], &S07_REWARD[0][0],
    &S08_END[0][0], &S00_WAKEUP[0][0], &S06_PUZZLE[0][0],
    &S04_BOSS[0][0], &S03_HALL2[0][0], &S05_HALL3[0][0] };

// List of pointers that point to the interactables on that specific screen
const short* S0IAS[LEVEL_HEIGHT * LEVEL_WIDTH] = {
    &S02_IAS[0], &S01_IAS[0], &S07_IAS[0],
    &S08_IAS[0], &S00_IAS[0], &S06_IAS[0],
    &S04_IAS[0], &S03_IAS[0], &S05_IAS[0] };

// List of pointers that point to the walls/obstacles of that specific screen
const short* LEVEL0_WALL[LEVEL_HEIGHT * LEVEL_WIDTH] = {
    &S02_WALL[0], &S01_WALL[0], &S07_WALL[0],
    &S08_WALL[0], &S00_WALL[0], &S06_WALL[0],
    &S04_WALL[0], &S03_WALL[0], &S05_WALL[0] };

// ----- Each 3x3 Level -----
const long long** level_index[] = { &LEVEL0[0] }; // incomplete

// ========================================
// ========== STRUCT/DECLARATION ==========
// ========================================

// Struct that represents each character
typedef struct {
  char char_des_index_value;  // Which character model?
  char des_state_index_value; // State of that character?
  char wall_detect;           // Where can/can't the character go? 0b(TOP)(BOT)(LEF)(RIT)
  signed char pos[4];         // [x_global_pos, x_tile_pos, y_global_pos, y_tile_pos]
  char heart_stat[2];         // [curr health, max health]
  char size[2];               // [x_size, y_size] -- In blocks
  char move_speed;            // Amount of blocks moved per movement
  char weapon;                // Weapon only relevant to MC
} char_type;

// Struct for the state of the game - processing overhead
typedef struct {
  char curr_level;        // Level index (collection of screens)
  char curr_screen[2];    // [screen_x_pos, screen_y_pos]
  char gems;              // Number of gems collected
  char interact;          // Character interacts with what is in front of them
  char enemies_on_screen; // Whether or not to periodically refresh the screen
  char pause;             // When the game logic is paused (i.e. reading a sign)
  char MC_start;          // Only for initializing the main character's initial position. Starts at 1, turns to 0
  char respawn_pos[2];    // Where to respawn the character on the screen if you die
  int num_deaths;
  char game_start;
} game_type;

// Struct for the interactables on the screen
typedef struct {
  char wall_detect;   // Information Variable -> information varies with the entity
  char index_value;   // Which entity is this?
  signed char pos[4]; // [x_global_pos, x_tile_pos, y_global_pos, y_tile_pos]
  char size[2];       // [x_size, y_size] -- In blocks
} interactable_type;

// Struct for attacking with the weapon
typedef struct {
  char attacking; // Whether the weapon is attacking or not
  char dir_index; // Which direction the weapon is attacking
  char attack_reg;
  signed int pos[2];     // [x_specific_pos, y_specific_pos] -- In blocks
} weapon_type;

// Struct for handling time variables - like "thresholds" for when something is active till
typedef struct {
  int current_time;      // Current time
  int enemy_move_rdy;    // How often the enemies move
  int death_cloud;       // How long the death cloud stays
  int frame_rate_time;   // Natural refresh rate of the game for checking periodic
  int attack_again_time; // When the attack can come out again
  int sword_out_time;    // When the sword hitbox retracts
  int input_enable_time; // When the player can input again
  int input_time;
} time_type;

// ----- Creating structs and lists and static variables -----
time_type time;
weapon_type weapon;
char_type main_char;
volatile game_type game_state;
char_type enemies[NUM_ENEMIES];
interactable_type entities[NUM_ENTITIES];
interactable_type sc_list[SCREEN_TRAN];

// Random Variables Needed for Threads
static char sw_x_pos;
static char sw_y_pos;
static char sw_w;
static char sw_h;
static signed char enemy_hit;

// =====================================
// ========== SOUND FUNCTIONS ==========
// =====================================


void initialize_sound_effect(int selection) {
  // If DMA ch3 (i.e. configuration c2) is NOT currently busy, see rp2040 datasheet page 113
  if (!((*(volatile uint32_t*)(DMA_BASE + 0x0d0)) & 0x01000000)) {
    dma_channel_configure(
      data_chan,                          // Channel to be configured
      &c2,                                // The configuration we just created
      &spi_get_hw(SPI_PORT)->dr,          // write address (SPI data register)
      sound_effect_references[selection], // The initial read address
      sound_effect_sizes[selection],      // Number of transfers
      true                                // DO start immediately.
    );
  }
  else if (selection != walk_sound) {
    // Clear channel interrupt enable
    dma_channel_set_irq0_enabled(data_chan, false);
    // Abort
    dma_channel_abort(data_chan);
    // Clear the irq (if there was one)
    dma_channel_acknowledge_irq0(data_chan);
    // Re-enable channel interrupt enable
    dma_channel_set_irq0_enabled(data_chan, true);

    busy_wait_ms(1);

    dma_channel_configure(
      data_chan,                          // Channel to be configured
      &c2,                                // The configuration we just created
      &spi_get_hw(SPI_PORT)->dr,          // write address (SPI data register)
      sound_effect_references[selection], // The initial read address
      sound_effect_sizes[selection],      // Number of transfers
      true                                // DO start immediately.
    );
  }
}

void stop_music() {
  // Stop operations inside timer ISR
  music_playable = 0;

  // Channel 0 parameters
  array_ptr_0 = 0;
  increment_0 = 0;
  current_amplitude_0 = 0;
  attack_count_0 = 0;

  length_copy_val_0 = note_length_array_0[array_ptr_0];
  phase_incr_main_0 = (note_array_0[array_ptr_0] * two32) / FS;

  // Channel 1 parameters
  array_ptr_1 = 0;
  increment_1 = 0;
  current_amplitude_1 = 0;
  attack_count_1 = 0;

  length_copy_val_1 = note_length_array_1[array_ptr_1];
  phase_incr_main_1 = (note_array_1[array_ptr_1] * two32) / FS;
}

void restart_music() {
  music_playable = 1;
}

// ============================================
// ========== FUNCTIONS FOR MOVEMENT ==========
// ============================================
/* passable_entity(value) returns 1 if the entity should not be passed through
and returns 0 if the entity can be passed through. */
char passable_entity(char value) {
  if ((value == BOULDER) || (value == BOULDER_RED) || (value == SIGN) || (value == PED_SW) || (value == PED_NO_SW) || (value == VILLAGER)) return 1;
  else return 0;
}

/* gen_wall_mask(C) generates the wall mask for C based on the SCREEN_WALL of
the current screen, signs, and boulders and places it in C->wall_detect. It
will consider the adjacent tile if the character is not centered */
void gen_wall_mask(char_type* C) {
  // First reset the position
  C->wall_detect = 0; // 0b(TOP)(BOT)(LEF)(RIT)
  // Get the pointer to the SCREEN_WALL
  const short* mask_pt = LEVEL0_WALL[game_state.curr_screen[0] + game_state.curr_screen[1] * LEVEL_HEIGHT];
  // Obtain the horizontal and vertical position of the character on the wall array
  short v_mask = (1 << (15 - C->pos[0])); // Horizontal Position : 0b0000010000
  short h_row = *(mask_pt + C->pos[2]);   // Which row the character is in the array

  //Checks if there is a wall one row above C
  if ((v_mask & *(mask_pt + C->pos[2] - 1)) || (C->pos[2] <= 0)) {
    C->wall_detect |= 0b01000000;
  }
  // If the character is off-centered on the x position, need to check adjacent tiles
  if (C->pos[1] != 0) {
    if (v_mask & (*(mask_pt + C->pos[2] - 1) << 1)) { // Check if the character is partially below
      C->wall_detect |= 0b01000000;
    }
    if (v_mask & (*(mask_pt + C->pos[2] + 1) << 1)) { // Checks if the character is partially above
      C->wall_detect |= 0b00010000;
    }
  }
  // Checks if there is a wall one row below C; Don't need extra conditionals because referencing the top-left of the character
  if ((v_mask & *(mask_pt + C->pos[2] + 1)) || (C->pos[2] >= 10)) {
    C->wall_detect |= 0b00010000;
  }

  //Checks if there is a wall one column left of C
  if ((h_row & (1 << (15 - C->pos[0] + 1))) || (C->pos[0] <= 0)) {
    C->wall_detect |= 0b00000100;
  }
  // If the character is off-centered on the y position, need to check adjacent tiles
  if (C->pos[3] != 0) {
    if (*(mask_pt + C->pos[2] + 1) & (1 << (15 - C->pos[0] + 1))) { // Check if the character is partially to the right
      C->wall_detect |= 0b00000100;
    }
    if (*(mask_pt + C->pos[2] + 1) & (1 << (15 - C->pos[0] - 1))) { // Checks if the character is partially to the left
      C->wall_detect |= 0b00000001;
    }
  }
  // Checks if there is a wall one column right of C; Don't need extra conditionals because referencing the top-left of the character
  if ((h_row & (1 << (15 - C->pos[0] - 1))) || (C->pos[0] >= 15)) {
    C->wall_detect |= 0b00000001;
  }

  // Checks the entities to generate additional wall_detect
  for (int i = 0; i < NUM_ENTITIES; i++) {
    if (passable_entity(entities[i].index_value)) {
      signed char x_dist = C->pos[0] - entities[i].pos[0]; // Calculating the distance in x coordinates
      signed char y_dist = C->pos[2] - entities[i].pos[2]; // Calculating the distance in y coordinates
      // If the x coord is lined up and the y is close
      if ((x_dist == 0) && (abs(y_dist) < 2)) {
        if (y_dist > 0) { // If character is below the entity
          C->wall_detect |= 0b01000000;
        }
        else { // If character is above entity
          C->wall_detect |= 0b00010000;
        }
      }
      // If the y coord is lined up and the x is close
      if ((y_dist == 0) && (abs(x_dist) < 2)) {
        if (x_dist > 0) { // If character is to the right of entity
          C->wall_detect |= 0b00000100;
        }
        else { // If character is to the left of entity
          C->wall_detect |= 0b00000001;
        }
      }
      // Need to consider cases:
      // If the character is off-centered on the x ition, need to check adjacent tiles
      if (C->pos[1] != 0) {
        if (x_dist == -1) { // If character is above:
          if (y_dist == -1) { // Check if below entity
            C->wall_detect |= 0b00010000;
          }
          else if (y_dist == 1) { // Check if above entity
            C->wall_detect |= 0b01000000;
          }
        }
      }
      // If the character is off-centered on the y position, need to check adjacent tiles
      if (C->pos[3] != 0) {
        if (y_dist == -1) { // If character is to the left
          if (x_dist == -1) { // Check if to the left of entity
            C->wall_detect |= 0b00000001;
          }
          else if (x_dist == 1) { // Check if to the right of entity
            C->wall_detect |= 0b00000100;
          }
        }
      }
    }
  }
}

// VERY SIMILAR FUNCTION BUT DIFFERENT INPUT
/* gen_wall_mask_IC(I) generates the wall mask for interactable_type I based
on the SCREEN_WALL ofthe current screen, signs, and boulders and places it in I->wall_detect. It
will consider the adjacent tile if the interactale is not centered */
void gen_wall_mask_IC(interactable_type* I) {
  // First reset the position
  I->wall_detect = 0; // 0b(TOP)(BOT)(LEF)(RIT)
  // Get the pointer to the SCREEN_WALL
  const short* mask_pt = LEVEL0_WALL[game_state.curr_screen[0] + game_state.curr_screen[1] * LEVEL_HEIGHT];
  // Obtain the horizontal and vertical position of the character on the wall array
  short v_mask = (1 << (15 - I->pos[0])); // Horizontal Position : 0b0000010000
  short h_row = *(mask_pt + I->pos[2]);   // Which row the character is in the array

  // Checks if there is a wall one row above I
  if ((v_mask & *(mask_pt + I->pos[2] - 1)) || I->pos[2] <= 0) {
    I->wall_detect |= 0b01000000;
  }
  // If the interactable is off-centered on the x position, need to check adjacent tiles
  if (I->pos[1] != 0) {
    if (v_mask & (*(mask_pt + I->pos[2] - 1) << 1)) { // Check if the interactable is partially below
      I->wall_detect |= 0b01000000;
    }
    if (v_mask & (*(mask_pt + I->pos[2] + 1) << 1)) { // Checks if the character is partially above
      I->wall_detect |= 0b00010000;
    }
  }
  // Checks if there is a wall one row below I; Don't need extra conditionals because referencing the top-left of the interactable
  if ((v_mask & *(mask_pt + I->pos[2] + 1)) || I->pos[2] >= 10) {
    I->wall_detect |= 0b00010000;
  }

  //Checks if there is a wall one column left of I
  if ((h_row & (1 << (15 - I->pos[0] + 1))) || I->pos[0] <= 0) {
    I->wall_detect |= 0b00000100;
  }
  // If the interactable is off-centered on the y position, need to check adjacent tiles
  if (I->pos[3] != 0) {
    if (*(mask_pt + I->pos[2] + 1) & (1 << (15 - I->pos[0] + 1))) { // Check if the interactable is partially to the right
      I->wall_detect |= 0b00000100;
    }
    if (*(mask_pt + I->pos[2] + 1) & (1 << (15 - I->pos[0] - 1))) { // Checks if the interactable is partially to the left
      I->wall_detect |= 0b00000001;
    }
  }
  // Checks if there is a wall one column right of I; Don't need extra conditionals because referencing the top-left of the interactable
  if ((h_row & (1 << (15 - I->pos[0] - 1))) || I->pos[0] >= 15) {
    I->wall_detect |= 0b00000001;
  }

  // Checks the entities to generate additional wall_detect
  for (int i = 0; i < NUM_ENTITIES; i++) {
    if (passable_entity(entities[i].index_value) && (&entities[i] != I)) {
      signed char x_dist = I->pos[0] - entities[i].pos[0]; // Calculating the distance in x coordinates
      signed char y_dist = I->pos[2] - entities[i].pos[2]; // Calculating the distance in y coordinates
      // If the x coord is lined up and the y is close
      if ((x_dist == 0) && (abs(y_dist) < 2)) {
        if (y_dist > 0) { // If interactable is below the entity
          I->wall_detect |= 0b01000000;
        }
        else { // If interactable is above entity
          I->wall_detect |= 0b00010000;
        }
      }
      // If the y coord is lined up and the x is close
      if ((y_dist == 0) && (abs(x_dist) < 2)) {
        if (x_dist > 0) { // If interactable is to the right of entity
          I->wall_detect |= 0b00000100;
        }
        else { // If interactable is to the left of entity
          I->wall_detect |= 0b00000001;
        }
      }
      // Need to consider cases:
      // If the interactable is off-centered on the x position, need to check adjacent tiles
      if (I->pos[1] != 0) {
        if (x_dist == -1) { // If character is above:
          if (y_dist == -1) { // Check if below entity
            I->wall_detect |= 0b00010000;
          }
          else if (y_dist == 1) { // Check if above entity
            I->wall_detect |= 0b01000000;
          }
        }
      }
      // If the character is off-centered on the y position, need to check adjacent tiles
      if (I->pos[3] != 0) {
        if (y_dist == -1) { // If character is to the left
          if (x_dist == -1) { // Check if to the left of entity
            I->wall_detect |= 0b00000001;
          }
          else if (x_dist == 1) { // Check if to the right of entity
            I->wall_detect |= 0b00000100;
          }
        }
      }
    }
  }
}


/* force_move_char_ver(y_mov, C) will move C vertically by y_mov and will obey walls */
void force_move_char_ver(short y_mov, char_type* C) {
  if (y_mov > 0) {    // If moving down, check if it can move down against mask
    if (C->wall_detect & 0b00110000) {
      return;
    }
  }
  else if (y_mov < 0) { // If moving up, check if it can move down, but need to check if actually at edge
    if ((C->wall_detect & 0b11000000) && (C->pos[3] == 0)) {
      C->pos[3] = 0;
      return;
    }
  }
  // Changing position
  C->pos[3] += y_mov;
  // If exiting a local tile, change global tile
  if (C->pos[3] > 15) {
    C->pos[3] -= 16;
    C->pos[2] += 1;
  }
  else if (C->pos[3] < 0) {
    C->pos[3] += 16;
    C->pos[2] -= 1;
  }
  // Generates a new wall mask
  gen_wall_mask(C);
}

/* force_move_char_hor(x_mov, C) will move C horizontally by x_mov and will obey walls */
void force_move_char_hor(short x_mov, char_type* C) {
  if (x_mov > 0) {      // If moving right, check if it can move right against mask
    if (C->wall_detect & 0b00000011) {
      return;
    }
  }
  else if (x_mov < 0) { // If moving left, check if it can move left against mask
    if ((C->wall_detect & 0b00001100) && (C->pos[1] == 0)) {
      C->pos[1] = 0;
      return;
    }
  }
  // Changing position
  C->pos[1] += x_mov;
  // If exiting a local tile, change global tile
  if (C->pos[1] > 15) {
    C->pos[1] -= 16;
    C->pos[0] += 1;
  }
  else if (C->pos[1] < 0) {
    C->pos[1] += 16;
    C->pos[0] -= 1;
  }
  // Generates a new wall mask
  gen_wall_mask(C);
}

/* move_char_hor(x_mov, C) will move C horizontally by x_mov and will change the
direction the character is looking. It will obey walls and will return if C
bumps into a wall determined by wall_detect */
void move_char_hor(short x_mov, char_type* C) {
  if (x_mov > 0) {    // If moving right, check if it can move right against mask
    if (C == &main_char) {
      initialize_sound_effect(walk_sound);
      if (C->des_state_index_value == RIGHT) {
        C->des_state_index_value = RIGHT_2;
      }
      else {
        C->des_state_index_value = RIGHT;
      }
    }
    else {
      C->des_state_index_value = RIGHT; // Turning the character right
    }
    if (C->wall_detect & 0b00000011) {
      // Don't make sound for anything other than the MC
      if (C == &main_char) {
        initialize_sound_effect(wall_bump_sound);
      }
      return;
    }
  }
  else if (x_mov < 0) { // If moving left, check if it can move left against mask
    if (C == &main_char) {
      initialize_sound_effect(walk_sound);
      if (C->des_state_index_value == LEFT) {
        C->des_state_index_value = LEFT_2;
      }
      else {
        C->des_state_index_value = LEFT;
      }
    }
    else {
      C->des_state_index_value = LEFT; // Turning the character left
    }
    if ((C->wall_detect & 0b00001100) && (C->pos[1] <= 0)) {
      // Don't make sound for anything other than the MC
      if (C == &main_char) {
        C->pos[1] = 0;
        initialize_sound_effect(wall_bump_sound);
      }
      return;
    }
  }
  C->pos[1] += x_mov;
  if (C->pos[1] > 15) {
    C->pos[1] -= 16;
    C->pos[0] += 1;
  }
  else if (C->pos[1] < 0) {
    C->pos[1] += 16;
    C->pos[0] -= 1;
  }
  gen_wall_mask(C);
}

/* move_char_ver(y_mov, C) will move C vertically by y_mov and will change the
direction the character is looking. It will obey walls and will return if C
bumps into a wall determined by wall_detect */
void move_char_ver(short y_mov, char_type* C) {
  if (y_mov > 0) {    // If moving down, check if it can move down against mask
    if (C == &main_char) {
      initialize_sound_effect(walk_sound);
      if (C->des_state_index_value == FRONT) {
        C->des_state_index_value = FRONT_2;
      }
      else {
        C->des_state_index_value = FRONT;
      }
    }
    else {
      C->des_state_index_value = FRONT; // Turning the character forward
    }
    if (C->wall_detect & 0b00110000) {
      // Don't make sound for anything other than the MC
      if (C == &main_char) {
        initialize_sound_effect(wall_bump_sound);
      }
      return;
    }
  }
  else if (y_mov < 0) {
    if (C == &main_char) {
      initialize_sound_effect(walk_sound);
      if (C->des_state_index_value == BACK) {
        C->des_state_index_value = BACK_2;
      }
      else {
        C->des_state_index_value = BACK;
      }
    }
    else {
      C->des_state_index_value = BACK; // Turning the character forward
    }
    if ((C->wall_detect & 0b11000000) && (C->pos[3] <= 0)) {
      // Don't make sound for anything other than the MC
      C->pos[3] = 0;
      if (C == &main_char) {
        initialize_sound_effect(wall_bump_sound);
      }
      return;
    }
  }
  // Changing position
  C->pos[3] += y_mov;
  // If exiting a local tile, change global tile
  if (C->pos[3] > 15) {
    C->pos[3] -= 16;
    C->pos[2] += 1;
  }
  else if (C->pos[3] < 0) {
    C->pos[3] += 16;
    C->pos[2] -= 1;
  }
  // Generates a new wall mask
  gen_wall_mask(C);
}

/* force_mov_IC_ver(y_mov, I) will move I vertically by y_mov and will obey walls. */
void force_move_IC_ver(short y_mov, interactable_type* I) {
  if (y_mov > 0) {    // If moving down, check if it can move down against mask
    if (I->wall_detect & 0b00110000) {
      return;
    }
  }
  else if (y_mov < 0) { // If moving up, check if it can move down, but need to check if actually at edge
    if ((I->wall_detect & 0b11000000) && ((I->pos[3] + y_mov) < 0)) {
      I->pos[3] = 0;
      return;
    }
  }
  // Changing position
  I->pos[3] += y_mov;
  // If exiting a local tile, change global tile
  if (I->pos[3] > 15) {
    I->pos[3] -= 16;
    I->pos[2] += 1;
  }
  else if (I->pos[3] < 0) {
    I->pos[3] += 16;
    I->pos[2] -= 1;
  }
  // Generates a new wall mask
  gen_wall_mask_IC(I);
}

/* force_move_IC_hor(x_mov, I) will move I horizontally by x_mov and will obey walls. */
void force_move_IC_hor(short x_mov, interactable_type* I) {
  if (x_mov > 0) {      // If moving right, check if it can move right against mask
    if (I->wall_detect & 0b00000011) {
      return;
    }
  }
  else if (x_mov < 0) {
    // If moving left, check if it can move left against mask
    if ((I->wall_detect & 0b00001100) && ((I->pos[1] + x_mov) < 0)) {
      I->pos[1] = 0;
      return;
    }
  }
  // Changing position
  I->pos[1] += x_mov;
  // If exiting a local tile, change global tile
  if (I->pos[1] > 15) {
    I->pos[1] -= 16;
    I->pos[0] += 1;
  }
  else if (I->pos[1] < 0) {
    I->pos[1] += 16;
    I->pos[0] -= 1;
  }
  // Generates a new wall mask
  gen_wall_mask_IC(I);
}

// ==============================================
// ========== ENEMY SPECIFIC FUNCTIONS ==========
// ==============================================

// randNum(lower, upper) returns a random number between lower and upper
int randNum(char lower, char upper) {
  return (rand() % (upper - lower + 1)) + lower;
}

// move_enemies() moves the enemies with some pathfinding
void move_enemies() {
  // Generates an exact position
  short mc_exact_x = main_char.pos[0] * 16 + main_char.pos[1];
  short mc_exact_y = main_char.pos[2] * 16 + main_char.pos[3];
  game_state.enemies_on_screen = 0;   // For refresh rate - don't refresh if no enemies
  for (int i = 0; i < NUM_ENEMIES; i++) {
    // Enemy exact position
    if (enemies[i].char_des_index_value != DEAD) {
      short ene_exact_x = enemies[i].pos[0] * 16 + enemies[i].pos[1];
      short ene_exact_y = enemies[i].pos[2] * 16 + enemies[i].pos[3];
      short x_dist = mc_exact_x - ene_exact_x;
      short y_dist = mc_exact_y - ene_exact_y;
      gen_wall_mask(&enemies[i]);
      // If enemies are alive
      game_state.enemies_on_screen = 1; // Refresh rate
      // Which direction should it go?
      if (randNum(0, abs(x_dist) + abs(y_dist)) > abs(x_dist)) {
        if (y_dist > 0) { // Moving enemy down towards MC
          move_char_ver(enemies[i].move_speed, &enemies[i]);
        }
        else {                                // Moving enemy up towards MC
          move_char_ver(-enemies[i].move_speed, &enemies[i]);
        }
      }
      else {
        if (x_dist > 0) { // Moving enemy right towards MC
          move_char_hor(enemies[i].move_speed, &enemies[i]);
        }
        else {                                // Moving enemy left towards MC
          move_char_hor(-enemies[i].move_speed, &enemies[i]);
        }
      }
    }
  }
}

// =======================================
// ========== DRAWING FUNCTIONS ==========
// =======================================

// draw_enemy(E) draws the proper enemy in the right direction on top
void draw_enemy(char_type* E) {
  const long long** des_index_pt = enemy_index[E->char_des_index_value]; // Get the pointer to the array for the model to draw
  const char** des_mask_pt = enemy_mask_index[E->char_des_index_value];  // Get the pointer to the mask for the model
  // Draw
  drawTop((E->pos[0] * 16 + E->pos[1]), (E->pos[2] * 16 + E->pos[3]), *(des_index_pt + E->des_state_index_value), *(des_mask_pt + E->des_state_index_value));
}

// draw_main_char(MC) draws the main character
void draw_main_char(char_type* MC) {
  const long long** des_index_pt = main_char_index[MC->char_des_index_value]; // Get the pointer to the array for the model to draw
  const char** des_mask_pt = main_char_mask_index[MC->char_des_index_value];  // Get the pointer to the mask for the model
  // Draw
  drawTop((MC->pos[0] * 16 + MC->pos[1]), (MC->pos[2] * 16 + MC->pos[3]), *(des_index_pt + MC->des_state_index_value), *(des_mask_pt + MC->des_state_index_value));
}

/* draw_enemies() draws all the enemies in enemies[] if they aren't dead, and if
they are, it will use time.death_cloud to see if a death cloud needs to be drawn.*/
void draw_enemies() {
  time.current_time = time_us_32();
  for (int i = 0; i < NUM_ENEMIES; i++) {
    // Drawing death cloud if the enemy just died, and check the latest enemy hit
    if ((time.current_time < time.death_cloud) && (enemy_hit >= 0)) {
      draw_enemy(&enemies[enemy_hit]);
    }
    // Drawing the enemy
    if (enemies[i].char_des_index_value != DEAD) {
      draw_enemy(&enemies[i]);
    }
  }
}

// draw_entities() draws all the entities in entities[] if they aren't nothing
void draw_entities() {
  for (int i = 0; i < NUM_ENTITIES; i++) {
    // Draw entities if there is something there
    if (entities[i].index_value != NOTHING) {
      drawTop(entities[i].pos[0] * 16 + entities[i].pos[1], entities[i].pos[2] * 16 + entities[i].pos[3], entity_map[entities[i].index_value], entity_mask[entities[i].index_value]);
    }
  }
  // // Draw Transitions for troubleshooting
  // for (int i = 0; i < SCREEN_TRAN; i++) {
  //   drawRect(62 + (sc_list[i].pos[0] * 16 + sc_list[i].pos[1]) * 2, 92 + (sc_list[i].pos[2] * 16 + sc_list[i].pos[3]) * 2, sc_list[i].size[0] * 2, sc_list[i].size[1] * 2, RED);
  // }
}

// draw_start_screen( ) draws the start screen
void draw_start_screen() {
  setCursor(110, 75); //120
  setTextColor(WHITE);
  setTextSize(3);
  writeString("The Saga of Steve");

  setTextSize(1);
  setCursor(425, 75);
  writeString("(demo)");
  setTextSize(2);
  setCursor(10, 260);
  writeString("Controls:");
  setCursor(35, 285);
  writeString("W");
  setCursor(10, 305);
  writeString("A S D");
  setCursor(95, 310);
  writeString("to move");
  setCursor(35, 345);
  writeString("L    to interact");
  setCursor(35, 365);
  writeString("K    to fight");
  setCursor(35, 385);
  writeString("C    to respawn");



  setCursor(160, 120);
  setTextSize(2);
  writeString("Press L to start!");

  setTextSize(1);
  setCursor(425, 360);
  writeString("Credits:");
  setCursor(425, 370);
  writeString("Programming-Tangia Sun");
  setCursor(425, 380);
  writeString("Assets-Emily Speckhals");
  setCursor(425, 390);
  writeString("Sound-Max Klugherz");
}

void draw_wake_up_text() {
  sleep_ms(500);
  setCursor(80, 80);
  setTextColor(WHITE);
  setTextSize(3);
  writeString("hey you...");
  sleep_ms(1500);
  setCursor(280, 200);
  writeString("wake up...");
  sleep_ms(1500);
}

// draw_interface() draws everything on top of the level
void draw_interface() {
  char num_gems[32];
  // Draw Hearts
  setCursor(397, -13);
  setTextColor(RED);
  setTextSize(2);
  writeString("- L I F E -");
  drawHearts(main_char.heart_stat[0], main_char.heart_stat[1], &FULL_HEART_DESIGN[0], &EMPTY_HEART_DESIGN[0]);
  // If the character has a weapon, it will draw
  if (main_char.weapon) {
    drawTileFine(52, -7, &DISP_SW_TILE[0][0]);
  }
  // Drawing the display gem and number of gems
  drawTileFine(120, -6, &GEM_TILE_DISP[0][0]);
  setCursor(150, -1);
  setTextColor(WHITE);
  setTextSize(3);
  writeString("X");
  drawTileMonoFine(168, -6, BLACK);
  sprintf(num_gems, "%d", game_state.gems);
  writeString(num_gems);
  // Drawing the blue square for the item
  drawItemSpace(&corner_row_des[0], &full_row_des[0]);
}

// write_text(index) will write the string in ALL_STR[index] at the top of the screen
void write_text(char index) {
  fillRect(60, 30, 520, 60, BLACK);
  setCursor(60, -2);
  setTextColor(WHITE);
  setTextSize(2);
  writeString((char*)ALL_STR[index]);
}

// draw_weapon(x, y, des, mask) draws the full tile des and mask at x, y
void draw_weapon(short sword_x, short sword_y, const long long* des, const char* mask) {
  drawTopBoundless(sword_x, sword_y, des, mask);
}

// draw_bg() draws the background of the game
void draw_bg() {
  // Get the background
  const long long* start_pt = LEVEL0[game_state.curr_screen[0] + game_state.curr_screen[1] * LEVEL_HEIGHT];
  long long value = *start_pt;    // Value is checking the index
  long long mono_check;           // Checking if the tile is mono_colored
  // Scan over every element in the array and draw accordingly
  for (int i = 1; i < 12; i++) {
    for (int k = 0; k < 2; k++) {
      for (int j = 8; j > 0; j--) {
        // Checking if the first index is 0xF
        mono_check = (value >> 8 * (8 - j)) & 0xFF;
        if (mono_check >= 240) {  // If 0xF, then draw the tile
          drawTile(j + (k * 8), i, block_map[mono_check - 240]);
        }                         // If 0xE0, draw white
        else if (mono_check == 224) {
          drawTileMono(j + (k * 8), i, WHITE);
        }
        else {                    // Draw the mono tile
          drawTileMono(j + (k * 8), i, (mono_check >> 4));
        }
      }
      // Cycle to the next value
      start_pt = start_pt + 1;
      value = *start_pt;
    }
  }
}

// ==========================================
// ========== GAME LOGIC FUNCTIONS ==========
// ==========================================

/* char mc_hitbox_check() returns 0 if the character wasn't hit,
returns 1 if the character got hit from the top
returns 2 if the character got hit from the bottom
returns 3 if the character got hit from the right
returns 4 if the character got hit from the left */
char mc_hitbox_check() {
  for (int i = 0; i < NUM_ENEMIES; i++) {
    if ((abs(main_char.pos[0] - enemies[i].pos[0]) < 3) && enemies[i].char_des_index_value != DEAD) {
      if (abs(main_char.pos[2] - enemies[i].pos[2]) < 3) {
        short mc_exact_x = main_char.pos[0] * 16 + main_char.pos[1];
        short mc_exact_y = main_char.pos[2] * 16 + main_char.pos[3];
        short ene_exact_x = enemies[i].pos[0] * 16 + enemies[i].pos[1];
        short ene_exact_y = enemies[i].pos[2] * 16 + enemies[i].pos[3];
        // 2 and 0 are offsets from the center
        if ((mc_exact_x < ene_exact_x + enemies[i].size[0]) && (mc_exact_x + 2 + main_char.size[0] > ene_exact_x) &&
          (mc_exact_y < ene_exact_y + enemies[i].size[1]) && (mc_exact_y + 0 + main_char.size[1] > ene_exact_y)) {
          signed char top_dist = mc_exact_y - ene_exact_y;
          signed char side_dist = mc_exact_x - ene_exact_x;
          if (abs(top_dist) > abs(side_dist)) { // hit vertically
            if (top_dist > 0) {
              return 1;
            }
            else {
              return 2;
            }
          }
          if (abs(top_dist) < abs(side_dist)) { // hit horizontally
            if (side_dist < 0) {
              return 3;
            }
            else {
              return 4;
            }
          }
        }
      }
    }
  }
  return 0;
}

/* signed char attack_hitbox_check(x, y, w, h) will check the rectangle specified
by position x and y, and width and height w and h (0,0) on the top left. It will return
the index of the enemy hit in enemies[]. Returns -1 if nothing was hit. */
signed char attack_hitbox_check(short x, short y, short w, short h) {
  for (int i = 0; i < NUM_ENEMIES; i++) {
    if (abs(main_char.pos[0] - enemies[i].pos[0]) < 3 && enemies[i].char_des_index_value != DEAD) {
      if (abs(main_char.pos[2] - enemies[i].pos[2]) < 3) {
        short ene_exact_x = enemies[i].pos[0] * 16 + enemies[i].pos[1];
        short ene_exact_y = enemies[i].pos[2] * 16 + enemies[i].pos[3];
        if ((x < ene_exact_x + enemies[i].size[0]) && (x + w > ene_exact_x) &&
          (y < ene_exact_y + enemies[i].size[1]) && (y + h > ene_exact_y)) {
          if (enemies[i].char_des_index_value == DEAD) {
            continue;
          }
          else {
            return i;
          }
        }
      }
    }
  }
  return -1;
}

/* signed char check_environment() will check all the entities around the character
and return the index of the interactable the character hits in entities[]. Return -1 if
nothing was touched. */
signed char check_environment() {
  for (int i = 0; i < NUM_ENTITIES; i++) {
    if ((abs(main_char.pos[0] - entities[i].pos[0]) < 3) && (entities[i].index_value != NOTHING)) {
      if (abs(main_char.pos[2] - entities[i].pos[2]) < 3) {
        short mc_exact_x = main_char.pos[0] * 16 + main_char.pos[1];
        short mc_exact_y = main_char.pos[2] * 16 + main_char.pos[3];
        short ent_exact_x = entities[i].pos[0] * 16 + entities[i].pos[1];
        short ent_exact_y = entities[i].pos[2] * 16 + entities[i].pos[3];
        // 2 and 0 are offsets from the center
        if ((mc_exact_x < ent_exact_x + entities[i].size[0]) && (mc_exact_x + 2 + main_char.size[0] > ent_exact_x) &&
          (mc_exact_y < ent_exact_y + entities[i].size[1]) && (mc_exact_y + 0 + main_char.size[1] > ent_exact_y)) {
          return i;
        }
      }
    }
  }
  return -1;
}

/* signed char check_front() will check if there is anything interactable in front
of the character, and return the index of the interactable the character interacts
with in entites[]. Retrns -1 if nothing was interacted with. */
signed char check_front() {
  for (int i = 0; i < NUM_ENTITIES; i++) {
    if (entities[i].index_value == BOULDER || (entities[i].index_value == BOULDER_RED) || (entities[i].index_value == SIGN) || (entities[i].index_value == PED_SW) || (entities[i].index_value == VILLAGER)) {
      char facing_dir = main_char.des_state_index_value;
      signed short x_dist = main_char.pos[0] - entities[i].pos[0]; // positive if MC is right of interactable, neg if left
      signed short y_dist = main_char.pos[2] - entities[i].pos[2]; // positive if MC is under interactable, neg if top
      if ((abs(x_dist) < 2) && ((y_dist == 0) || ((y_dist == -1) && (main_char.pos[3] != 0))) && ((facing_dir == RIGHT) || (facing_dir == RIGHT_2) || (facing_dir == LEFT) || (facing_dir == LEFT_2))) {
        if ((facing_dir == RIGHT || facing_dir == RIGHT_2) && (x_dist == -1)) {
          return i;
        }
        else if ((facing_dir == LEFT || facing_dir == LEFT_2) && (x_dist == 1) && main_char.pos[1] == 0) {
          return i;
        }
      }
      if ((abs(y_dist) < 2) && (x_dist == 0 || (x_dist == -1 && (main_char.pos[1] != 0))) && ((facing_dir == FRONT) || (facing_dir == FRONT_2) || (facing_dir == BACK) || (facing_dir == BACK_2))) {
        if ((facing_dir == FRONT || facing_dir == FRONT_2) && (y_dist == -1)) {
          return i;
        }
        else if ((facing_dir == BACK || facing_dir == BACK_2) && (y_dist == 1) && main_char.pos[3] == 0) {
          return i;
        }
      }
    }
  }
  return -1;
}

/* signed char check_level() will check if the character has touched a "bar" that
causes the main character to move to an adjacent screen. It will return the index
in sc_list[]. Returns -1 if it didn't touch a screen transition block. */
signed char check_level() {
  for (int i = 0; i < SCREEN_TRAN; i++) {
    if (abs(main_char.pos[0] - sc_list[i].pos[0]) < 2) {
      if (abs(main_char.pos[2] - sc_list[i].pos[2]) < 2) {
        short mc_exact_x = main_char.pos[0] * 16 + main_char.pos[1];
        short mc_exact_y = main_char.pos[2] * 16 + main_char.pos[3];
        short screen_exact_x = sc_list[i].pos[0] * 16 + sc_list[i].pos[1];
        short screen_exact_y = sc_list[i].pos[2] * 16 + sc_list[i].pos[3];
        if ((mc_exact_x < screen_exact_x + sc_list[i].size[0]) && (mc_exact_x + 2 + main_char.size[0] > screen_exact_x) &&
          (mc_exact_y < screen_exact_y + sc_list[i].size[1]) && (mc_exact_y + 0 + main_char.size[1] > screen_exact_y)) {
          return i;
        }
      }
    }
  }
  return -1;
}

/* die() dies the character */
void die() {
  static int i;
  fillRect(1, 1, 640, 480, BLACK);
  setCursor(120, 180);
  setTextColor(RED);
  setTextSize(2);
  writeString("You died.");
  setCursor(140, 200);
  for (i = 0; i < game_state.num_deaths; i++) {
    writeString("Again. ");
  }
  setCursor(300, 370);
  writeString("Press C to try again");
}

// ====================================
// ========== INITIALIZATION ==========
// ====================================

/* init_enemy(G, x_pos, y_pos, name) will initialize an enemy in G of type name at
x_pos, y_pos, and will have hard coded sizes and hearts*/
void init_enemy(char_type* G, char x_pos, char y_pos, char name) {
  if (name == GUARDIAN) {
    G->char_des_index_value = GUARDIAN;
    G->des_state_index_value = FRONT;
    G->pos[0] = x_pos;
    G->pos[1] = 0;
    G->pos[2] = y_pos;
    G->pos[3] = 0;
    G->heart_stat[0] = 1;
    G->heart_stat[1] = 1;
    G->move_speed = 2;
    G->size[0] = 15;
    G->size[1] = 16;
    G->weapon = 0;
    gen_wall_mask(G);
  }
  else if (name == DEAD) {
    G->char_des_index_value = DEAD;
    G->des_state_index_value = FRONT;
    G->pos[0] = 0;
    G->pos[1] = 0;
    G->pos[2] = 0;
    G->pos[3] = 0;
    G->heart_stat[0] = 0;
    G->heart_stat[1] = 0;
    G->move_speed = 0;
    G->size[0] = 0;
    G->size[1] = 0;
    G->weapon = 0;
    gen_wall_mask(G);
  }
  else if (name == SPIDER) {
    G->char_des_index_value = SPIDER;
    G->des_state_index_value = FRONT;
    G->pos[0] = x_pos;
    G->pos[1] = 0;
    G->pos[2] = y_pos;
    G->pos[3] = 0;
    G->heart_stat[0] = 3;
    G->heart_stat[1] = 3;
    G->move_speed = 4;
    G->size[0] = 16;
    G->size[1] = 13;
    G->weapon = 0;
    gen_wall_mask(G);
  }
}

/* init_entity(I, x_pos, y_pos, name) will initialize an entity in I of type name at
x_pos, y_pos, and will have hard coded sizes and positions */
void init_entity(interactable_type* I, char x_pos, char y_pos, char name) {
  I->wall_detect = 0;
  I->index_value = name;
  I->pos[0] = x_pos;
  I->pos[2] = y_pos;
  if (name == GEM) {
    I->pos[1] = 5;
    I->pos[3] = 3;
    I->size[0] = 7;
    I->size[1] = 10;
  }
  else if (name == HEART) {
    I->pos[1] = 2;
    I->pos[3] = 2;
    I->size[0] = 11;
    I->size[1] = 11;
  }
  else {
    I->pos[1] = 0;
    I->pos[3] = 0;
    I->size[0] = 16;
    I->size[1] = 16;
  }
}

/* init_sign(I, x_pos, y_pos, index) will initialize an entity in I of type sign, where
the string show by the sign is ALL_STR[index], at x_pos, y_pos */
void init_sign(interactable_type* I, char x_pos, char y_pos, char index) {
  I->wall_detect = index;
  I->index_value = SIGN;
  I->pos[0] = x_pos;
  I->pos[1] = 0;
  I->pos[2] = y_pos;
  I->pos[3] = 0;
  I->size[0] = 16;
  I->size[1] = 16;
}

/* init_villager(I, x_pos, y_pos, index) will initialize an entity in I of type villager, where
the string show by the sign is ALL_STR[index], at x_pos, y_pos. This is an SSE, and will change
to read ALL_STR[index + 1] when the SSE is done */
void init_villager(interactable_type* I, char x_pos, char y_pos, char index) {
  I->wall_detect = index;
  I->index_value = VILLAGER;
  I->pos[0] = x_pos;
  I->pos[1] = 0;
  I->pos[2] = y_pos;
  I->pos[3] = 0;
  I->size[0] = 16;
  I->size[1] = 16;
}

/* init_sc_tran(I, x_pos, y_pos, direction) will initialize an entity in I of type screen_transition,
where direction indicates which way the character is exiting the screen. direction is defined as:
0: FILLER - Does nothing
1: top of the screen
2: bottom of the screen
3: left of the screen
4: right of the screen */
void init_sc_tran(interactable_type* I, char x_pos, char y_pos, char direction) {
  I->wall_detect = direction;
  I->index_value = SC_TR;
  I->pos[0] = x_pos;
  I->pos[2] = y_pos;
  if (direction == 1) { // top of screen
    I->pos[1] = 0;
    I->pos[3] = 2;
    I->size[0] = 16;
    I->size[1] = 2;
  }
  else if (direction == 2) { // bottom of screen
    I->pos[1] = 0;
    I->pos[3] = 12;
    I->size[0] = 16;
    I->size[1] = 2;
  }
  else if (direction == 3) { // left of screen
    I->pos[1] = 2;
    I->pos[3] = 0;
    I->size[0] = 2;
    I->size[1] = 16;
  }
  else if (direction == 4) { // right of screen
    I->pos[1] = 12;
    I->pos[3] = 0;
    I->size[0] = 2;
    I->size[1] = 16;
  }
  else {
    I->pos[1] = 0;
    I->pos[3] = 0;
    I->size[0] = 0;
    I->size[0] = 0;
  }
}

/* init_screen() will check the curr_screen of the game and see which interactables/enemies should be initialized.
It considers SSE by hard coding for specific elements that should only happen once*/
void init_screen() {
  // initialize interactables
  char index = game_state.curr_screen[0] + (game_state.curr_screen[1] * LEVEL_HEIGHT);
  const short* interactable_pt = S0IAS[index];
  short value = *interactable_pt;
  char enemy_index = 0;
  char entity_index = 0;
  char screen_tran_index = 0;
  // initialize so it doesn't break
  for (int i = 0; i < NUM_ENEMIES; i++) {
    init_enemy(&enemies[i], 0, 0, DEAD);
  }
  for (int i = 0; i < NUM_ENTITIES; i++) {
    init_entity(&entities[i], 0, 0, NOTHING);
  }
  for (int i = 0; i < SCREEN_TRAN; i++) {
    init_sc_tran(&sc_list[i], 0, 0, 0);
  }

  // read through
  for (int i = 0; i < NUM_INTERACTABLES; i++) {
    char type = (value >> 12) & 0xF;
    char info = (value >> 8) & 0xF;
    char x_pos = (value >> 4) & 0xF;
    char y_pos = (value & 0xF);
    if (type == 0xF) {
      break;
    }
    else if (type == 1) {
      if (enemy_index < NUM_ENEMIES) {
        init_enemy(&enemies[enemy_index], x_pos, y_pos, info);
        enemy_index += 1;
      }
    }
    else if (type == 2 || type == 3 || type == 4) {
      if (entity_index < NUM_ENTITIES) {
        if (type == 2) {
          if ((info == HEART) && (SSE[index] == 0)) {
            init_entity(&entities[entity_index], x_pos, y_pos, info);
          }
          else if ((info == HEART) && (SSE[index] == 1)) {
          }
          else if ((info == PED_SW) && (SSE[index] == 0)) {
            init_entity(&entities[entity_index], x_pos, y_pos, PED_SW);
          }
          else if ((info == PED_SW) && (SSE[index] == 1)) {
            init_entity(&entities[entity_index], x_pos, y_pos, PED_NO_SW);
          }
          else {
            init_entity(&entities[entity_index], x_pos, y_pos, info);
          }

        }
        else if (type == 3) {
          init_sign(&entities[entity_index], x_pos, y_pos, info);
        }
        else if (type == 4) {
          if (SSE[index] == 0) {
            init_villager(&entities[entity_index], x_pos, y_pos, info);
          }
          else if (SSE[index] == 1) {
            init_villager(&entities[entity_index], x_pos, y_pos - 1, info + 1);
          }
        }
        entity_index += 1;
      }
    }
    else if (type == 0) {
      if (screen_tran_index < SCREEN_TRAN) {
        init_sc_tran(&sc_list[screen_tran_index], x_pos, y_pos, info);
      }
      screen_tran_index += 1;
    }
    else if (type == 0xA && game_state.MC_start) {
      main_char.pos[0] = x_pos;
      main_char.pos[1] = 0;
      main_char.pos[2] = y_pos;
      main_char.pos[3] = 0;
      main_char.des_state_index_value = info;
      game_state.MC_start = 0;
      game_state.respawn_pos[0] = main_char.pos[0];
      game_state.respawn_pos[1] = main_char.pos[1];
      game_state.respawn_pos[2] = main_char.pos[2];
      game_state.respawn_pos[3] = main_char.pos[3];
    }
    interactable_pt += 1;
    value = *interactable_pt;
  }
}

/* init_game() will fill in all the values for the game and call init_screen. */
void init_game() {
  // initialize game states
  game_state.curr_level = 0;
  game_state.curr_screen[0] = 1;
  game_state.curr_screen[1] = 1;
  game_state.gems = 0;
  game_state.interact = 0;
  game_state.enemies_on_screen = 1;
  game_state.pause = 1;
  game_state.MC_start = 1;
  game_state.num_deaths = 0;
  game_state.game_start = 0;
  // initialize time
  time.current_time = 0;
  time.enemy_move_rdy = 0;
  time.death_cloud = 0;
  time.frame_rate_time = 0;
  time.attack_again_time = 0;
  time.sword_out_time = 0;
  time.input_enable_time = 0;
  time.input_time = 0;
  init_screen();
}

/* init_main_char(MC, W) will initialize MC with weapon W, facing the front. */
void init_main_char(char_type* MC, weapon_type* W) {
  MC->char_des_index_value = FULL; // index into main_char_index/main_char_mask_index
  MC->heart_stat[0] = 1;
  MC->heart_stat[1] = 2;
  MC->move_speed = 4;
  MC->size[0] = 12;
  MC->size[1] = 16;
  MC->weapon = 0; // has no weapon
  W->attacking = 0;
  W->attack_reg = 0;
  W->dir_index = SWORD_D;
  W->pos[0] = 0;
  W->pos[1] = 0;
}

// ====================================================
// === graphics: responsible for drawing the screen ===
// ====================================================
static PT_THREAD(protothread_graphics(struct pt* pt)) {

  PT_BEGIN(pt);
  // the protothreads interval timer
  while (!game_state.game_start) {
    draw_start_screen();
    sleep_ms(600);
    fillRect(175, 180, 220, 20, BLACK);
    sleep_ms(600);

  }
  fillRect(1, 1, 640, 480, BLACK);
  draw_wake_up_text();
  fillRect(1, 1, 640, 480, BLACK);
  draw_interface();
  game_state.pause = 0;
  while (true) {
    time.current_time = time_us_32();
    draw_bg(&game_state);
    draw_enemies();
    draw_entities();
    draw_main_char(&main_char);
    if (weapon.attacking && (time.current_time < time.sword_out_time) && main_char.des_state_index_value >= 4) {
      draw_weapon(weapon.pos[0], weapon.pos[1], weapon_des[main_char.des_state_index_value - 8], weapon_mask[main_char.des_state_index_value - 8]);
      // drawRect((sw_x_pos) * 2 + 62, (sw_y_pos * 2) + 92, sw_w * 2, sw_h * 2, RED);
    }
    PT_SEM_SAFE_WAIT(pt, &graphic_go);
  }
  PT_END(pt);
} // graphics thread

// =====================================
// === periodics: timer-based events ===
// =====================================
static PT_THREAD(protothread_periodic(struct pt* pt)) {
  PT_BEGIN(pt);
  while (true) {
    time.current_time = time_us_32();
    if (!game_state.pause) {
      if ((time.current_time > time.enemy_move_rdy) && game_state.enemies_on_screen) {
        move_enemies();
        time.enemy_move_rdy = time.current_time + enemy_move_cd_us;
        PT_SEM_SAFE_SIGNAL(pt, &game_go);
      }
      if ((time.current_time > time.sword_out_time) && (weapon.attacking == 1)) {
        weapon.attacking = 0;
        if (main_char.des_state_index_value >= 4) {
          main_char.des_state_index_value -= 4; // reset to non-attacking position
        }
        PT_SEM_SAFE_SIGNAL(pt, &game_go);
      }
    }
    PT_YIELD_usec(periodic_timer);
  }
  PT_END(pt);
}

// ================================
// === game: handles game logic ===
// ================================
static PT_THREAD(protothread_game(struct pt* pt)) {
  PT_BEGIN(pt);
  static char game_over;
  static char hit_dir;
  static signed char entity_index;
  static signed char hit_placeholder;
  static signed char level_dir_index;
  while (1) {
    PT_SEM_SAFE_WAIT(pt, &game_go);
    time.current_time = time_us_32();
    static int i;
    // Checking for an attack
    if (weapon.attacking && weapon.attack_reg) {
      // Drawing the sword
      if (main_char.des_state_index_value == FRONT || main_char.des_state_index_value == FRONT_2) {
        main_char.des_state_index_value = F_ATTACK;
        weapon.pos[0] = main_char.pos[0] * 16 + main_char.pos[1] + 7;
        weapon.pos[1] = main_char.pos[2] * 16 + main_char.pos[3] + 9;
        sw_x_pos = weapon.pos[0] + 2;
        sw_y_pos = weapon.pos[1] + 4;
        sw_w = 3;
        sw_h = 10;
      }
      else if (main_char.des_state_index_value == BACK || main_char.des_state_index_value == BACK_2) {
        main_char.des_state_index_value = B_ATTACK;
        weapon.pos[0] = main_char.pos[0] * 16 + main_char.pos[1] + 3;
        weapon.pos[1] = main_char.pos[2] * 16 + main_char.pos[3] - 9;
        sw_x_pos = weapon.pos[0] + 2;
        sw_y_pos = weapon.pos[1];
        sw_w = 3;
        sw_h = 10;
      }
      else if (main_char.des_state_index_value == RIGHT || main_char.des_state_index_value == RIGHT_2) {
        main_char.des_state_index_value = R_ATTACK;
        weapon.pos[0] = main_char.pos[0] * 16 + main_char.pos[1] + 9;
        weapon.pos[1] = main_char.pos[2] * 16 + main_char.pos[3] + 7;
        sw_x_pos = weapon.pos[0] + 4;
        sw_y_pos = weapon.pos[1] + 2;
        sw_w = 10;
        sw_h = 3;
      }
      else if (main_char.des_state_index_value == LEFT || main_char.des_state_index_value == LEFT_2) {
        main_char.des_state_index_value = L_ATTACK;
        weapon.pos[0] = main_char.pos[0] * 16 + main_char.pos[1] - 9;
        weapon.pos[1] = main_char.pos[2] * 16 + main_char.pos[3] + 7;
        sw_x_pos = weapon.pos[0];
        sw_y_pos = weapon.pos[1] + 2;
        sw_w = 10;
        sw_h = 3;
      }
      // Seeing if the sword hits anything
      hit_placeholder = attack_hitbox_check(sw_x_pos, sw_y_pos, sw_w, sw_h);
      if (hit_placeholder >= 0) {
        weapon.attack_reg = 0;
        enemy_hit = hit_placeholder;
        enemies[enemy_hit].heart_stat[0] -= 1;
        if (enemies[enemy_hit].heart_stat[0] == 0) {
          initialize_sound_effect(kill_enemy_sound);
          time.death_cloud = time.current_time + death_cloud_cd_us;
          enemies[enemy_hit].des_state_index_value = FRONT;
          enemies[enemy_hit].char_des_index_value = DEAD;
        }
        else {
          initialize_sound_effect(hit_enemy_sound);
          if (main_char.des_state_index_value == F_ATTACK) { // hit on char's top
            for (i = 0; i < 10; i++) {
              force_move_char_ver(2, &enemies[enemy_hit]);
              sleep_us(5000);
              draw_bg(&game_state);
              draw_enemies();
              draw_main_char(&main_char);
              draw_weapon(weapon.pos[0], weapon.pos[1], weapon_des[main_char.des_state_index_value - 8], weapon_mask[main_char.des_state_index_value - 8]);
            }
          }
          else if (main_char.des_state_index_value == B_ATTACK) { // hit on char's bottom
            for (i = 0; i < 10; i++) {
              force_move_char_ver(-2, &enemies[enemy_hit]);
              sleep_us(5000);
              draw_bg(&game_state);
              draw_enemies();
              draw_main_char(&main_char);
              draw_weapon(weapon.pos[0], weapon.pos[1], weapon_des[main_char.des_state_index_value - 8], weapon_mask[main_char.des_state_index_value - 8]);
            }
          }
          else if (main_char.des_state_index_value == L_ATTACK) { // hit on char's right
            for (i = 0; i < 10; i++) {
              force_move_char_hor(-2, &enemies[enemy_hit]);
              sleep_us(5000);
              draw_bg(&game_state);
              draw_enemies();
              draw_main_char(&main_char);
              draw_weapon(weapon.pos[0], weapon.pos[1], weapon_des[main_char.des_state_index_value - 8], weapon_mask[main_char.des_state_index_value - 8]);
            }
          }
          else if (main_char.des_state_index_value == R_ATTACK) { // hit on char's left
            for (i = 0; i < 10; i++) {
              force_move_char_hor(2, &enemies[enemy_hit]);
              sleep_us(5000);
              draw_bg(&game_state);
              draw_enemies();
              draw_main_char(&main_char);
              draw_weapon(weapon.pos[0], weapon.pos[1], weapon_des[main_char.des_state_index_value - 8], weapon_mask[main_char.des_state_index_value - 8]);
            }
          }
        }
      }
    }

    // Checking for an interact
    if (game_state.interact) {
      // Seeing if it interacts with anything
      entity_index = check_front();
      if (entity_index >= 0) {
        static int i;
        if ((entities[entity_index].index_value == BOULDER) || (entities[entity_index].index_value == BOULDER_RED)) {
          initialize_sound_effect(boulder_sound);
          gen_wall_mask_IC(&entities[entity_index]);
          if (main_char.des_state_index_value == FRONT || main_char.des_state_index_value == FRONT_2) {
            for (i = 0; i < 8; i++) {
              force_move_IC_ver(2, &entities[entity_index]);
              sleep_us(10000);
              draw_bg(&game_state);
              draw_entities();
              draw_main_char(&main_char);
            }
          }
          else if (main_char.des_state_index_value == BACK || main_char.des_state_index_value == BACK_2) {
            for (i = 0; i < 8; i++) {
              force_move_IC_ver(-2, &entities[entity_index]);
              sleep_us(10000);
              draw_bg(&game_state);
              draw_entities();
              draw_main_char(&main_char);
            }
          }
          else if (main_char.des_state_index_value == LEFT || main_char.des_state_index_value == LEFT_2) {
            for (i = 0; i < 8; i++) {
              force_move_IC_hor(-2, &entities[entity_index]);
              sleep_us(10000);
              draw_bg(&game_state);
              draw_entities();
              draw_main_char(&main_char);
            }
          }
          else if (main_char.des_state_index_value == RIGHT || main_char.des_state_index_value == RIGHT_2) {
            for (i = 0; i < 8; i++) {
              force_move_IC_hor(2, &entities[entity_index]);
              sleep_us(10000);
              draw_bg(&game_state);
              draw_entities();
              draw_main_char(&main_char);
            }
          }
        }
        else if (entities[entity_index].index_value == SIGN || (entities[entity_index].index_value == VILLAGER)) {
          game_state.pause = 1;
          if (entities[entity_index].index_value == SIGN) {
            write_text(entities[entity_index].wall_detect);
            game_state.interact = 0;
            while (game_state.interact == 0) {
            }
          }
          else if ((entities[entity_index].index_value == VILLAGER)) {
            if ((game_state.gems >= 50) && !SSE[game_state.curr_screen[0] + (game_state.curr_screen[1] * LEVEL_HEIGHT)]) {
              game_state.gems -= 50;
              write_text(entities[entity_index].wall_detect + 1);
              game_state.interact = 0;
              SSE[game_state.curr_screen[0] + (game_state.curr_screen[1] * LEVEL_HEIGHT)] = 1;
              while (game_state.interact == 0) {
              }
              for (i = 0; i < 8; i++) {
                force_move_IC_ver(-2, &entities[entity_index]);
                sleep_us(5000);
                draw_bg(&game_state);
                draw_entities();
                draw_main_char(&main_char);
              }
            }
            else {
              game_state.interact = 0;
              write_text(entities[entity_index].wall_detect);
              game_state.interact = 0;
              while (game_state.interact == 0) {
              }
            }
          }
          game_state.pause = 0;
          fillRect(60, 30, 520, 60, BLACK);
        }
        else if (entities[entity_index].index_value == PED_SW) {
          initialize_sound_effect(important_item_sound);
          entities[entity_index].index_value = PED_NO_SW;
          SSE[game_state.curr_screen[0] + game_state.curr_screen[1] * LEVEL_HEIGHT] = 1;
          main_char.weapon = 1;
          draw_entities();
        }
        draw_interface();
      }
      gen_wall_mask(&main_char);
      game_state.interact = 0;
    }

    // Checking if it passively touches anything
    entity_index = check_environment();
    if (entity_index >= 0) {
      if (entities[entity_index].index_value == GEM) {
        initialize_sound_effect(get_rupee_sound);
        game_state.gems += 1;
      }
      else if (entities[entity_index].index_value == GEM_RED) {
        initialize_sound_effect(get_rupee_sound);
        game_state.gems += 10;
      }
      else if (entities[entity_index].index_value == HEART) {
        initialize_sound_effect(important_item_sound);
        main_char.heart_stat[1] += 1;
        main_char.heart_stat[0] = main_char.heart_stat[1];
        SSE[game_state.curr_screen[0] + (game_state.curr_screen[1] * LEVEL_HEIGHT)] = 1;
      }
      entities[entity_index].index_value = NOTHING;
      draw_interface();
    }

    // Checking if a level transition is touched
    level_dir_index = check_level();
    if (level_dir_index >= 0) {
      game_state.pause = 1;
      if (sc_list[level_dir_index].wall_detect == 1) {
        // if (game_state.curr_screen[0] == 2 && game_state.curr_screen[1] == 2) stop_music(); //Stop background music if entering cave
        game_state.curr_screen[1] -= 1;
        main_char.pos[2] += 10;
        main_char.pos[3] -= 4;
      }
      else if (sc_list[level_dir_index].wall_detect == 2) {
        // if (game_state.curr_screen[0] == 2 && game_state.curr_screen[1] == 1) restart_music(); //Restart background music if exiting cave
        game_state.curr_screen[1] += 1;
        main_char.pos[2] -= 10;
        main_char.pos[3] += 4;
      }
      else if (sc_list[level_dir_index].wall_detect == 3) {
        game_state.curr_screen[0] -= 1;
        main_char.pos[0] += 15;
        main_char.pos[1] -= 4;
      }
      else if (sc_list[level_dir_index].wall_detect == 4) {
        game_state.curr_screen[0] += 1;
        main_char.pos[0] -= 15;
        main_char.pos[1] += 4;
      }
      if ((game_state.curr_screen[0] == 1 && game_state.curr_screen[1] == 1)
        || (game_state.curr_screen[0] == 2 && game_state.curr_screen[1] == 1)
        || (game_state.curr_screen[0] == 2 && game_state.curr_screen[1] == 0)) {
        stop_music();
      }
      else {
        restart_music();
      }
      game_state.respawn_pos[0] = main_char.pos[0];
      game_state.respawn_pos[1] = main_char.pos[1];
      game_state.respawn_pos[2] = main_char.pos[2];
      game_state.respawn_pos[3] = main_char.pos[3];
      fillRect(1, 1, 640, 480, BLACK);
      draw_interface();
      init_screen();
      PT_SEM_SIGNAL(pt, &graphic_go);
      game_state.enemies_on_screen = 1;
      gen_wall_mask(&main_char);
      game_state.pause = 0;
    }

    // will return 1 if hit from top, 2 if hit from bottom, 3 if hit from left, 4 if hit from right
    // Checking if the player gets hit
    hit_dir = mc_hitbox_check();
    if (hit_dir) {
      main_char.heart_stat[0] -= 1;
      if (main_char.heart_stat[0] != 0) {
        initialize_sound_effect(get_hit_sound);
        draw_interface();
      }
      if (main_char.heart_stat[0] == 0) {
        stop_music();
        initialize_sound_effect(link_die_sound);
        game_state.pause = 1;
        draw_bg(&game_state);
        draw_interface();
        // draw_enemies();
        drawTop(main_char.pos[0] * 16 + main_char.pos[1], main_char.pos[2] * 16 + main_char.pos[3], &CLOUD_DES[0][0], &CLOUD_MASK[0][0]);
        sleep_ms(500);
        die();
        game_over = 'a';
        while (game_over != 'c') {
          sscanf(pt_serial_in_buffer, "%c", &game_over);
        }
        restart_music();
        fillRect(1, 1, 640, 480, BLACK);
        main_char.pos[0] = game_state.respawn_pos[0];
        main_char.pos[1] = game_state.respawn_pos[1];
        main_char.pos[2] = game_state.respawn_pos[2];
        main_char.pos[3] = game_state.respawn_pos[3];
        main_char.heart_stat[0] = main_char.heart_stat[1];
        game_state.num_deaths += 1;
        init_screen();
        draw_interface();
        game_over = 'a';
        game_state.pause = 0;
      }
      else if (hit_dir == 1) { // hit on char's top
        for (i = 0; i < 20; i++) {
          force_move_char_ver(1, &main_char);
          sleep_us(5000);
          draw_bg(&game_state);
          draw_enemies();
          draw_main_char(&main_char);
        }
      }
      else if (hit_dir == 2) { // hit on char's bottom
        for (i = 0; i < 20; i++) {
          force_move_char_ver(-1, &main_char);
          sleep_us(5000);
          draw_bg(&game_state);
          draw_enemies();
          draw_main_char(&main_char);
        }
      }
      else if (hit_dir == 3) { // hit on char's right
        for (i = 0; i < 20; i++) {
          force_move_char_hor(-1, &main_char);
          sleep_us(5000);
          draw_bg(&game_state);
          draw_enemies();
          draw_main_char(&main_char);
        }
      }
      else if (hit_dir == 4) { // hit on char's left
        for (i = 0; i < 20; i++) {
          force_move_char_hor(1, &main_char);
          sleep_us(5000);
          draw_bg(&game_state);
          draw_enemies();
          draw_main_char(&main_char);
        }
      }
    }

    // sprintf(pt_serial_out_buffer, "%d", entity);
    // serial_write;
    PT_SEM_SAFE_SIGNAL(pt, &graphic_go);
  }
  PT_END(pt);
}

// ======================================================
// === serial: reading serial monitor for user inputs ===
// ======================================================
static PT_THREAD(protothread_serial(struct pt* pt)) {
  static char user_input;
  PT_BEGIN(pt);
  while (1) {
    time.current_time = time_us_32();
    if ((time.current_time > time.input_enable_time)) {
      serial_read;
      sscanf(pt_serial_in_buffer, "%c", &user_input);
      // sprintf(pt_serial_out_buffer, "%d", game_state.num_deaths);
      // serial_write;
      if (!game_state.game_start) {
        if (user_input == 'l') {
          game_state.game_start = 1;
        }
      }
      else if (game_state.pause) {
        if (user_input == 'l') {
          game_state.interact = 1;
          game_state.pause = 0;
        }
      }
      else if ((main_char.heart_stat[0] != 0)) {
        gen_wall_mask(&main_char);
        if (user_input == 'w') {
          if (main_char.des_state_index_value == BACK || main_char.des_state_index_value == BACK_2) {
            if (time.current_time > time.input_time) {
              time.input_time = time.current_time + input_cd;
              move_char_ver(-main_char.move_speed, &main_char);
            }
          }
          else {
            move_char_ver(-main_char.move_speed, &main_char);
          }
        }
        else if (user_input == 'a') {
          if (main_char.des_state_index_value == LEFT || main_char.des_state_index_value == LEFT_2) {
            if (time.current_time > time.input_time) {
              time.input_time = time.current_time + input_cd;
              move_char_hor(-main_char.move_speed, &main_char);
            }
          }
          else {
            move_char_hor(-main_char.move_speed, &main_char);
          }
        }
        else if (user_input == 's') {
          if (main_char.des_state_index_value == FRONT || main_char.des_state_index_value == FRONT_2) {
            if (time.current_time > time.input_time) {
              time.input_time = time.current_time + input_cd;
              move_char_ver(main_char.move_speed, &main_char);
            }
          }
          else {
            move_char_ver(main_char.move_speed, &main_char);
          }
        }
        else if (user_input == 'd') {
          if (main_char.des_state_index_value == RIGHT || main_char.des_state_index_value == RIGHT_2) {
            if (time.current_time > time.input_time) {
              time.input_time = time.current_time + input_cd;
              move_char_hor(main_char.move_speed, &main_char);
            }
          }
          else {
            move_char_hor(main_char.move_speed, &main_char);
          }
        }
        else if (user_input == 'k') {
          if ((time.current_time > time.attack_again_time) && weapon.attacking == 0) {
            if (main_char.weapon) {
              weapon.attacking = 1;
              weapon.attack_reg = 1;
              time.attack_again_time = time.current_time + attack_again_cd;
              time.sword_out_time = time.current_time + sword_out_cd;
              initialize_sound_effect(sword_slash_sound);
              time.input_enable_time = time.current_time + input_enable_cd;
            }
          }
        }
        else if (user_input == 'l') {
          game_state.interact = 1;
        }
        else if (user_input == 't') {
        }
      }
      PT_SEM_SAFE_SIGNAL(pt, &game_go);
    }
  } // END WHILE(1)
  PT_END(pt);
} // serial thread

// ========================================
// === core 1 main -- started in main below
// ========================================
void core1_main() {
  //
  //  === add threads  ====================
  // for core 1
  pt_add_thread(protothread_serial);
  //
  // === initalize the scheduler ==========
  pt_schedule_start;
  // NEVER exits
  // ======================================
}

// ========================================
// === core 0 main
// ========================================
int main() {
  // set the clock
  // set_sys_clock_khz(250000, true); // 171us
  // start the serial i/o
  stdio_init_all();
  // announce the threader version on system reset

  // Initialize the VGA screen
  initVGA();

  // start core 1 threads
  multicore_reset_core1();
  multicore_launch_core1(&core1_main);

  PT_SEM_SAFE_INIT(&graphic_go, 0);
  PT_SEM_SAFE_INIT(&game_go, 0);
  init_main_char(&main_char, &weapon);
  init_game();
  gen_wall_mask(&main_char);

  // Initialize SPI channel (channel, baud rate set to 20MHz)
  spi_init(SPI_PORT, 20000000);

  // Format SPI channel (channel, data bits per transfer, polarity, phase, order)
  spi_set_format(SPI_PORT, 16, 0, 0, 0);

  // Map SPI signals to GPIO ports, acts like framed SPI with this CS mapping
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

  buildLookupTable();

  // Setup the data channel
  c2 = dma_channel_get_default_config(data_chan);          // Default configs
  channel_config_set_transfer_data_size(&c2, DMA_SIZE_16); // 16-bit txfers
  channel_config_set_read_increment(&c2, true);            // yes read incrementing
  channel_config_set_write_increment(&c2, false);          // no write incrementing
  // (X/Y)*sys_clk, where X is the first 16 bytes and Y is the second
  // sys_clk is 125 MHz unless changed in code. Configured to ~7.7 kHz
  dma_timer_set_fraction(0, 0x0004, 0xffff);
  // 0x3b means timer0 (see SDK manual)
  channel_config_set_dreq(&c2, 0x3b);

  // Negative delay so means we will call repeating_timer_callback, and call it again
  // 25us (40kHz) later regardless of how long the callback took to execute
  add_repeating_timer_us(-25, repeating_timer_callback, NULL, &timer);
  stop_music();

  // === config threads ========================
  // for core 0
  pt_add_thread(protothread_graphics);
  pt_add_thread(protothread_periodic);
  pt_add_thread(protothread_game);

  // === initalize the scheduler ===============
  pt_schedule_start;
  // NEVER exits
  // ===========================================
} // end main
