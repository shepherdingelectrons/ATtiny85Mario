#define F_CPU 16000000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "mario.h"
#include "mario_tiles.h"
#include "bricks.h"
#include "font_digits.h"
#include "oled_lib.h"


//#define TEST_BUTTONS 1

struct music_table {
  uint16_t startpos;
  uint8_t len;
};

enum mario_state {idle, left, right, squash, dead};
enum mario_jumpstate {nojump, jumpup, jumpdown};
enum mario_direction {faceleft, faceright};
enum game_state {normal, paused, paused_ready, pause_return, mario_dying};

struct character {
  int16_t x, y;
  int16_t oldx[2], oldy[2];
  enum mario_state state;
  enum mario_jumpstate jumpstate;
  enum mario_direction dir;
  int8_t vx, vy;
  uint8_t frame;
  uint8_t collision;
  uint8_t coincollector;
  uint8_t mask;
  uint8_t w;
};

#define MAX_GOOMBAS 3

struct character mario, goomba[MAX_GOOMBAS], fireball;

struct toneController {
  uint8_t music_index, music_pos, music_note, noteordelay;
  uint16_t current_delay;
  uint8_t soundeffect; // 0 = Music, 1=Sound effect (affects play priority and loop behaviour)
};

struct toneController MusicController;
struct toneController SoundEffectController;

#define COIN_SOUND 6
#define JUMP_SOUND 7
#define HITHEAD_SOUND 8
#define PAUSE_SOUND 9

#define FIREBALL_SPEED 3

const uint8_t music_seq[] = {0, 1, 1, 2, 3, 2, 4, 2, 3, 2, 4, 5}; // 12 elements: 0-11. Go back to position 1 on overflow.

// soundeffect = 0: 1 passed onto myTone: Timer1 is used for Music - SPEAKER 
// soundeffect = 1: 0 passed onto myTone: Timer0 is used for sound effects - SPEAKER2

//#define SINGLE_SPEAKER // means we want to mix MUSIC and SOUND_EFFECTS in hardware (using SOUND_EFFECTS pin PB1), else have a pin for each

#define MUSIC_SPEAKER 4 // IC pin 3: PB4, Timer 1, OC1B - MUSIC (was speaker)
#define SOUNDEFFECT_SPEAKER 1// IC pin 6: PB1, Timer 0, OCOB - Sound effects (was speaker 2)

#define WORLD_MAX_LEN 63

#define ADC0 0
#define ADC1 1
#define ADC2 2
#define ADC3 3

#define TEMPO_DEFAULT 800
#define TEMPO_FAST 500

uint8_t screen[WORLD_MAX_LEN + 1], soundeffectplaying = 0;

uint16_t viewx = 0, old_viewx = 0, rviewx_trigger, MusicSpeed = TEMPO_DEFAULT, mario_acc_timer = 0;
int delta_viewx = 0;

volatile long mymicros=0,ISR_micro_period=0; 
volatile long tone_timer1_toggle_count=0, tone_timer0_toggle_count = 0;

uint8_t coinframe = 1, cointimer = 0, coincount = 0;
uint32_t curr_seed = 0;
enum game_state gamestate = normal;

void drawSprite(int16_t, int16_t, uint8_t, uint8_t, const unsigned char *, uint8_t);
void getWorld(uint16_t);
void drawScreen(void);
void playSoundEffect(uint8_t);
void initMusic(struct toneController *);
void handleMusic(struct toneController *);
void mytone(unsigned long, unsigned long, uint8_t);
void handlemap_collisions(struct character *);
void collidecoins(uint8_t, uint8_t, uint8_t);
uint8_t collideThings(int, int, int, int, int, int, uint8_t, uint8_t);
uint8_t collideMario(int, int, uint8_t, uint8_t);
uint8_t findcoiny(uint8_t);
void animate_character(struct character *);
void draw_mario(void);
void blank_character(uint8_t, struct character *);
void vblankout(int, int, uint8_t);
void readbuttons(void);
void drawCoin(int, uint8_t);
void updateDisplay(uint8_t);
void oledWriteInteger(uint8_t, uint8_t, uint8_t);
void setup(void);
uint16_t readADC(uint8_t);
uint8_t next_random(uint8_t);
void killGoomba(uint8_t g_id);
void spawnGoomba(uint16_t);

void setup() {
	//_delayms(50);
	oledInit(0x3c, 0, 0);
	oledFill(0);
	oledInit(0x3d, 0, 0);
	oledFill(0);

	//oledFill(0b00001111);

	mario.mask = 3; //fireball.mask = 3;
	mario.w = 2;//fireball.w = 2;
	
	for (uint8_t i=0;i<MAX_GOOMBAS;i++)
	{
		goomba[i].mask = 3;
		goomba[i].w = 2;
		goomba[i].state=dead;
	}
	
	mario.x = 5;
	mario.y = 0;
	mario.state = idle;
	mario.jumpstate = jumpdown;
	mario.vx = 0;
	mario.vy = 0;
	mario.frame = 0;
	mario.dir = faceright;
	mario.coincollector = 1;

	fireball.mask=1;
	fireball.w=1;
	fireball.state=dead;
	
	//analogReference(0); // 0 = Vcc as Aref (2 = 1.1v internal voltage reference)

	//pinMode(SPEAKER, OUTPUT);
	//pinMode(SPEAKER2, OUTPUT);

#ifdef SINGLE_SPEAKER
	DDRB |= (1 << SOUNDEFFECT_SPEAKER);
#else
	DDRB |= (1 << MUSIC_SPEAKER); // OUTPUT
	DDRB |= (1 << SOUNDEFFECT_SPEAKER); // OUTPUT
#endif
	// Setup screen
	getWorld(0); //WORLD_MAX_LEN + 1); //,16);
	getWorld(16);
	getWorld(32);
	getWorld(48);

	rviewx_trigger = 24;//12;// if viewx > trigger*8 getWorld element in certain position.
	//drawScreen();

	MusicController.soundeffect = 0;
	SoundEffectController.soundeffect = 1;
	soundeffectplaying = 0;
	initMusic(&MusicController);

	// Change music speaker to a hardware pin that Timer1 supports - i.e. pin4 = PB4
	TCCR0A = 0;//notone0A();
	TIMSK = 0;

	TCCR1 = 0 ;
	GTCCR = 0;
	TIFR = 0;
	
	//goomba.x = 100; goomba.y = 0;
	//goomba.vx = 1; goomba.state=dead;

}


