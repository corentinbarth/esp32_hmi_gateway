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

// Initialise la carte SD
// Retourne SD_OK si OK, SD_ERR_OPEN si erreur à l'ouverture
int init_sd(){

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    //verif begin
    if (!SD.begin(SD_CS)) {
        Serial.println("Erreur : carte SD non détectée");
        return SD_ERR;
    }

    Serial.println("Carte SD détectée !");
    return SD_OK;
}


// Parcourt la racine de la carte SD
// Format : "fichier1.txt,fichier2.jpg,fichier3.mp3"
//retourne le nombre de fichier lu, -1 si erreur

int get_File_List(char file_names[][64], int maxfiles) {

    File root = SD.open("/");

    //verif ouverture
    if (!root) {
        Serial.println("Erreur : impossible d'ouvrir la racine");
    return SD_ERR;
    }

    File file = root.openNextFile();

   

    Serial.println("Fichiers : ");

    int i=0; //initialisation compteur

    while (file) {
        if (!file.isDirectory()){ //on ignore les sous dossier

            // On vérifie qu'on ne dépasse pas la taille du buffer
            if (i >= maxfiles) return SD_ERR;

            strcpy(file_names[i], file.name());
            i++;
            Serial.println(file.name()); //affichage dans le moniteur série
        }
        //prochain fichier
        file = root.openNextFile();
 
    }

  root.close();

  return i;
}



#endif