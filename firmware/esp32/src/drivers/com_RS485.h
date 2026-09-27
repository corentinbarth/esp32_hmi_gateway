//include Guards
#ifndef COM_RS485_H
#define COM_RS485_H

#include <Arduino.h>
#include <stdio.h> //uses to read the fiiles
#include <stdint.h>
#include "driver/uart.h"      // UART_NUM_1, uart_param_config, uart_write_bytes...
#include "driver/gpio.h"      // gpio_set_direction, gpio_set_level...
#include "freertos/FreeRTOS.h" // pdMS_TO_TICKS
#include "freertos/task.h"


#define SOF         0xAA //start of frame
#define CMD_START   0x01 //start
#define CMD_DATA    0x02 //DATA
#define CMD_END     0x03 //end
#define CMD_ACK     0x04 //acknoledgment


#define PIN_TX      GPIO_NUM_17
#define PIN_RX      GPIO_NUM_16
#define PIN_DE_RE   GPIO_NUM_4

#define NUM_STATES  360
#define NUM_LEDS    48

#define PB -1 //problem receiving the acknoledgment
#define OK 0 //acknoldgment received, no problem

/*communication initialization*/
void init_com();

/*Reading the file and extracting num_states and num_leds, then returning the data serially (sent by Python)*/
uint8_t* read_fichier(char* file_name, uint16_t *num_states, uint8_t *num_leds);

/*CRC code to verify the integrityof the message*/
uint16_t crc16(uint8_t *data, uint16_t len);

/*UART frame transmission*/
void uart_write(uint8_t *data, uint16_t len);

/**Start frame*/
void send_start(uint8_t seq, uint8_t num_leds, uint16_t num_states);

/*Data frame*/
void send_DATA(uint8_t seq, uint8_t *data, uint8_t num_leds, uint16_t num_states );

/*End frame*/
void send_end(uint8_t seq);

/*reception of acknoledgments*/
int receive_ack(uint8_t seq);

/*The master node, which transmits the full image*/
int send_image(char* file_name);

#endif