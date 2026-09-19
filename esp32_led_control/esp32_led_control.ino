#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "com_RS485.h"
#include "webpage.h"
#include "sd_manager.h"
#include "screen_manager.h"
#include "joystick_manage.h"


//Pin de la LED
#define LED_PIN 2

//relais
#define RELAIS1 18
#define RELAIS2 19



//Pin du bouton
#define BUTTON1_PIN 14 // bouton du joystick
#define BUTTON2_PIN 12 //pour relais 1
#define BUTTON3_PIN 13 // pour relais 2


//tableau avec nom des fichiers
char fileList[20][64];  // Max 20 fichiers, 64 caractères par nom


int selectedIndex = 0; //index du fichier sélectionné, initialement fichier 0
int file_count; //nombre de fichier dans la carte SD
bool led_state = false;
bool relais2_state = false;
bool bouton1_precedent = HIGH;
bool bouton2_precedent = HIGH;
bool bouton3_precedent = HIGH;
bool bouton1_actuel, bouton2_actuel, bouton3_actuel;


// Configuration du point d'accès WiFi 
const char* AP_SSID     = "char_phelma";
const char* AP_PASSWORD = "evge3004";

// Serveur web sur le port 80
AsyncWebServer server(80);


void setup() {

  /*INIT port série*/
  Serial.begin(115200);
  delay(3000);

  /*INIT Bouton*/
  pinMode(BUTTON1_PIN,INPUT_PULLUP);
  pinMode(BUTTON2_PIN,INPUT_PULLUP);

 /*INIT LED*/
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); //on éteint la led par défaut

  /*INIT Relais*/
  //relais 1
  pinMode(RELAIS1, OUTPUT);
  digitalWrite(RELAIS1, LOW); //on éteint la led par défaut
  //relais 2
  pinMode(RELAIS2, OUTPUT);
  digitalWrite(RELAIS2, LOW); //on éteint la led par défaut

  /*INIT Screen*/
  if (screen_init() == SCREEN_OK) {
    screen_display_char("Initialisation...",1);
    delay(3000);
  }
  
  /*Init point d'accès wifi*/
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("AP démarré → http://192.168.4.1");
  screen_display_char("connexion : 192.168.4.1",1);
  delay(3000);

  /*INIT SD*/
  if (init_sd() == SD_OK) {
    
    file_count = get_File_List(fileList, 20); //on récupère les noms des fichiers, max 20 fichiers

    if (file_count == -1) Serial.println("Erreur SD");

    else{ 

      //affichage des noms des fichiers sur l'écran
      screen_display_menu(fileList,file_count, selectedIndex);
    }
  }

  else screen_display_char("probleme carte SD",1); // cas d'erreur SD

  /*Init com avec STM32*/
  init_com();

  // Route principale : envoie la page HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { //quand quelqu'un fait une requête GET sur / (la page d'acceuil)
    request->send_P(200, "text/html", INDEX_HTML); // on renvoie la page HTML à l'utilisateur
  });

  // Route /on : allume la LED -> on reçoit on de la page html
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest* request) { 
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(RELAIS1, HIGH);
    request->send(200, "text/plain", "ON");
  });

  // Route /off : éteint la LED
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest* request) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAIS1, LOW);
    request->send(200, "text/plain", "OFF");
  });

  server.begin();
}


void loop() {

  int direction = retour_joystick(); 
  int lastIndex = selectedIndex; // On mémorise l'ancienne position

  // Pour descendre 
  if (direction == BAS) {
      selectedIndex = (selectedIndex + 1) % file_count;
      screen_display_menu(fileList, file_count, selectedIndex);
  }

  // Pour remonter 
  if (direction == HAUT) {
      selectedIndex = (selectedIndex - 1 + file_count) % file_count; //on ajoute file_count pour ne pas avoir un modulo négatif
      screen_display_menu(fileList, file_count, selectedIndex);
  }

  // On ne rafraîchit l'écran QUE si l'index a changé
  if (selectedIndex != lastIndex) {
    screen_display_menu(fileList, file_count, selectedIndex);
    delay(200); // "Debounce" temporel : laisse le temps de relâcher le joystick
  }
  
  bouton1_actuel = digitalRead(BUTTON1_PIN);
  bouton2_actuel = digitalRead(BUTTON2_PIN);
  bouton3_actuel = digitalRead(BUTTON3_PIN);

  // bouton du joystick pour selection menu
  if (bouton1_actuel == LOW && bouton1_precedent == HIGH) {

    //on affiche le fichier sélectionné
    screen_display_char(fileList[selectedIndex], 1);
    //
    send_image(fileList[selectedIndex]);

    delay(6000); // Petit délai pour le "rebond" mécanique des contacts

    // on réactulalise le menu initialisé
    screen_display_menu(fileList, file_count, selectedIndex);

  }

  // bouton 2 pour activer relais 1
  if (bouton2_actuel == LOW && bouton2_precedent == HIGH) { // on attend que bouton2_actuel passe à LOW
    
    led_state = !led_state; // On inverse l'état
    digitalWrite(LED_PIN, led_state ? HIGH : LOW);
    digitalWrite(RELAIS1, led_state ? HIGH : LOW);

    delay(200); // Petit délai pour le "rebond" mécanique des contacts

  }

  // bouton 3 pour activer relais 2
  if (bouton3_actuel == LOW && bouton3_precedent == HIGH) { // on attend que bouton2_actuel passe à LOW
    
    relais2_state = !relais2_state; // On inverse l'état
    digitalWrite(LED_PIN, relais2_state ? HIGH : LOW);
    digitalWrite(RELAIS1, relais2_state ? HIGH : LOW);

    delay(200); // Petit délai pour le "rebond" mécanique des contacts

  }

  // On met à jour pour le prochain tour
  bouton1_precedent = bouton1_actuel; 
  bouton2_precedent = bouton2_actuel; 
  bouton3_precedent = bouton3_actuel; 
}

