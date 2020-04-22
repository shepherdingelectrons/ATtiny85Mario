# ATtiny85Mario

![ATtiny85 Mario](https://1.bp.blogspot.com/-czGPkAP6zps/XhzPi6E_isI/AAAAAAAAA2w/S9NqXxgfAWMCNwzo1DuLqfwZ18h-fWzTQCLcBGAsYHQ/s1600/Mario_ATtiny85.jpg)

Read about the software and hardware of ATtiny85Mario here:
https://shepherdingelectrons.blogspot.com/2020/01/tinymario.html

and the construction of a handset to play ATtiny85Mario in:
https://shepherdingelectrons.blogspot.com/2020/03/attiny-mario-handset.html

![mario animation](https://1.bp.blogspot.com/-wQc9w1t_zNc/Xm0aSpYDvUI/AAAAAAAAA5U/FX9NpMwJBkk7XVnXdGtJghxoaZgVtxxPwCLcBGAsYHQ/s1600/mario_gif2.gif)

Mario-style game with sound effects and music on twin OLED displays driven by ATtiny85 AVR. Having fun with the multiple challenges of working with small system constraints! :-)



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
