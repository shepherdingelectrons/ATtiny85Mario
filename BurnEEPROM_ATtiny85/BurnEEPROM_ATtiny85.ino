#include <avr\eeprom.h>
#include "music_notes.h"
#include "mario_theme.h"

struct melody{
  uint16_t* melody; //+ 0
  uint8_t* noteDurations; // +2
  uint8_t numNotes; // +3
  uint8_t progmem; // + 4
};

struct music_table{
uint16_t startpos;
uint8_t len;
};

#define NUM_ENTRIES 10

struct music_table music_entries[NUM_ENTRIES];

const struct melody theme_music[]={{Melody_Intro, NoteDurations_Intro, sizeof(Melody_Intro)>>1,1},    // 0
                            {Melody_Refrain, NoteDurations_Refrain, sizeof(Melody_Refrain)>>1,1}, // 1
                            {Melody_Part2_1, NoteDurations_Part2_1, sizeof(Melody_Part2_1)>>1,0}, // 2
                            {Melody_Part2_2, NoteDurations_Part2_2, sizeof(Melody_Part2_2)>>1,0}, // 3
                            {Melody_Part2_3, NoteDurations_Part2_3, sizeof(Melody_Part2_3)>>1,0}, // 4
                            {Melody_Part3_1, NoteDurations_Part3_1, sizeof(Melody_Part3_1)>>1,1}, // 5
                            {CoinSound, CoinSound_Durations,sizeof(CoinSound)>>1,0}, // 6 - Coin sound effect
                            {JumpSound, JumpSound_Durations,sizeof(JumpSound)>>1,1},// 7 - Jump sound effect
                            {HitHeadSound, HitHead_Durations,sizeof(HitHeadSound)>>1,1},// 8 - Hit head sound effect
                            {PauseSound, PauseSound_Durations,sizeof(PauseSound)>>1,0}// 9 - Pause sound effect
                            };

uint8_t *pos;
uint8_t buffer_P[100];

// Play back variables:
struct toneController{
  uint8_t music_index=0, music_pos=0, music_note=0,noteordelay=0;    
  uint16_t current_delay=0;
  uint8_t soundeffect; // 0 = Music, 1=Sound effect (affects play priority and loop behaviour)
};

struct toneController MusicController;

const uint8_t music_seq[] ={0,1,1,2,3,2,4,2,3,2,4,5}; // 12 elements: 0-11. Go back to position 1 on overflow.

#define SPEAKER 4//3 //4 // Can use hardware Timer1 OC1B

extern volatile uint8_t *tone_timer_pin_register;
extern volatile long tone_timer_toggle_count;
uint8_t dummy_register;

///////////////////////////////////////////// end playback variables

  
void setup() {
  // put your setup code here, to run once:
  burnEEPROM();
  DDRB|=(1<<SPEAKER); // OUTPUT
  
  MusicController.soundeffect=0;
  initMusic(&MusicController);
}

void initMusic(struct toneController *controller)
{
     controller->music_pos=0; // First position in music sequence, i.e. 0,1,2,3 etc )
     controller->music_index = music_seq[controller->music_pos];//pgm_read_byte(&music_seq[0]+music_pos); // melody 0 (=intro
     controller->music_note = 0; // First note of music_index melody     
     controller->noteordelay=1; //simulate end of a delay to start the note
}

uint8_t* burnmusic(uint8_t entry)
{
  uint16_t *melody;
  uint8_t *noteDurations;
  uint8_t progmem, len;

  uint8_t duration,power;
  
  // Extract the good stuff
  melody = theme_music[entry].melody;
  noteDurations = theme_music[entry].noteDurations;
  progmem = theme_music[entry].progmem;
  len = theme_music[entry].numNotes;
  
  // Sort the table entry out
  music_entries[entry].len = len;
  music_entries[entry].startpos = pos;
  eeprom_write_block(&music_entries[entry],entry*sizeof(music_table),sizeof(music_table));
  
  // Write notes in next pos
  if (progmem)
  {
    memcpy_P(&buffer_P[0],melody,len*2);
  }
  else
  {
    memcpy(&buffer_P[0], melody,len*2);
  }

  // melody in buffer_P
  
  for (uint8_t i=0;i<len;i++)
    { 
      if (progmem) duration = pgm_read_byte(noteDurations+i);
      else duration = noteDurations[i];
      
      power = 1;
      if (duration==2) power = 1;
      else if (duration==4) power=2;
      else if (duration==8) power=3;
      else if (duration==16) power=4;
      else if (duration==32) power=5;
      else if (duration==64) power=6;
      else if (duration==128) power=7;

      buffer_P[1+(i*2)] |= (power<<5); // bytes are swapped in uint16_t
    }
  eeprom_write_block(buffer_P, pos, len*2); // 2 bytes per note  
  pos+=(len*2);

  /*
  // Write durations to EEPROM
  if (progmem)
  {
    memcpy_P(&buffer_P[0],noteDurations,len);
    eeprom_write_block(buffer_P, pos, len);
  }
  else
  {
    eeprom_write_block(noteDurations, pos, len); // 2 bytes per note
  }
  pos+=(len);*/
}