int main(void)
{
	static long loop_micros=0;
	//int thingy=-7,thingvy=1;
	setup();
	
	sei();
	/* Replace with your application code */
	while (1)
	{
		
		//curr_seed++;// = (curr_seed*1664525 + 1013904223) % 65536; // next seed
		
		//vblankout(60,thingy,16); // blankout PREVIOUS position
		//vblankout(60,thingy+8,16);
		
		/*thingy+=thingvy;
		if (thingy<-30) thingvy=1;
		else if (thingy>80) thingvy=-1;
		
		drawSprite(60,thingy,16,16,&mario_idle[0],0);*/
		readbuttons();
		
		if (gamestate == normal)
		{
			handlemap_collisions(&mario); // Check for collisions with Mario and the world
			
			if (fireball.state!=dead)
			{
				handlemap_collisions(&fireball);
				if (fireball.y<0 || fireball.y>=63) {fireball.frame=90;fireball.y=0;fireball.vy=0;} 
				
				// 1 = UP, 2=RIGHT, 4=DOWN, 8=LEFT
				if (fireball.collision &1) fireball.vy=FIREBALL_SPEED;
				if (fireball.collision &4) fireball.vy=-FIREBALL_SPEED;
				if (fireball.collision &2) fireball.vx=-FIREBALL_SPEED;
				if (fireball.collision &8) fireball.vx=FIREBALL_SPEED;

				if (fireball.collision!=0) fireball.frame+=30;

				fireball.frame++;
				if (fireball.frame>100) fireball.state=dead;
			}

			animate_character(&mario); // just applies gravity, could be done elsewhere (i.e. handlemap_collisions physics?)
			
			for (uint8_t g=0;g<MAX_GOOMBAS;g++)
			{
				if (goomba[g].state!=dead) // Not dead. Alive you might say.
				{
					handlemap_collisions(&goomba[g]);
					animate_character(&goomba[g]);
				
					if (goomba[g].collision & 2) goomba[g].vx = -1;
					else if (goomba[g].collision & 8) goomba[g].vx = 1;

					if (goomba[g].x <= 0) {
						goomba[g].x = 0;
						goomba[g].vx = 1;
					}						// rviewx_trigger+40 is the *next* screen to write (i.e. doesn't exist yet), so turn around goomba if near edge
					if ((goomba[g].x>>3) >= rviewx_trigger+38) goomba[g].vx=-1; // if goomba attempst to walk off the edge of the known universe
					goomba[g].frame++;
					if (goomba[g].state==idle && goomba[g].frame >= 10) goomba[g].frame = 0;
					if (goomba[g].state==squash && goomba[g].frame>200) goomba[g].state=dead;
				} // end enemy not dead
				
				if (goomba[g].state==idle)
				{
					if (collideMario(goomba[g].x, goomba[g].y, 16, 16))
					{
						if (mario.vy > 0 && mario.jumpstate==jumpdown) {killGoomba(g);mario.vy = -7;mario.jumpstate = jumpup;playSoundEffect(JUMP_SOUND);}
						else playSoundEffect(HITHEAD_SOUND);
					}
					if (fireball.state==idle && collideThings(goomba[g].x, goomba[g].y, 16,16,fireball.x, fireball.y,8,8)) killGoomba(g);
					if (goomba[g].x<((int16_t)viewx-40)) killGoomba(g);
				}
			} // end goomba loop
			
			
			
			if (mario.frame++ >= 6) mario.frame = 0;

			cointimer++;  // Animate coin
			if (cointimer >= 5)
			{
				cointimer = 0;
				coinframe++;
				if (coinframe >= 5) coinframe = 1;
			}

			if (coincount >= 10 && coincount < 20) MusicSpeed = TEMPO_FAST; else MusicSpeed = TEMPO_DEFAULT;

			if (mario.x <= 192) viewx = 0;
			else if (viewx < mario.x - 192) viewx = mario.x - 192; // Do we need to change the viewport coordinate?

			uint16_t blockx = viewx >> 3; // viewx / 8;
			if (blockx >= rviewx_trigger) // convert viewx to blocks
			{
				getWorld(rviewx_trigger + 40);

				rviewx_trigger += 16;
			}

		}

		updateDisplay(0); // Draw screen
		oledWriteInteger(10, 0, coincount);
		//  oledWriteInteger(80, 0, mymicros/1000000L);
		drawCoin(0, 0);
		updateDisplay(1);
		
		// 1E6 / fps
		if (gamestate == normal) handleMusic(&MusicController);
		handleMusic(&SoundEffectController);
	
		while (mymicros-loop_micros<=25000) // 40 fps
		{
			if (gamestate == normal) handleMusic(&MusicController);
			handleMusic(&SoundEffectController);
		}
		loop_micros = mymicros;
	}

}

uint8_t next_random(uint8_t range)
{
	curr_seed = (curr_seed*1664525 + 1013904223) % 65536;
	return (uint8_t) (curr_seed % range);
}

uint16_t readADC(uint8_t adcpin)
{
	uint16_t value=0;
	
	for (uint8_t i=0;i<16;i++)
	{
	ADCSRA &=~(1<<ADEN); // disable ADC Enable, to be safe
	
	//while (ADCSRA & (1<<ADSC)); // wait for ADSC bit to clear
	
	ADMUX = (adcpin & 7) << (MUX0); // REF bits are zero = Vcc as voltage reference
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);// divide by 16 | (1<<ADPS1);// | (1<<ADPS0); // Enable ADC in general
	
	ADCSRA |= (1<<ADSC); // Start ADC conversion
	while(ADCSRA & (1<<ADSC)); // ADSC goes to zero when finished
	//while(!(ADCSRA & (1<<ADIF)));
	value+=ADC;
	}
	return value>>4; // divide by eight, average
}

