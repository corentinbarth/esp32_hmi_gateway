/*
Pins :
VCC 3.3V
GND GND
SCL GPIO22
SDA GPIO21*/

//screen settings
#define Pix_w 128 //number of pixels in width
#define Pix_h 64 //number of pixels in height
#define broche_rst -1 //reset Oled
#define adresseI2C 0x3C //I2C screen adress


//config de l'écran
Adafruit_SSD1306 screen(Pix_w, Pix_h, &Wire, broche_rst);

/*screen initialisation*/
int screen_init(){
    
    //begining of the comunication
    if (!screen.begin(SSD1306_SWITCHCAPVCC, adresseI2C)) {
    Serial.println("Ecran non détecté");
    return SCREEN_ERR;
    }

    Serial.println("Ecran détecté !");

    return SCREEN_OK;
}

/*Display string from string*/
void screen_display_char(char* text, int text_size){

    //screen initialization before display
    screen.clearDisplay(); //clear the buffer

    // Multipliying text size
    screen.setTextSize(text_size);   // text_size : int >= 1
    
    screen.setTextColor(WHITE); //screen color

    //cursor initialization
    int16_t positionX = 0;                     // x-coordinate on the horizontal plane
    int16_t positionY = 0;                     // y-coordinate on the vertical plane
    screen.setCursor(positionX, positionY);  

    //displaying text
    screen.print(text);

    screen.display();

}

/*Display menu from array of filenames*/
void screen_display_menu(char fileList[][64], int file_count, int selectedIndex) {

    //screen initialization before display
    screen.clearDisplay(); //clear the buffer
    
    screen.setTextSize(1);

    //cursor initialization
    int16_t positionX = 0;                     // x-coordinate on the horizontal plane
    int16_t positionY = 0;                     // y-coordinate on the horizontal plane
    screen.setCursor(positionX, positionY);  

    //displaying title
    screen.setTextColor(WHITE); 
    screen.println("----- FICHIERS -----");
    
    //displaying menu
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