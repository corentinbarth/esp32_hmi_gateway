#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <Adafruit_SSD1306.h>

/*
Pins de branchement
VCC 3.3V
GND GND
SCL GPIO22
SDA GPIO21*/

//paramètres de l'écran
#define Pix_w 128 //nombreDePixelsEnLargeur
#define Pix_h 64 //nombreDePixelsEnHauteur
#define broche_rst -1 //brocheResetOLED
#define adresseI2C 0x3C //Adresse I2C de l'écran Oled

// Codes de retour
#define SCREEN_OK   0   // Tout s'est bien passé
#define SCREEN_ERR  -1   // problème 


//config de l'écran
Adafruit_SSD1306 screen(Pix_w, Pix_h, &Wire, broche_rst);

int screen_init(){
    
    //init communication
    if (!screen.begin(SSD1306_SWITCHCAPVCC, adresseI2C)) {
    Serial.println("Ecran non détecté");
    return SCREEN_ERR;
    }

    Serial.println("Ecran détecté !");

    return SCREEN_OK;
}

void screen_display_char(char* text, int text_size){

    //initialisation de l'écran avant affichage
    screen.clearDisplay(); //vide la mémoire tampon

    // Multiplie la « grosseur » du texte
    screen.setTextSize(text_size);   // (nombre entier supérieur ou égal à 1, uniquement)
    
    screen.setTextColor(WHITE); //couleur de l'écran

    //initialisation des curseurs à l'écran
    int16_t positionX = 0;                     // Coordonnées X en pixels (sur le plan horizontal)
    int16_t positionY = 0;                      // Coordonnées Y en pixels (sur le plan vertical)
    screen.setCursor(positionX, positionY);  

    //affichage text
    screen.print(text);

    screen.display(); //affichage : envoie les éléments en mémoire à l'écran

}

void screen_display_tab(char fileList[][64],int file_count){

    //initialisation de l'écran avant affichage
    screen.clearDisplay(); //vide la mémoire tampon

    uint8_t niveauDeGrossissementTexte = 1;               // Multiplie la « grosseur » du texte
    screen.setTextSize(niveauDeGrossissementTexte);   // (nombre entier supérieur ou égal à 1, uniquement)
    
    screen.setTextColor(WHITE); //couleur de l'écran

    //initialisation des curseurs à l'écran
    int16_t positionX = 0;                     // Coordonnées X en pixels (sur le plan horizontal)
    int16_t positionY = 0;                      // Coordonnées Y en pixels (sur le plan vertical)
    screen.setCursor(positionX, positionY);  

    //affichage text
    screen.println("----- FICHIERS -----");

    for (int i = 0; i < file_count; i++) {
        screen.println(fileList[i]);  // Affiche chaque fichier
    }

    screen.display(); //affichage : envoie les éléments en mémoire à l'écran

}


void screen_display_menu(char fileList[][64], int file_count, int selectedIndex) {

    //initialisation de l'écran avant affichage
    screen.clearDisplay(); //vide la mémoire tampon
    
    screen.setTextSize(1);

    //initialisation des curseurs à l'écran
    int16_t positionX = 0;                     // Coordonnées X en pixels (sur le plan horizontal)
    int16_t positionY = 0;                      // Coordonnées Y en pixels (sur le plan vertical)
    screen.setCursor(positionX, positionY);  

    //affichage text
    screen.setTextColor(WHITE); 
    screen.println("----- FICHIERS -----");
    
    for (int i = 0; i < file_count; i++) {
        if (i == selectedIndex) {
            // Effet de sélection : Texte noir sur rectangle blanc
            screen.setTextColor(BLACK, WHITE); 
            screen.print("> "); //petit curseur
        } 
        
        else {
            screen.setTextColor(WHITE);
            screen.print("  "); 
        }
        
        screen.println(fileList[i]);
    }
    
    screen.display();
}



#endif