void burnEEPROM(void)
{
   pos=(NUM_ENTRIES*sizeof(music_table)); // Leave room for entry table at start

   for (uint8_t i=0;i<NUM_ENTRIES;i++)
      burnmusic(i); 

  // for (uint16_t i=0; i<512;i++)
//    eeprom_write_byte(i,0xFF);
   
  // burnmusic(0);
}


void loop() {
  // put your main code here, to run repeatedly:
  handleMusic(&MusicController);
}


void handleMusic(struct toneController *controller)
{  
  uint16_t *melody;
  uint8_t *noteDurations;
  uint8_t numNotes;
  uint8_t *entry_pos;

  if ((controller->soundeffect==0 && tone_timer_toggle_count==0))
  {    
    if (controller->noteordelay==0) // 0 = NOTE just ended, therefore now delay
    {
      // Note ended, start a delay
      controller->current_delay = (controller->current_delay * 0.30f); 
      
      if (controller->soundeffect==0) // Music
      {  
      tone(SPEAKER, 100, controller->current_delay); // do a low tone instead of using millis and switch off the pin!

      tone_timer_pin_register = &dummy_register; // don't do software toggle of the pin :)
      PORTB&=~(1<<SPEAKER);
      }
      
      // Advance to next note position
      controller->music_note++;           
      entry_pos = controller->music_index*sizeof(music_table);
      entry_pos+=2;
      numNotes = eeprom_read_byte(entry_pos); // next position is numNotes
      
      if (controller->music_note>=numNotes) //theme_music[controller->music_index].numNotes) // exceeded notes in this melody part
      {
        controller->music_note=0;
        controller->music_pos++; // advance sequence position

        if (controller->soundeffect==0 && controller->music_pos>=12) // In music handler, and sequence has overflowed.
        {
          controller->music_pos=1; // We are in the music handler
        }  
        controller->music_index = music_seq[controller->music_pos];//pgm_read_byte(&music_seq[0]+music_pos);
      
      }
      controller->noteordelay=1; //delay is next
    } // end note ended
   else if (controller->noteordelay==1) // 1 = DELAY
    {
     // Delay ended, play next note
     
     entry_pos = controller->music_index*sizeof(music_table);
     melody = eeprom_read_word((uint16_t*)entry_pos); // EEPROM position in entry table
     entry_pos+=2;
     numNotes = eeprom_read_byte(entry_pos); // next position is numNotes
     //noteDurations = (uint8_t*)(melody +(numNotes));//melody + (uint16_t)numNotes; // noteDurations follows the melody in EEPROM
    
     uint16_t ms=1000,freq;

     freq = eeprom_read_word(melody+controller->music_note);

     uint8_t note;
     note = freq >> 13;// Extract 2,4,8,16th,32nd note etc from freq
        // 2 = 1, 4 = 2, 8 = 3, etc.
     freq &= 0x1FFF;
     
     controller->current_delay = ms>>note; // shift rather than divide.
     
     //entry_pos = noteDurations+controller->music_note;
     //controller->current_delay = (ms / (uint16_t) eeprom_read_byte(noteDurations+controller->music_note));

    if (controller->soundeffect==0) // Music
    {
      tone(SPEAKER, freq, controller->current_delay);
    }
    
    controller->noteordelay=0;
    }// end delay ended
  } // wnd time delay lapsed
}
