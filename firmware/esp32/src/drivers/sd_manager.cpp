/*
MISO GPIO25
SCK GPIO33
MOSI GPIO27 
CS GPIO32 
VCC 3.3V 
GND GND*/

// ── Pins SD module ─────────────────────────────────────────────────
#define SD_MISO 25
#define SD_SCK  33
#define SD_MOSI 27
#define SD_CS   32

/*SD card initialization*/
// return SD_OK if OK and SD_ERR_OPEN if error upon opening
int init_sd(){

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    //verification begin
    if (!SD.begin(SD_CS)) {
        Serial.println("Erreur : carte SD non détectée");
        return SD_ERR;
    }

    Serial.println("Carte SD détectée !");
    return SD_OK;
}


// Browse the root of the SD card
// Format : "fichier1.txt,fichier2.jpg,fichier3.mp3"
//return the number of read file and -1 if error

int get_File_List(char file_names[][64], int maxfiles) {

    File root = SD.open("/");

    //opening verification
    if (!root) {
        Serial.println("Erreur : impossible d'ouvrir la racine");
    return SD_ERR;
    }

    File file = root.openNextFile();

   

    Serial.println("Fichiers : ");

    int i=0; //counter initialization

    while (file) {
        if (!file.isDirectory()){ //ignoring subfolders

            //Check that we are below the buffer length
            if (i >= maxfiles) return SD_ERR;

            strcpy(file_names[i], file.name());
            i++;
            Serial.println(file.name());
        }
        //next file
        file = root.openNextFile();
 
    }

  root.close();

  return i;
}