void drawSprite(int16_t x, int16_t y, uint8_t w, uint8_t h, const unsigned char *buf, uint8_t flip)
{
  // Do clipping:
  //if (x<0) x=0

  // Assume don't need clipping for now
  //ssd1306_send_command3(renderingFrame | (y >> 3), 0x10 | ((x & 0xf0) >> 4), x & 0x0f);
  //uint8_t offset_y = y & 0x07;

  int16_t sx, sy;
  unsigned char *pos = (unsigned char*)buf;
  uint8_t start_height=0, stop_height=h/8, xoff = 0, xw = w;
  // i.e. for write_higeht 16 means 2 lots of 8 bits.

  if (x >= 128 || x <= -w) return;
  if (y>63) return;
  if (x < 0) {
    xoff = -x;
    x = 0;
  }
  if (x + w > 128) xw = 128 - x;

  uint8_t temp[16];
  
  if (y<0)
  {
	start_height=1+((-y)>>3); // i.e. -7 to -1 (inclusive): start_height = 1
	y=0;
  }
  
  if (y+h>64) stop_height = 8-(y>>3);
	
  for (uint8_t j = start_height; j < stop_height; j++) // goes from 0 to 1 (across y-axis)
  {
    sx = x ;
    sy = y + (j-start_height) * 8;

    // SetCursor
    //ssd1306_send_command3(renderingFrame | (sy >> 3), 0x10 | ((sx & 0xf0) >> 4), sx & 0x0f);
    oledSetPosition(sx, sy);

    //ssd1306_send_data_start();

    // Copy into local buffer forwards or backswards, then pass on :)

    for (uint8_t i = xoff; i < xw; i++) // i.e. i goes 0 to 15 (across x axis) if not clipped
    {
      if (flip == 0) pos = (unsigned char*)buf + j + (i * (h>>3));
      else pos = (unsigned char*)buf + j + ((w - 1 - i) * (h>>3)); // h/8
      temp[i - xoff] =  pgm_read_byte(pos);
    }

    // cursor is incremented auto-magically to the right (x++) per write
    //     if (flip==0) pos=buf+j+(i*write_height);
    //   else pos=buf+j+((w-1-i)*write_height);

    oledWriteDataBlock(temp, xw - xoff); // DUNCAN - rewrite to allow whole row at once!
    //ssd1306_send_byte(pgm_read_byte(pos)); //DUNCAN

    //ssd1306_send_stop();
  }
}

void killGoomba(uint8_t g_id)
{
	goomba[g_id].state = squash;
	goomba[g_id].frame=17;
	goomba[g_id].vx=0;
	goomba[g_id].x++;//.x++ forces a blank replot
}
void spawnGoomba(uint16_t pos)
{	// Check here for goomba availability
	for (uint8_t g=0;g<MAX_GOOMBAS;g++)
	{
		if (goomba[g].state==dead)
		{
			goomba[g].x = (pos<<3);
			goomba[g].y = 0;
			goomba[g].vx = 1;
			goomba[g].vy = 0;
			goomba[g].frame = 0;
			goomba[g].state=idle;
			return;
		}	
	}
}
void getWorld(uint16_t vx)//, uint8_t screen_num)
{
  // vx is in units of bricks (i.e. vx/8), one screen is 16 wide
  
  uint8_t floor, gap=0;
  //uint16_t screen_num = vx>>4; // divide by 16
  uint8_t cx = (vx & 63);
  
  floor = 5 + next_random(3);  // can be 5, 6 or 7

  for (uint8_t i=cx; i<cx+16;i++)
  {
	screen[i]=0;
	
	if (!next_random(5)) screen[i]=1; // Spawn a coin
	
	if (!gap)
	{
		//SetColumn(ifloor,7);
		for (uint8_t f=floor;f<=7;f++)
			screen[i] |= (1<<f);
		if (!next_random(10)) // 1 in 10 chance of setting a gap
		{
			gap = 2 + next_random(4); // 2-5 
			if (vx==0) gap=0; // no gaps on first screen
		}
		else if (!next_random(5) && vx!=0) // change floor height
		{
			if (!next_random(2) && floor<7) floor++;
			else if (floor>1) floor--;
		}
		if (!next_random(16)) spawnGoomba((uint16_t)vx+8);// spawn half way across screen so there are no boundary issues on the right
	} 
	else gap--;
  }
/*
  for (i = cx; i < cx + w; i++) // cx is &WOLRD_MAX_LEN so cannot exceed 64.
  {
    //le = curr_seed & 0x07;
    //curr_seed  = (curr_seed*1664525 + 1013904223) % 65536; // next seed

    //element_ptr = (uint8_t *)levelelements[le];

    for (j = 0; j < 4; j++) // 4 = width of level element
    {
      if ((i + j) <= WORLD_MAX_LEN) screen[i + j] = *(element_ptr + j); //pgm_read_byte(element_ptr+j);
      else return;
    }

    i += 3; // width of level element - 1
  }*/
}

