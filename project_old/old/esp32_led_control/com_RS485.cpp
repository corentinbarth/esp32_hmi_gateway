#include "com_RS485.h"
#include <SD.h>
#include <SPI.h>

/*MAX485          ESP32
  DI    ←———   TX  (data transmission) GPIO17
  RO    ———→   RX  (data emission) GPIO16
  DE    ←———   DE_RE (data direction control) GPIO4
  RE    ←———   DE_RE (data direction control, same pin as DE) GPIO4*/

/*
    GPIO = 1  →  DE=1, RE=1  → data transmission
    GPIO = 0  →  DE=0, RE=0  →  data reception*/




/*opening and reading the file .bin*/
// .bin -> serialized polar image from processing



uint8_t num_leds; // 48 LED
uint16_t num_states; // 255 states


/*communication initialization*/
void init_com(){

    //UART configuration
    uart_config_t uart_config = {
    .baud_rate  = 115200,        // transmission speed
    .data_bits  = UART_DATA_8_BITS,  // 8 bits per byte
    .parity     = UART_PARITY_DISABLE, // no parity
    .stop_bits  = UART_STOP_BITS_1,   // 1 stop bit
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE // no flow control
    };

    uart_param_config(UART_NUM_1, &uart_config);

    //GPIO pins use for UART
    /*PIN_TX → transmission pin
    PIN_RX → reception pin
    UART_PIN_NO_CHANGE → RTS/CTS not used*/

    uart_set_pin(UART_NUM_1, PIN_TX, PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    //intern buffer allocation
    /*1024 → reception buffer lenghts (in bytes)
    0 → no transmission buffer (we handle it manually*/
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);

    gpio_set_direction(PIN_DE_RE, GPIO_MODE_OUTPUT); // output GPIO
    gpio_set_level(PIN_DE_RE, 0); //LOW = default receive mode

}



/*Reading the file and extracting num_states and num_leds, then returning the data serially (sent by Python)*/
uint8_t* read_fichier(char* file_name, uint16_t *num_states, uint8_t *num_leds){


    // Path construction
    char full_name[128];
    snprintf(full_name, sizeof(full_name),"/%s", file_name);


    File f = SD.open(full_name); //opening in binary read mode

    if (!f) {
        Serial.printf("fail to open the file : %s\n", full_name);
        return NULL;
    }
    
    //header reading
    //header = [NUM_STATES(most significant bit)), NUM_STATES(least significant bit), NUM_LEDS]
    uint8_t header[3];

    //syntax : fread(transmission adress, element length, number of elements, file);
    //using fread to read raw bytes

    if(f.read(header,3)!=3){
        Serial.println("fail to read headers");
        f.close();
        return NULL;
    }

    *num_states = (header[0]<<8) | header[1];
    *num_leds = header[2];

    uint32_t data_size = (uint32_t)(*num_states)*(*num_leds)*3;
    uint8_t* data = (uint8_t*) calloc(data_size, sizeof(uint8_t));

    
    uint32_t bytes_lus = f.read(data, data_size);

    if (bytes_lus != data_size){
        Serial.printf("incomplete reading");
        free(data);
        f.close();
        return NULL;
    }

    f.close(); //end of data extraction

    return data;
}


/*CRC code to verify the integrityof the message*/

uint16_t crc16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;  // initial value

    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}


/*UART frame transmission*/
void uart_write(uint8_t *data, uint16_t len)
{
    gpio_set_level(PIN_DE_RE, 1); // transmission mode

    uart_write_bytes(UART_NUM_1, (const char*)data, len);
    uart_wait_tx_done(UART_NUM_1, 100); // waiting end of emission

    gpio_set_level(PIN_DE_RE, 0); // back to transmission mode for reception of acknoledgment
}


/*Frame parsing*/
/* 
SOF         0xAA //start of frame
CMD_START   0x01 //start
CMD_DATA    0x02 //DATA
CMD_END     0x03 //end
CMD_ACK     0x04 //acknoledgment

NUM_LEDS    48 //LED number
NUM_STATES  360 //the selected resolution */


/*Start frame*/
void send_start(uint8_t seq, uint8_t num_leds, uint16_t num_states){

    //DATA = state number + LED number
    uint8_t payload[3]; //Array of 3 integers
    //2 bytes because > 255
    //MSB/LSB = no problem with little/big endian 
    payload[0] = (num_states >> 8) & 0xFF;  // MSB
    payload[1] = num_states & 0xFF; // LSB
    payload[2] = num_leds;

    uint8_t trame[10]; 
    trame[0] = SOF;
    trame[1] = CMD_START;
    trame[2] = seq;
    trame[3] = 0x00;  // LEN LSB
    trame[4] = 0x03;  // LEN MSB (3 Bytes)
    trame[5] = payload[0];
    trame[6] = payload[1];
    trame[7] = payload[2];

    uint16_t crc = crc16(&trame[1], 7);     // CRC on CMD+SEQ+LEN+PAYLOAD
    trame[8]  = (crc >> 8) & 0xFF; //MSB
    trame[9]  = crc & 0xFF; // LSB

    uart_write(trame, 10);
}

