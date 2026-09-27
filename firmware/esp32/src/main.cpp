#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "com_RS485.h"
#include "webpage.h"
#include "sd_manager.h"
#include "screen_manager.h"
#include "joystick_manage.h"

//Constant 
#define NUMBER_MAX_FILES 20 //max 20 files
#define LENGTH_MAX_STRING 64 //64 characters for each name

//LED Pin
#define LED_PIN 2

//relais
#define RELAY1 18
#define RELAY2 19



//button pin
#define BUTTON1_PIN 14 // jostick button
#define BUTTON2_PIN 12 //for relay 1
#define BUTTON3_PIN 13 //for Relay 2


//Array with filenames
char fileList[NUMBER_MAX_FILES][LENGTH_MAX_STRING];


int selectedIndex = 0; //index of the selected file, 0 initially
int file_count; //number of file on SD card
bool led_state = false;
bool relay2_state = false;
//initial buttons states
bool button1_previous = HIGH;
bool button2_previous = HIGH;
bool button3_previous = HIGH;
bool button1_current, button2_current, button3_current;


//Wifi access point configuration
const char* AP_SSID     = "char_phelma";
const char* AP_PASSWORD = "evge3004";

//web server on port 80
AsyncWebServer server(80);


void setup() {

  Serial.begin(115200);
  delay(3000);

  /*Init button*/
  pinMode(BUTTON1_PIN,INPUT_PULLUP); //button 1with pull up resistance
  pinMode(BUTTON2_PIN,INPUT_PULLUP); //button 2 with pull up resistance

 /*Init LED*/
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); //default value : LOW

  /*Init relay*/
  //relay 1
  pinMode(RELAY1, OUTPUT);
  digitalWrite(RELAY1, LOW); //default value : LOW
  //relay 2
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY2, LOW); //default value : LOW

  /*INIT Screen*/
  if (screen_init() == SCREEN_OK) {
    screen_display_char("Initialisation...",1);
    delay(3000);
  }
  
  /*Wifi access point init*/
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("AP started → http://192.168.4.1");
  screen_display_char("connexion @IP : 192.168.4.1",1);
  delay(3000);

  /*Init  SD*/
  if (init_sd() == SD_OK) {
    
    file_count = get_File_List(fileList, NUMBER_MAX_FILES);

    if (file_count == -1) Serial.println("Error with SD card");

    else{ 

      //display names of the files
      screen_display_menu(fileList,file_count, selectedIndex);
    }
  }

  else screen_display_char("Problem with SD card",1); //erros SD

  /*Init com with STM32*/
  init_com();

  // Main road : sending HTML web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { //Get request on / (home page)
    request->send_P(200, "text/html", INDEX_HTML); //send the page to the user
  });

  // Road /on : turn the pin on
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest* request) { 
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(RELAY1, HIGH);
    request->send(200, "text/plain", "ON");
  });

  // Road /off : turn the pin off
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest* request) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAY1, LOW);
    request->send(200, "text/plain", "OFF");
  });

  server.begin();
}


void loop() {

  int direction = retour_joystick(); 
  int lastIndex = selectedIndex; // On mémorise l'ancienne position

  /*joystick manager*/
  //to go down
  if (direction == DOWN) {
      selectedIndex = (selectedIndex + 1) % file_count;
      screen_display_menu(fileList, file_count, selectedIndex);
  }

  //to go up 
  if (direction == UP) {
      selectedIndex = (selectedIndex - 1 + file_count) % file_count; //on ajoute file_count pour ne pas avoir un modulo négatif
      screen_display_menu(fileList, file_count, selectedIndex);
  }

  /*screen manager*/
  // the screen is only refresh if the index has changed
  if (selectedIndex != lastIndex) {
    screen_display_menu(fileList, file_count, selectedIndex);
    delay(200); // "Debounce" temporel : laisse le temps de relâcher le joystick
  }
  
  button1_current = digitalRead(BUTTON1_PIN);
  button2_current = digitalRead(BUTTON2_PIN);
  button3_current = digitalRead(BUTTON3_PIN);

  //joystick button to select file (button 1)
  if (button1_current == LOW && button1_previous == HIGH) {

    //display the selected file
    screen_display_char(fileList[selectedIndex], 1);
    //
    send_image(fileList[selectedIndex]);

    delay(6000); // display time

    // back to display menu
    screen_display_menu(fileList, file_count, selectedIndex);

  }

  // button 2 to turn on relais 1
  if (button2_current == LOW && button2_previous == HIGH) { 
    
    led_state = !led_state; 
    digitalWrite(LED_PIN, led_state ? HIGH : LOW);
    digitalWrite(RELAY1, led_state ? HIGH : LOW);

    delay(200); // avoid mechanical bounce

  }

  // button 3 to turn on relais 2
  if (button3_current == LOW && button3_previous == HIGH) { 
    
    relay2_state = !relay2_state; // On inverse l'état
    digitalWrite(LED_PIN, relay2_state ? HIGH : LOW);
    digitalWrite(RELAY1, relay2_state ? HIGH : LOW);

    delay(200); // avoid mechanical bounce

  }

  // update for the next loop
  button1_previous = button1_current; 
  button2_previous = button2_current; 
  button3_previous = button3_current; 
}

