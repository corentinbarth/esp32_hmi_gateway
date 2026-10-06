#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <SPI.h>
#include <SD.h>

/*
MISO GPIO25
SCK GPIO33
MOSI GPIO27 
CS GPIO32 
VCC 3.3V 
GND GND*/

// Codes de retour
#define SD_OK   0   // Tout s'est bien passé
#define SD_ERR  -1   // problème 



// ── Pins du module SD ─────────────────────────────────────────────────
#define SD_MISO 25
#define SD_SCK  33
#define SD_MOSI 27
#define SD_CS   32

/*SD card initialization*/
// return SD_OK if OK and SD_ERR_OPEN if error upon opening 
int init_sd()

// Browse the root of the SD card
// Format : "fichier1.txt,fichier2.jpg,fichier3.mp3"
//return the number of read file and -1 if error
int get_File_List(char file_names[][64], int maxfiles)


#endif