void drawScreen()
{
  uint8_t starti, stopi, wrapped_i=0, coin = 0, pipe = 0;
  int xoffset;

  starti = (viewx >> 3) & WORLD_MAX_LEN;  // wrap round (could be WORLD_BUFFER - 1)
  xoffset = -(viewx & 0x07);
  stopi = starti + 16;

  if (starti > 0) {
    starti--;  // Handles blanking blacking out edge when *just* disappeared off screen
    xoffset -= 8;
  }

  if (xoffset != 0) stopi++; // Need to add an extra brick on the right because we are not drawing left brick fully
  //if (stopi>WORLD_BUFFER) stopi=WORLD_BUFFER;

  for (uint8_t i = starti; i < stopi; i++) // limit of 256
  {
    if (delta_viewx < 0) wrapped_i = (i + 1) & WORLD_MAX_LEN; // wrap round (could be WORLD_BUFFER - 1)
    else if (delta_viewx > 0) wrapped_i = (i - 1) & WORLD_MAX_LEN;

    coin = 0;
    if (screen[i & WORLD_MAX_LEN] & (1 << 0)) coin = 1;

    //if ((screen[i & WORLD_MAX_LEN] & 0xFE) == 0b11100000) pipe++; // A pipe column (first or second...)
    //else pipe = 0;

    for (uint8_t y = 1; y < 8; y++)
    {
      if (screen[i & WORLD_MAX_LEN] & (1 << y)) // There is a block here
      {
        if (delta_viewx < 0) // screen is scrolling right and we are within array limit
          if (!(screen[wrapped_i] & (1 << y))) vblankout(xoffset + 8, y << 3, -delta_viewx); // View moving to the right, blocks scrolling to the left,
        // therefore blank out smearing trail to the right of block
        // but only if there isn't a block there.
        if (delta_viewx > 0)
          if (!(screen[wrapped_i] & (1 << y))) vblankout(xoffset - delta_viewx, y << 3, delta_viewx);


        if ((y == 5 || y == 6) && pipe != 0) // it's a pipe column
        {
          if (y == 5 && pipe == 2) drawSprite(xoffset - 8, y << 3, 16, 16, &mario_pipe2[0], 0);
          //drawSprite(xoffset, y<<3,8,8,&block8x8[0], 0);
        }
        else
        { // don't draw bricks on pipes
          //if (y == 7) drawSprite(xoffset, y << 3, 8, 8, &block8x8[0], 0);
          //else 
		  if ((screen[i & WORLD_MAX_LEN] & (1 << (y - 1))) == 0) // y>0 and nothing above, draw grassy brick
            drawSprite(xoffset, y << 3, 8, 8, &topbrick[0], 0);
          else drawSprite(xoffset, y << 3, 8, 8, &brick8x8[0], 0); // else brick is default
        }

        if (coin)
        {
          if (y == 1) drawCoin(xoffset, (y - 1) << 3);
          else drawCoin(xoffset, (y - 2) << 3);
          coin = 0;
        }
      }
      //else vblankout(xoffset,y<<3,8);
    }
    xoffset += 8;
  }
}

void playSoundEffect(uint8_t num)
{
  if (soundeffectplaying) return; // only one at once
  // 6 - coin collect sound

  tone_timer0_toggle_count = 0; // stop current sound

  SoundEffectController.music_pos = 0;
  SoundEffectController.music_index = num;
  SoundEffectController.noteordelay = 1;

  soundeffectplaying = 1;
}

void initMusic(struct toneController *controller)
{
  controller->music_pos = 0; // First position in music sequence, i.e. 0,1,2,3 etc )
  controller->music_index = music_seq[controller->music_pos];//pgm_read_byte(&music_seq[0]+music_pos); // melody 0 (=intro
  controller->music_note = 0; // First note of music_index melody
  controller->noteordelay = 1; //simulate end of a delay to start the note
}

void handleMusic(struct toneController *controller)//, uint8_t channel_id)
{
  uint16_t *melody;
  uint8_t numNotes;
  uint8_t *entry_pos;
  
  // soundeffect = 1: Timer0 is used for music - SPEAKER2
  // soundeffect = 0: Timer1 is used for sound effects - SPEAKER 

  if ((controller->soundeffect == 0 && tone_timer1_toggle_count == 0) || (controller->soundeffect == 1 && tone_timer0_toggle_count == 0 && soundeffectplaying == 1))
  {
    if (controller->noteordelay == 0) // 0 = NOTE just ended, therefore now delay
    {
      // Note ended, start a delay
      controller->current_delay = (controller->current_delay/3);

      if (controller->soundeffect == 0) mytone(0, controller->current_delay,1);
	  else mytone(0, controller->current_delay,0);

      // Advance to next note position
      controller->music_note++;								// sizeof(music_table)
      numNotes = eeprom_read_byte((uint8_t *)(controller->music_index * 3) + 2); // next position is numNotes

      if (controller->music_note >= numNotes) // exceeded notes in this melody part
      {
        controller->music_note = 0;
        controller->music_pos++; // advance sequence position

        if (controller->soundeffect == 1) // we are in the sound effect handler
        {
          soundeffectplaying = 0; // Global, control is passed back onto music handler, music will restart at next tone.
        }
        else if (controller->music_pos >= 12) // In music handler, and sequence has overflowed.
        {
          controller->music_pos = 1; // We are in the music handler
        }
        controller->music_index = music_seq[controller->music_pos];//pgm_read_byte(&music_seq[0]+music_pos);
      }
      controller->noteordelay = 1; //delay is next
    } // end note ended
    else if (controller->noteordelay == 1) // 1 = DELAY
    {
      // Delay ended, play next note
      entry_pos = (uint8_t *)(controller->music_index * 3);//sizeof(music_table);
      melody = (uint16_t *)eeprom_read_word((uint16_t*)entry_pos); // EEPROM position in entry table
      entry_pos += 2;

      //numNotes = eeprom_read_byte(entry_pos); // next position is numNotes
      //noteDurations = (uint8_t*)(melody + (numNotes)); // noteDurations follows the melody in EEPROM

      uint16_t ms = 1000, freq;

      if (controller->soundeffect == 0) ms = MusicSpeed; // Music speed might change

      freq = eeprom_read_word(melody + controller->music_note);  // duration is now encoded into top 3 bits of frequency.
	  uint8_t note;
	  note = freq >> 13;// Extract 2,4,8,16th,32nd note etc from freq
	  freq &= 0x1FFF;
	  controller->current_delay = ms>>note; // shift rather than divide.
	  
      if (controller->soundeffect == 0) mytone(freq, controller->current_delay,1);
      else mytone(freq, controller->current_delay,0); // Sound effects
     
      controller->noteordelay = 0;
    }// end delay ended
  } // wnd time delay lapsed
}

