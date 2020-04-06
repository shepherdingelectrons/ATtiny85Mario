// https://github.com/mikemalburg/arduino_annoyotrons_piezo/blob/master/annoy_piezo_mario_overworld/annoy_piezo_mario_overworld.ino

static const uint16_t CoinSound[] =
{
	NOTE_B5, NOTE_E6
};

static const uint8_t CoinSound_Durations[] =
{
	8,4
};

/*
static const uint16_t JumpSound[] PROGMEM =
{
  NOTE_GS4, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3, NOTE_C4, NOTE_CS4,NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4
};

static const uint8_t JumpSound_Durations[] PROGMEM=
{
  128,32,128,128,128,128,128,128,128,128,128,128,128,128
};*/

static const uint16_t  HitHeadSound[] PROGMEM=
{
  NOTE_C3
};

static const uint8_t HitHead_Durations[] PROGMEM=
{
 16
};

static const uint16_t  JumpSound[] PROGMEM =
{
  NOTE_C6, NOTE_G6, NOTE_C7
};

static const uint8_t JumpSound_Durations[] PROGMEM=
{
 32,32,32
};

static const uint16_t Melody_Intro[] = 
{
  NOTE_E4, NOTE_E4, 0, NOTE_E4,
  0, NOTE_C4, NOTE_E4, 0,
  NOTE_G4, 0, 0,
  NOTE_G3, 0, 0,
 
};
static const uint8_t NoteDurations_Intro[] = 
{ 
  8, 8, 8, 8,
  8, 8, 8, 8,
  8, 4, 8,
  8, 4, 8,
};

static const uint16_t  Melody_Refrain[] PROGMEM=
{
  // Refrain
  NOTE_C4, 0, NOTE_G3,
  0, NOTE_E3, 0,
  NOTE_A3, 0, NOTE_B3,
  0, NOTE_AS3, NOTE_A3, 0,
  
  NOTE_G3, NOTE_E4, NOTE_G4,
  NOTE_A4, 0, NOTE_F4, NOTE_G4,
  0, NOTE_E4, 0, NOTE_C4,
  NOTE_D4, NOTE_B3, 0
};

static const uint8_t NoteDurations_Refrain[] PROGMEM = 
{
  8, 4, 8,
  4, 8, 4,
  8, 8, 8,
  8, 8, 8, 8,
  
  4, 4, 4,
  8, 8, 8, 8,
  8, 8, 8, 8,
  8, 8, 4
};

// Pages: 2 - 3
static const uint16_t  Melody_Part2_1[] =
{
	0, NOTE_G4, NOTE_FS4,
	NOTE_F4, NOTE_DS4, 0, NOTE_E4,
	0, NOTE_GS3, NOTE_A4, NOTE_C4,
	0, NOTE_A4, NOTE_C4, NOTE_D4,
};
static const uint8_t NoteDurations_Part2_1[] =
{
	4, 8, 8,
	8, 8, 8, 8,
	8, 8, 8, 8,
	8, 8, 8, 8,
};

static const uint16_t  Melody_Part2_2[] =
{
	0, NOTE_G4, NOTE_FS4,
	NOTE_F4, NOTE_DS4, 0, NOTE_E4,
	0, NOTE_C5, 0, NOTE_C5,
	NOTE_C5, 0, 0,

};
static const uint8_t NoteDurations_Part2_2[] =
{
	4, 8, 8,
	8, 8, 8, 8,
	8, 8, 8, 8,
	8, 8, 4
};

static const uint16_t  Melody_Part2_3 [] =
{
	0, NOTE_DS4, 0,
	0, NOTE_D4, 0,
	NOTE_C4, 0, 0,
	0
};
static const uint8_t NoteDurations_Part2_3 [] PROGMEM=
{
	4, 8, 8,
	8, 8, 4,
	8, 8, 4,
	2
};

// Page 4
static const uint16_t  Melody_Part3_1[] PROGMEM=
{
  NOTE_C4, NOTE_C4, 0, NOTE_C4,
  0, NOTE_C4, NOTE_D4, 0,
  NOTE_E4, NOTE_C4, 0, NOTE_A4,
  NOTE_G3, 0, 0,

  NOTE_C4, NOTE_C4, 0, NOTE_C4,
  0, NOTE_C4, NOTE_D4, NOTE_E4,
  0,

  NOTE_C4, NOTE_C4, 0, NOTE_C4,
  0, NOTE_C4, NOTE_D4, 0,
  NOTE_E4, NOTE_C4, 0, NOTE_A4,
  NOTE_G4, 0, 0,
  NOTE_E4, NOTE_E4, 0, NOTE_E4,
  0, NOTE_C4, NOTE_E4, 0,
  NOTE_G4, 0, 0,
  NOTE_G3, 0, 0,

};
static const uint8_t NoteDurations_Part3_1[] PROGMEM= 
{
  8, 8, 8, 8, 
  8, 8, 8, 8, 
  8, 8, 8, 8, 
  8, 8, 4,

  8, 8, 8, 8, 
  8, 8, 8, 8, 
  1,
 
  8, 8, 8, 8, 
  8, 8, 8, 8, 
  8, 8, 8, 8, 
  8, 8, 4,
  8, 8, 8, 8, 
  8, 8, 8, 8, 
  8, 8, 4,
  8, 8, 4,
   
  
};
