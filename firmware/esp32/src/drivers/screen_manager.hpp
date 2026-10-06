#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Adafruit_SSD1306.h>

/*
Pins de branchement
VCC 3.3V
GND GND
SCL GPIO22
SDA GPIO21*/


// return code
#define SCREEN_OK   0   //everything ok
#define SCREEN_ERR  -1   //something wrong happened

/*screen initialisation*/
int screen_init()

/*Display string from string*/
void screen_display_char(char* text, int text_size)

/*Display menu from array of filenames*/
void screen_display_menu(char fileList[][64], int file_count, int selectedIndex)

#endif