void mytone(unsigned long frequency, unsigned long duration, uint8_t timer) // Hard coded for pin PB1 (OCR0A)
{
  uint32_t ocr;
  uint8_t prescalarbits = 0b001;
  uint8_t output = 1;

// soundeffect = 0: 1 passed onto myTone: Timer1 is used for Music - SPEAKER
// soundeffect = 1: 0 passed onto myTone: Timer0 is used for sound effects - SPEAKER2

  // timer = 0 - use timer 0 and SPEAKER2 pin
  // timer = 1 - use timer 1 and SPEAKER pin
  
  // Try music on PB1, using OC1A (rather than OC1B on PB4)

  if (frequency == 0)
  { // Means we want to switch off the pin and do a pause, but not output to the hardware pin
    output = 0; frequency = 300; // A dummy frequency - timings are calculated but no pin output.
  }

  ocr = F_CPU / (2 * frequency);
  
  if (timer==0)
  {
    if (ocr > 256)
    {
      ocr >>= 3; //divide by 8
      prescalarbits = 0b010;  // ck/8
      if (ocr > 256)
      {
        ocr >>= 3; //divide by a further 8
        prescalarbits = 0b011; //ck/64
        if (ocr > 256)
        {
          ocr >>= 2; //divide by a further 4
          prescalarbits = 0b100; //ck/256
          if (ocr > 256)
          {
            // can't do any better than /1024
            ocr >>= 2; //divide by a further 4
            prescalarbits = 0b101; //ck/1024
          } // ck/1024
        }// ck/256
      }// ck/64
    }//ck/8*/
  } //timer==0
  else
  {
    prescalarbits = 1;
    while (ocr > 0xff && prescalarbits < 15) {
          prescalarbits++;
          ocr>>=1;
    }
  }
   
  ocr -= 1;

  if (timer==0)
  {
    tone_timer0_toggle_count = (2 * frequency * duration) / 1000;
  }
  else
  {
    // timer1 scalar code here
    tone_timer1_toggle_count = (2 * frequency * duration) / 1000;
    // Music uses timer1 and is on continuously so use as a crude timer
    ISR_micro_period = 500000L / frequency; // 1E6 / (2*frequency), because ISR is called 2 times every period.
  }

  if (output)
  {
    if (timer==0)
    {
    TCCR0A = (1 << COM0B0) | (1 << WGM01); // = CTC // Fast PWM | (1<<WGM01) | (1<<WGM00);
    }
    else 
    {
	#ifndef SINGLE_SPEAKER
		GTCCR = (1 << COM1B0); // set the OC1B to toggle on match
	#endif
    }
  }
  else // no output 
  {
    if (timer==0)
    {
      TCCR0A |= (1 << WGM01); // I guess this is always true
      TCCR0A &= ~(1 << COM0B0); // No hardware pin output 
	  #ifndef SINGLE_SPEAKER
		PORTB &= ~(1 << SOUNDEFFECT_SPEAKER); // Set pin LOW
	  #endif
    }
    else
    {
      // timer 1 code to turn off hardware support here
	  #ifdef SINGLE_SPEAKER
		  TCCR1 &= (1 <<COM1A0);  // disconnect OC1A
	  #else 
		  GTCCR &= (1 <<COM1B0); // disconnect hardware pin
		  PORTB &= ~(1 << MUSIC_SPEAKER); // Set pin LOW
	  #endif
    }
  }

  if (timer==0)
  {
    TCCR0B = prescalarbits << CS00; //(1<<CS01);// | (1<<CS00); // Scalar. //prescalarbits;//
    TCNT0 = 0; // Set timer0 counter to 0
    OCR0A = ocr; // set compare value
    TIMSK |= (1 << OCIE0A); // Activate timer0 COMPA ISR
  }
  else
  {
    TCCR1 = (1<<CTC1)| (prescalarbits<<CS10); // CTC1 : Clear Timer/Counter on Compare Match, after compare match with OCR1C value
	#ifdef SINGLE_SPEAKER
		if (output) TCCR1 |= (1<<COM1A0);
	#endif
    TCNT1 = 0; // timer 1 counter = 0
    OCR1C = ocr; // set compare value 
	#ifdef SINGLE_SPEAKER
		TIMSK |= (1 << OCIE1A); // Activate timer1 COMPA ISR
	#else
		TIMSK |= (1 << OCIE1B); // Activate timer1 COMPB ISR
	#endif
  }
  
}

