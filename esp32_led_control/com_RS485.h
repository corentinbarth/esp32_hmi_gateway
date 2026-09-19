//include Guards
#ifndef COM_RS485_H
#define COM_RS485_H

#include <Arduino.h>
#include <stdio.h> //pour lecture de fichier
#include <stdint.h>
#include "driver/uart.h"      // UART_NUM_1, uart_param_config, uart_write_bytes...
#include "driver/gpio.h"      // gpio_set_direction, gpio_set_level...
#include "freertos/FreeRTOS.h" // pdMS_TO_TICKS
#include "freertos/task.h"

#define SOF         0xAA //start of frame
#define CMD_START   0x01 //début d'image
#define CMD_DATA    0x02 //DATA
#define CMD_END     0x03 //fin d'image
#define CMD_ACK     0x04 //acquitement

#define PIN_TX      GPIO_NUM_17
#define PIN_RX      GPIO_NUM_16
#define PIN_DE_RE   GPIO_NUM_4

#define NUM_STATES  360
#define NUM_LEDS    48

#define PB -1 //problème à la reception de l'acquitement
#define OK 0 //acquitement reçu sans problème

//#define NUM_LEDS    48
//#define NUM_STATES  360 //résolution choisie
//on les utilises avec des pointeurs plutôt que des variables globales

/*initialisatoion de la communication*/
void init_com();


/*lecture fichier et extraction de num_states et num_leds, retourne les data en série (envoyé par le pyhton)*/
uint8_t* read_fichier(char* file_name, uint16_t *num_states, uint8_t *num_leds);

/*code CRC pour vérifier que le message n'est pas corrompu*/
uint16_t crc16(uint8_t *data, uint16_t len);

/*envoie de la trame en UART*/
void uart_write(uint8_t *data, uint16_t len);

/*trame de start*/
void send_start(uint8_t seq, uint8_t num_leds, uint16_t num_states);

/*trame de DATA*/
void send_DATA(uint8_t seq, uint8_t *data, uint8_t num_leds, uint16_t num_states );

/*trame de fin*/
void send_end(uint8_t seq);

/*réception des acquitements*/
int receive_ack(uint8_t seq);

/*le chef d'orchestre = celui qui envoie l'image complète*/
int send_image(char* file_name);

#endif