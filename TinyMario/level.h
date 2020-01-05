// Contains structures to represent level features for procedural level generation

static const unsigned char steps_up[] = {
0b11000000,0b11100000,0b11110000, 0b11111001
};

/*static const unsigned char steps_down[] PROGMEM = {
	0b11111000,0b11110000,0b11100000, 0b11000000
};*/

static const unsigned char gap[]  = {
	0b10000000,0b0,0b0, 0b10000000
};
/*
static const unsigned char biggap_platform[] PROGMEM = {
	0b10010000,0b10010000,0b10010000,0b10010000
};*/
static const unsigned char platform[] = {
	0b10010000,0b10010000,0b10010000,0b10010001
};/*
static const unsigned char platform2[] PROGMEM = {
  0b10011000,0b10011000,0b10011000,0b10010000
};*/
static const unsigned char flat[] = {
  0b10000000,0b10000000,0b10000000,0b10000000
};
static const unsigned char flat2[] = {
  0b10000000,0b10000000,0b10000001,0b10000001
};
static const unsigned char pipe[] = {
  0b10000000,0b11100000,0b11100000,0b10000000
};                  
static const unsigned char climb[] = {
  0b10010000,0b10011000,0b10011000,0b10010000
};