/*DATA frame*/
void send_DATA(uint8_t seq, uint8_t *data, uint8_t num_leds, uint16_t num_states )
{
    //length payload
    //DATA = 8 data lines, each line with num_leds pixels of 3 bytes each
    uint16_t len = 8*num_leds*3;

    uint8_t *trame = (uint8_t*) calloc(7+len,sizeof(uint8_t)); //Array 7+len integers

    trame[0] = SOF;
    trame[1] = CMD_DATA;
    trame[2] = seq;
    trame[3] = (len>>8) & 0xFF;  // LEN MSB
    trame[4] = len & 0xFF; // LEN LSB

    //Payload = DATA
    for (int i=0;i<len;i++){ //4 bytes for an integer

        //Each bytes = a colors : data = [R,G,B,R,G,B...]
        trame[5+i]= data[i];

    }

    uint16_t crc = crc16(&trame[1],4+len);     // CRC on CMD+SEQ+LEN+PAYLOAD
    trame[5+len]  = (crc >> 8) & 0xFF; //MSB
    trame[5+len+1]  = crc & 0xFF; // LSB

    uart_write(trame, 7+len);
    free(trame);
}


/*trame de fin*/
void send_end(uint8_t seq)
{

    uint8_t trame[7];
    trame[0] = SOF;
    trame[1] = CMD_END;
    trame[2] = seq;
    trame[3] = 0x00;  // LEN MSB
    trame[4] = 0x00;  // LEN LSB

    uint16_t crc = crc16(&trame[1], 4);     // CRC on CMD+SEQ+LEN
    trame[5]  = (crc >> 8) & 0xFF; //MSB
    trame[6]  = crc & 0xFF; // LSB

    uart_write(trame, 7);
}

/*reception of acknoledgments*/
int receive_ack(uint8_t seq){

    uint8_t buffer[7];

    //uart_read_bytes(UART_NUM_1, buffer, length, timeout);
    //returns the number of bytes read
    int recu = uart_read_bytes(UART_NUM_1, buffer, 7, pdMS_TO_TICKS(100));

    // we must receive 7 bytes
    if (recu != 7)
    {
        // timeout, no ACK
        return PB; //probleme
    }

    
    if((buffer[0]!=0XAA) || (buffer[1]!=0X04) || (buffer[2]!=seq) || (buffer[3]!=0X00) || (buffer[4]!=0X00)){
        return PB;
    }

    uint16_t crc = crc16(&buffer[1], 4); //4 bytes : 1 CMD, 1 seq, 2 LEN
    uint8_t poids_fort_crc = (crc>>8) & 0xFF;
    uint8_t poids_faible_crc = crc & 0XFF;

    if((poids_fort_crc != buffer[5]) || (poids_faible_crc != buffer[6])){//problem with CRC code = corrupted frame

        return PB;
    }

    return OK;
}

/*The master node, which transmits the full image*/
int send_image(char* file_name){
    
    uint8_t seq=0;
    
    // memory diagnostic 
    Serial.printf("Heap avant alloc : %d\n", ESP.getFreeHeap());

    //reading the file, extracting num_states and num_leds and returns datas
    uint8_t* data = read_fichier(file_name, &num_states, &num_leds);

    // diagnostic after reading
    Serial.printf("Heap après alloc : %d\n", ESP.getFreeHeap());
    Serial.printf("num_states=%d, num_leds=%d\n", num_states, num_leds);

    if (data==NULL){
        Serial.println("memory allocation failure !");
        return PB;
    }

    uint32_t data_size = num_states * num_leds * 3;
    uint8_t *data_end = data + data_size; //pointeur de fin


    //sending start frame + acknoledgments
    int tentatives = 0;
    int ack = PB;
    while(tentatives < 3){
        send_start(seq, num_leds, num_states);//sending start frame

        ack = receive_ack(seq);
        if(ack == OK){tentatives = 5;} //break might be usefull

        tentatives++;
    }

    if(ack == PB){
        return PB;
    }

    seq+=1;

    //The data pointer is updated, and the frame is sent to the STM32
    uint8_t *curr = data;

    while(curr < data_end){
        
        vTaskDelay(1); // reseting watchdog to avoid reboot

        //send data + ACK
        tentatives = 0;
        while(tentatives < 3){

            send_DATA(seq, curr, num_leds, num_states); //send data frame

            ack = receive_ack(seq);
            if(ack == OK) break; //break might be usefull
            
            tentatives++;
        }

        if(ack == PB){
            free(data);
            return PB;
        }

        curr += (8*num_leds*3);
        seq+=1;

        }


    //sending end frame + ACK
    tentatives = 0;
    while(tentatives < 3){
        send_end(seq);//sending end frame

        ack = receive_ack(seq);
        if(ack == OK){tentatives = 5;}

        tentatives++;
    }

    if(ack == PB){
        free(data);
        return PB;
    }
        
    free(data); //?allocated by read_file
    
    return OK;
}