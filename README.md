# ATtiny85Mario
Mario-style game with sound effects and music on twin OLED displays driven by ATtiny85 AVR. Having fun with the multiple challenges of working with small system constraints! :-)

A project (that will be detailed) on my blog https://shepherdingelectrons.blogspot.com/

**TinyMario**
Source code for Mario.  For music and sound effects to work, the EEPROM must be correctly burned first (see BurnEEPROM_ATtiny85 sketch)

- OLED code is derived from bitbang2's super fast code:
https://github.com/bitbank2/oled_turbo
- Mario theme music is taken from Mike Marburg's transcription of the score:
https://github.com/mikemalburg/arduino_annoyotrons_piezo/tree/master/annoy_piezo_mario_overworld
- Mario animation pixel art:
https://www.hackster.io/138689/pixel-art-on-oled-display-7f8697

**BurnEEPROM_ATtiny85**

Arduino sketch to compress the music and sound effects, then burn to ATTiny85 EEPROM  