#ifdef SINGLE_SPEAKER
ISR(TIMER1_COMPA_vect) {
#else
ISR(TIMER1_COMPB_vect) {
#endif
  if (tone_timer1_toggle_count != 0)
  {
    if (tone_timer1_toggle_count > 0)
    {
      tone_timer1_toggle_count--;
      if (tone_timer1_toggle_count == 0)
      {
        // turn off tone
		#ifdef SINGLE_SPEAKER
			TCCR1 &= (1 <<COM1A0); 
		#else
			GTCCR &= ~(1 << COM1B0); // Disconnect OC1B pin
		#endif
        return;
      }
    }
  }
  mymicros += ISR_micro_period;
}

ISR(TIMER0_COMPA_vect) {
  if (tone_timer0_toggle_count != 0)
  {
    if (tone_timer0_toggle_count > 0)
    {
      tone_timer0_toggle_count--;
      if (tone_timer0_toggle_count == 0)
      {
        // turn off tone
        //TCCR0A = 0;
        TCCR0A &= ~(1<<COM0B0);
        TIMSK &= ~(1 << OCIE0A); // Turn off interrupt bit.
        return;
      }
    }
  }
}

void handlemap_collisions(struct character *player)
{
  // Only apply velocities if no collisions in that direction, else clip to collision point.
  int newmario_x, newmario_y;
  uint8_t cellx, celly, ybitmask;
  uint16_t cellx_zone;

  //if (player->coincollector)
  //{
	if (player->vy < -7) player->vy = -7; //
  //}
  //else if (player->vy<-1) player->vy = -1; //non-coin collectors (i.e. goombas) have limited gravity
  
  newmario_x = player-> x + player->vx;
  newmario_y =  player->y + player->vy;
  player->collision = 0; // 1 = UP, 2=RIGHT, 4=DOWN, 8=LEFT

  //if ((newmario_x-(int16_t)viewx)<0) newmario_x=viewx;

  cellx_zone = (newmario_x >> 9); // 0-31 = 0, 32-63=1, 64-95=2, 96-127=3. >>8 = 3 + (log2 WORLD_BUFFER);

  // 0-63 = 0, 64-127 = 1 ... therefore 64*8 = 2^6 * 2^3 = 2^9 = 512
  cellx = (newmario_x >> 3) & WORLD_MAX_LEN;
  celly = newmario_y >> 3; // generate bit-mask for byte describing y-column in screen data.
  uint8_t yoffset = newmario_y & 0x07, xoffset = newmario_x & 0x07;

  //ybitmask = 1 << celly;
  if (yoffset==0) ybitmask = (player->mask) << celly;
  else ybitmask = ((player->mask<<1)+1)<<celly; // add an extra cell 
  
  if (newmario_y < 0) {
    celly = 0;  // means that a block is never found and mario can jump through the top of the screen!
    ybitmask = 0;
  }

  //if (ybitmask == 1) {
//    ybitmask = 0; // means no collisions with row0
 // }
 ybitmask&=~(1<<0); // means no collisions with row0

  uint8_t check_cell, check_cell2;

  if (player->vx > 0) // Check Mario's righthand side if he tried to move to the right
  {                 
    check_cell = (cellx + player->w) & WORLD_MAX_LEN; // WORLD_BUFFER - 1
    
    //if (screen[check_cell]&ybitmask || screen[check_cell] & (ybitmask << 1) || (yoffset != 0 && screen[check_cell] & (ybitmask << 2)))
    if (screen[check_cell]&ybitmask) // that's all
    { // clip
      // Strictly we should update cellx_zone to be relevant to the zone of the block that caused the collision
      player->x = (cellx_zone << 9) + (cellx << 3);
      if (cellx == 0) cellx = WORLD_MAX_LEN; else cellx -= 1; // handle wrap
      player->collision |= 2; // 1 = UP, 2=RIGHT, 4=DOWN, 8=LEFT
      player->vx = 0;
    }
    else
    {
      player->x = newmario_x;
    }
  }
  else if (player->vx < 0)
  {
    //player->x=newmario_x;
   // if (screen[cellx]&ybitmask || screen[cellx] & (ybitmask << 1) || (yoffset != 0 && screen[cellx] & (ybitmask << 2)))
   if (screen[cellx]&ybitmask) 
    { // clip
      ///cellx=(cellx+1) & 31; // if collided with left, push back to the right - handle wrap
      cellx++;
      if (cellx > WORLD_MAX_LEN) {
        cellx = 0;
        cellx_zone++;
      }
      player->x = (cellx_zone << 9) + (cellx << 3); // Strictly we should update cellx_zone to be relevant to the zone of the block that caused the collision
      player->collision |= 8; // 1 = UP, 2=RIGHT, 4=DOWN, 8=LEFT
      player->vx = 0;
    }
    else
    {
      player->x = newmario_x;
    }

  }

  //
  // celly+2 should be valid at this point
  
  ybitmask = 1 << (celly+player->w);
  ybitmask&=~(1<<0); // means no collisions with row 0
  
  if (player->vy > 0 || player->vy == 0) // going down
  {
    check_cell = (cellx + 1) & WORLD_MAX_LEN;
    check_cell2 = (cellx + 2) & WORLD_MAX_LEN;
    //if ((celly + 2 >= 8) || screen[cellx] & ybitmask  || screen[check_cell] & ybitmask  || (xoffset != 0 && screen[check_cell2] & ybitmask))
	if (screen[cellx] & ybitmask  || screen[check_cell] & ybitmask  || (xoffset != 0 && screen[check_cell2] & ybitmask))
    { // clip
      player->vy = 0; player->jumpstate = nojump;
      if (celly + 2 >= 8) celly = 6;
      player->y = (celly << 3);
      player->collision |= 4; // 1 = UP, 2=RIGHT, 4=DOWN, 8=LEFT
      player->vy = 0;
    }
    else
    {
      if (player->vy == 0 && player->jumpstate == nojump) {
        player->jumpstate = jumpdown;
      }
      player->y = newmario_y; // allow new vertical position
    }
  }
  else if (player->vy < 0) // Have we hit our head?
  {
    ybitmask = 1 << (celly);
    ybitmask&=~(1<<0); // means no collisions with row 0
  
    //player->y=newmario_y;// allow new vertical position
    check_cell = (cellx + 1) & WORLD_MAX_LEN; // WORLD_BUFFER -1
    check_cell2 = (cellx + 2) & WORLD_MAX_LEN; // WORLD_BUFFER -1

    if (screen[cellx] & (ybitmask) || screen[check_cell] & (ybitmask) || (xoffset != 0 && screen[check_cell2] & (ybitmask)))
    { // clip
      player->vy = 0; player->jumpstate = jumpdown;
      celly += 1;
      player->y = (celly << 3);
      if (player->coincollector) playSoundEffect(HITHEAD_SOUND);
      player->collision |= 1; // 1 = UP, 2=RIGHT, 4=DOWN, 8=LEFT
      player->vy = 0;
    }
    else
    {
      //if (mario.vy==0 && mario.jumpstate==nojump) {mario.jumpstate=jumpdown;}
      player->y = newmario_y; // allow new vertical position
    }
  }
  // Coin collision

  // mario is at: cellx, cellx+1 (and if xoffset!=0, cellx+2)
  // and at: celly, celly+1 (and if yoffset!=0, celly+2)

  if (player->coincollector)
  {
    collidecoins(cellx, celly, yoffset);
    collidecoins(cellx + 1, celly, yoffset);
    if (xoffset != 0) collidecoins(cellx + 2, celly, yoffset);
  }
}

void collidecoins(uint8_t cx, uint8_t celly, uint8_t yoffset)
{
  uint8_t coiny = 8, h;

  h = 2; if (yoffset != 0) h = 3;

  cx = cx & WORLD_MAX_LEN;

  if (screen[cx] & 1) // Is there a coin in this column?
  {
    coiny = findcoiny(cx);
    if (coiny != 8)
    {
      //if (collideMarioblock(cx, coiny))
      if (coiny >= celly && coiny < (celly + h))
      {
        screen[cx] &= 254;
        playSoundEffect(COIN_SOUND);
        coincount++;
        
        //starti = (viewx >> 3) & 31;
        //vblankout((cx<<3)-(viewx & 127), coiny*8,16);

        // This function is only called when screen = 0, address 0x3D (left screen)
        if (mario.x > (viewx + 128)) oled_addr = 0x3C; // right hand side screen
        for (uint8_t x = 0; x < 8; x++) vblankout(x << 4, coiny * 8, 16);
        oled_addr = 0x3D; //set address back to be safe
      }
    }
  }

}

uint8_t collideThings(int mx, int my, int mw, int mh, int tx, int ty, uint8_t w, uint8_t h)
{
  if ((mx + mw) < tx || mx > (tx + w)) return 0;
  if ((my + mh) < ty || my > (ty + h)) return 0;

  return 1;
}

uint8_t collideMario(int tx, int ty, uint8_t w, uint8_t h)
{
  if ((mario.x + 8 + 4) < tx || (mario.x + 4) > (tx + w)) return 0;
  if ((mario.y + 8 + 4) < ty || (mario.y + 4) > (ty + h)) return 0;

  return 1;
}

uint8_t findcoiny(uint8_t cx)
{
  for (uint8_t y = 1; y < 8; y++)
  {
    if (screen[cx & WORLD_MAX_LEN] & (1 << y)) // There is a block here
    {
      if (y == 1) return 0; //drawCoin(xoffset, (y-1)<<3);
      else return y - 2; //drawCoin(xoffset, (y-2)<<3);
    } // if found block
  } // for y
  return 8; // 8 means couldn't find a coin.
}

void animate_character(struct character *player)
{

  // Jumping
  if (player->jumpstate != nojump)
  {
    player->vy += 1; // Gravity
    if (player->vy >= 0 && player->jumpstate == jumpup) player->jumpstate = jumpdown; // flip state for animation
	if (player->y>80) {player->state=dead; playSoundEffect(HITHEAD_SOUND);}
  }
  else // idle, left or right
  {
    //player->frame++;//1-mario.frame; // animate frame

  }
}

void draw_mario()
{
  // Drawing  ***************************************
  if (mario.jumpstate == nojump) // idle, left or right
  {
    if (mario.state == idle) drawSprite(mario.x - viewx, mario.y, 16, 16, &mario_idle[0], mario.dir);
    else if (mario.state == left || mario.state == right)
    {
      if (mario.frame < 3) drawSprite(mario.x - viewx, mario.y, 16, 16, &mario_walk1[0], mario.dir);
      else drawSprite(mario.x - viewx, mario.y, 16, 16, &mario_walk2[0], mario.dir);
    } // end: left or right walking
  } // end: not jumping
  else if (mario.jumpstate == jumpup) drawSprite(mario.x - viewx, mario.y, 16, 16, &mario_jump1[0], mario.dir);
  else if (mario.jumpstate == jumpdown) drawSprite(mario.x - viewx, mario.y, 16, 16, &mario_jump2[0], mario.dir);
}

void blank_character(uint8_t screen_id, struct character *player)
{
  //static int oldmario_x, oldmario_y; //initialised


  //if (screen_id==0) // left - for now do this, later will have to store for each screen
  //{

  if ((player->x - (int16_t)viewx) != player->oldx[screen_id] || (player->y != player->oldy[screen_id])) // Blankout old mario position sprite if we've moved
  {
    vblankout(player->oldx[screen_id], player->oldy[screen_id], 16);
    vblankout(player->oldx[screen_id], player->oldy[screen_id] + 8, 16);
  }

  //oldmario_x = mario.x-(int16_t)viewx; oldmario_y = mario.y;
  player->oldx[screen_id] = player->x - (int16_t)viewx;
  player->oldy[screen_id] = player->y;
  //}
}

void vblankout(int sx, int sy, uint8_t w)//, uint8_t h)
{
  // Set cursor @ top left corner

  if (sx + w <= 0 || sx >= 128) return; // No amount of vertical strip will be visible

  if (sx < 0) {
    w = sx + w;
    sx = 0;
  }
  if (sx + w > 128) {
    w = 128 - sx;
  }
 
  if (sy<-7 || sy>63) return;
  if (sy<0) sy=0; // i.e. between -7 and -1 
  
  //ssd1306_send_command3(renderingFrame | (sy >> 3), 0x10 | ((sx & 0xf0) >> 4), sx & 0x0f);

  oledSetPosition(sx, sy);
  //ssd1306_send_data_start();

  uint8_t temp[16] = {0};

  oledWriteDataBlock(temp, w);

  /*
    for (uint8_t i=0; i<w; i++)// i.e. i goes 0 to 15 (across x axis)
    {
    //  ssd1306_send_byte(0x00); // DUNCAN
    }*/
  ////ssd1306_send_stop();

}

#define buttonBASE 22
#define button2BASE 22

void readbuttons(void)
{
  uint16_t sensorValue, sensor2Value;
  uint8_t buttons,buttons2;

  sensorValue = readADC(ADC3);//analogRead(A3); // was A2 / A3
  sensor2Value = 1023 - readADC(ADC0);

#ifdef TEST_BUTTONS
  uint8_t buf[10];

  //oledFill(0);
  itoa(pause_button, buf, 10);
  oledWriteString(0, 0, buf);

  itoa(pause_debounce, buf, 10);
  oledWriteString(40, 0, buf);

  itoa(gamestate, buf, 10);
  oledWriteString(70, 0, buf);
#endif

  buttons = (sensorValue + (buttonBASE / 2)) / buttonBASE; //round(sensorValue/buttonBASE);
  buttons2 = (sensor2Value + (button2BASE/2)) / button2BASE;
  
//  oledWriteInteger(50,30,(uint8_t)sensor2Value);
//  oledWriteInteger(50,40,(uint8_t)buttons2);
  
  if (gamestate == normal) // can only move if not paused, etc.
  {
    // Moving (independent of jumping)
    if (buttons & 1)
    {
      // if (mario.state!=right) {mario.vx++; mariosetvx=mario.vx;mario_acc_timer=30;}

      mario.state = right; mario.dir = faceright;
      //mario_acc_timer++;if (mario_acc_timer>30 && mario.vx<3) {mario_acc_timer=0;mario.vx++;mariosetvx=mario.vx;}
      //mario.vx=mariosetvx;
      mario.vx = 3;
    } // RIGHT
    else if (buttons & 4)
    {
      //if (mario.state!=left) {mario.vx--;mariosetvx=mario.vx;mario_acc_timer=0;}

      mario.state = left; mario.dir = faceleft;
      //mario_acc_timer++;if (mario_acc_timer>30 && mario.vx>-3) {mario_acc_timer=0;mario.vx--;mariosetvx=mario.vx;}
      mario.vx = -3; //mariosetvx;
    } // LEFT
    else
    {
      // De-accelerate
      if (mario.vx > 0) mario.vx -= 1;
      else if (mario.vx < 0) mario.vx += 1;
    }


    // Jumping
    if (buttons & 2 && mario.jumpstate == nojump) {
      mario.vy = -7;  // Only start jumping if not currently jumping
      mario.jumpstate = jumpup;
      playSoundEffect(JUMP_SOUND);
    }

    // Idle if not jumping and no keys pressed
    if (buttons == 0 && mario.jumpstate == nojump) mario.state = idle;
	
	if (buttons2 & 4) // Fire!!!
	{
		if (fireball.state==dead)
		{
			fireball.frame=0;
			fireball.x=mario.x+16; fireball.y=mario.y; fireball.vy=FIREBALL_SPEED; fireball.vx=FIREBALL_SPEED; fireball.state=idle;
			if (mario.dir==faceleft) {fireball.vx=-FIREBALL_SPEED;fireball.x=mario.x;}
		}	
	}
  } // end gamestate == normal

  if (buttons2 & 1)
  {
    if (gamestate == normal)
    {
      playSoundEffect(PAUSE_SOUND);
      oledWriteCommand2(0x81, 16);
      oled_addr = 0x3C;
      oledWriteCommand2(0x81, 16);
      oled_addr = 0x3D;
      gamestate = paused;
    }
    else if (gamestate == paused_ready) {
      gamestate = pause_return;
    }
  } // buttons2 == 0 (released pause button)
  else if (gamestate == paused)
  {
    gamestate = paused_ready;
  }
  else if (gamestate == pause_return)
  {
    gamestate = normal;
    oledWriteCommand2(0x81, 0x7F);
    oled_addr = 0x3C;
    oledWriteCommand2(0x81, 0x7F);
    oled_addr = 0x3D;
  }

}

void drawCoin(int sx, uint8_t sy)
{
  // Do coin(s)
  //  if (delta_viewx<0) vblankout(90-viewx+8,0,-delta_viewx);
  //  else if (delta_viewx>0) vblankout(90-viewx-delta_viewx,0,delta_viewx);

  if (delta_viewx < 0) vblankout(sx + 8, sy, -delta_viewx);
  else if (delta_viewx > 0) vblankout(sx - delta_viewx, sy, delta_viewx);

  if (coinframe == 1)
    drawSprite(sx, sy, 8, 8, &coin1[0], 0);
  else if (coinframe == 2 || coinframe == 4)
    drawSprite(sx, sy, 8, 8, &coin2[0], 0);
  else if (coinframe == 3)
    drawSprite(sx, sy, 8, 8, &coin3[0], 0);
}

void updateDisplay(uint8_t screen_id)
{
 // static int bally=0, ballvy=1;
  
  if (screen_id == 0) // Left screen
  {
    delta_viewx = old_viewx - viewx; // -ve means moving to right, +ve means moving to left.
    if (delta_viewx > 8) delta_viewx = 8; // Limit delta_viewx
    if (delta_viewx < -8) delta_viewx = -8;
    oled_addr = 0x3D;

   // vblankout(0,bally,8);
    //bally+=ballvy;
    //if (bally<0) {bally=0;ballvy=1;}
    //else if (bally>56) {bally=55;ballvy=-1;}
    
    //drawSprite(0,bally,8,8,&block8x8[0],0);
  }
  else // right hand screen  (0x3c)
  {
    viewx = viewx + 128;
    oled_addr = 0x3C;
  }

  drawScreen(); // Draws level scenery, coins, etc.
  
  // Draw fireball, goomba and other sprites etc
  if (fireball.state!=dead)
  {
	if (fireball.y>=0 && fireball.y<=63) // only if on screen
	{
		blank_character(screen_id, &fireball);
		if (fireball.frame<90) drawSprite(fireball.x-viewx, fireball.y,8,8,&fire[0],0); // this makes sure we blank before making dead
	}
  }
  
  blank_character(screen_id, &mario);
  draw_mario();

  for (uint8_t g=0; g<MAX_GOOMBAS;g++)
  {
		if (goomba[g].state!=dead) 
		{
			blank_character(screen_id, &goomba[g]);
			if (goomba[g].state!=squash) drawSprite(goomba[g].x - viewx, goomba[g].y, 16, 16, &goomba1[0], (goomba[g].frame > 3));
			else if ((goomba[g].frame>>4)&1) drawSprite(goomba[g].x - viewx, goomba[g].y+8, 16, 8, &squashed[0], 0);
			else vblankout(goomba[g].x - viewx,goomba[g].y+8,16);
		}
  }
  
  if (screen_id == 0) old_viewx = viewx; // left screen
  else {
    viewx = viewx - 128;  // restore previous viewx
    oled_addr = 0x3D;
  }
}

void oledWriteInteger(uint8_t x, uint8_t y, uint8_t number)
{
  uint8_t n, n_div = 100;
  oledSetPosition(x, y);

  for (uint8_t i = 0; i < 3; i++)
  {
    n = number / n_div; // 0-255, so n is never > 10
    oledWriteDataBlock(&ucSmallFont[n * 6], 6); // write character pattern
    number = number - n_div * n;
    n_div = n_div / 10; // 100 --> 10 --> 1
  }
}

