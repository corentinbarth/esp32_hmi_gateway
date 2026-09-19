#include "com_RS485.h"
#include <SD.h>
#include <SPI.h>

/*MAX485          ESP32
  DI    ←———   TX  (émission) GPIO17
  RO    ———→   RX  (réception) GPIO16
  DE    ←———   DE_RE (contrôle direction) GPIO4
  RE    ←———   DE_RE (contrôle direction, même broche que DE) GPIO4*/

/*
    GPIO = 1  →  DE=1, RE=1  →  émission
    GPIO = 0  →  DE=0, RE=0  →  réception*/



/*ouverture et lecture du fichier .bin*/
// .bin -> image polaire avec les octets des couleurs en série



uint8_t num_leds; // 48 leds
uint16_t num_states; // >255 états

/*initialisatoion de la communication*/
void init_com(){

    //configuration de l'UART
    uart_config_t uart_config = {
    .baud_rate  = 115200,        // vitesse de transmission
    .data_bits  = UART_DATA_8_BITS,  // 8 bits par octet
    .parity     = UART_PARITY_DISABLE, // pas de parité
    .stop_bits  = UART_STOP_BITS_1,   // 1 bit de stop
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE // pas de contrôle de flux
    };

    uart_param_config(UART_NUM_1, &uart_config);

    //donne les broches GPIO utilisé pour l'UART
    /*PIN_TX → broche d'émission
    PIN_RX → broche de réception
    UART_PIN_NO_CHANGE → on n'utilise pas RTS/CTS */

    uart_set_pin(UART_NUM_1, PIN_TX, PIN_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    //allocation des buffer interne
    /*1024 → taille du buffer de réception en octets
    0 → pas de buffer d'émission (on gère manuellement)*/
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);

    gpio_set_direction(PIN_DE_RE, GPIO_MODE_OUTPUT); // broche en sortie
    gpio_set_level(PIN_DE_RE, 0); // niveau bas = réception par défaut  

}



/*lecture fichier et extraction de num_states et num_leds, retourne les data en série (envoyé par le pyhton)*/
uint8_t* read_fichier(char* file_name, uint16_t *num_states, uint8_t *num_leds){


    // construction du chemin d'accès
    char full_name[128];
    snprintf(full_name, sizeof(full_name),"/%s", file_name);


    File f = SD.open(full_name); //ouverture du fichier en lecture binaire (read binaire)

    if (!f) {
        Serial.printf("Erreur ouverture fichier : %s\n", full_name);
        return NULL;
    }
    
    //lecture de l'en-tête
    //header = [NUM_STATES(poids fort), NUM_STATES(poids faible), NUM_LEDS]
    uint8_t header[3];

    //fread(adresse_destination, taille_element, nombre_elements, fichier);
    //on utilise fread pour lire des octets bruts

    if(f.read(header,3)!=3){
        Serial.println("erreur lecture en tête");
        f.close();
        return NULL;
    }

    *num_states = (header[0]<<8) | header[1];
    *num_leds = header[2];

    uint32_t data_size = (uint32_t)(*num_states)*(*num_leds)*3;
    uint8_t* data = (uint8_t*) calloc(data_size, sizeof(uint8_t));

    
    uint32_t bytes_lus = f.read(data, data_size);

    if (bytes_lus != data_size){
        Serial.printf("lecture icomplète");
        free(data);
        f.close();
        return NULL;
    }

    f.close(); //extraction des données terminée

    return data;
}

/*code CRC pour vérifier que le message n'est pas corrompu*/
//conventionné
uint16_t crc16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;  // valeur initiale

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


/*envoie de la trame en UART*/
void uart_write(uint8_t *data, uint16_t len)
{
    gpio_set_level(PIN_DE_RE, 1); // passage en émission

    uart_write_bytes(UART_NUM_1, (const char*)data, len);
    uart_wait_tx_done(UART_NUM_1, 100); // attendre fin d'émission

    gpio_set_level(PIN_DE_RE, 0); // retour en réception pour recevoir les acquitements
}


/*découpage de la trame*/
/* variable
SOF         0xAA //start of frame
CMD_START   0x01 //début d'image
CMD_DATA    0x02 //DATA
CMD_END     0x03 //fin d'image
CMD_ACK     0x04 //acquitement

NUM_LEDS    48 //nb de leds
NUM_STATES  360 //résolution choisie */


/*trame de start*/
void send_start(uint8_t seq, uint8_t num_leds, uint16_t num_states){

    //DATA = Nombre d'états + Nombre de leds
    uint8_t payload[3]; //tableau de 3 entiers
    //Nombre d'états sur 2 octets car >255
    //on découpe poids fort/faible pour éviter d'avoir des pb avec little/big endian
    payload[0] = (num_states >> 8) & 0xFF;  // octet de poids fort
    payload[1] = num_states & 0xFF; // octet de poids faible
    payload[2] = num_leds;

    uint8_t trame[10]; //tableau de 10 entiers
    trame[0] = SOF;
    trame[1] = CMD_START;
    trame[2] = seq;
    trame[3] = 0x00;  // LEN poids fort
    trame[4] = 0x03;  // LEN poids faible (3 octets)
    trame[5] = payload[0];
    trame[6] = payload[1];
    trame[7] = payload[2];

    uint16_t crc = crc16(&trame[1], 7);     // CRC sur CMD+SEQ+LEN+PAYLOAD
    trame[8]  = (crc >> 8) & 0xFF; //octet de poids fort
    trame[9]  = crc & 0xFF; // octet de poids faible

    uart_write(trame, 10);
}

/*trame de DATA*/
void send_DATA(uint8_t seq, uint8_t *data, uint8_t num_leds, uint16_t num_states )
{
    //taille de Payload
    //DATA = 8 lignes de données et chaque ligne c'est num_leds pixels de 3 octets chacun
    uint16_t len = 8*num_leds*3;

    uint8_t *trame = (uint8_t*) calloc(7+len,sizeof(uint8_t)); //tableau 7+len entiers

    trame[0] = SOF;
    trame[1] = CMD_DATA;
    trame[2] = seq;
    trame[3] = (len>>8) & 0xFF;  // LEN poids fort
    trame[4] = len & 0xFF; // LEN poids faible

    //Payload = DATA
    for (int i=0;i<len;i++){ //4 octets pour un int

        //chaque octet représente une couleur : data = [R,G,B,R,G,B...]
        trame[5+i]= data[i];

    }

    uint16_t crc = crc16(&trame[1],4+len);     // CRC sur CMD+SEQ+LEN+PAYLOAD
    trame[5+len]  = (crc >> 8) & 0xFF; //octet de poids fort
    trame[5+len+1]  = crc & 0xFF; // octet de poids faible

    uart_write(trame, 7+len);
    free(trame);
}


/*trame de fin*/
void send_end(uint8_t seq)
{
    //DATA = strictement rien

    uint8_t trame[7]; //tableau de 10 entiers
    trame[0] = SOF;
    trame[1] = CMD_END;
    trame[2] = seq;
    trame[3] = 0x00;  // LEN poids fort
    trame[4] = 0x00;  // LEN poids faible (3 octets)

    uint16_t crc = crc16(&trame[1], 4);     // CRC sur CMD+SEQ+LEN
    trame[5]  = (crc >> 8) & 0xFF; //octet de poids fort
    trame[6]  = crc & 0xFF; // octet de poids faible

    uart_write(trame, 7);
}

/*réception des acquitements*/
int receive_ack(uint8_t seq){

    uint8_t buffer[7];

    //uart_read_bytes(UART_NUM_1, buffer, longueur, timeout);
    //renvoi le nombre d'octets lus
    int recu = uart_read_bytes(UART_NUM_1, buffer, 7, pdMS_TO_TICKS(100));

    // on doit recevoir 7 octets pour une trame ACK
    if (recu != 7)
    {
        // timeout, ACK non reçu
        return PB; //problème
    }

    
    if((buffer[0]!=0XAA) || (buffer[1]!=0X04) || (buffer[2]!=seq) || (buffer[3]!=0X00) || (buffer[4]!=0X00)){
        return PB;
    }

    uint16_t crc = crc16(&buffer[1], 4); //4octets : 1 CMD, 1 seq, 2 LEN
    uint8_t poids_fort_crc = (crc>>8) & 0xFF;
    uint8_t poids_faible_crc = crc & 0XFF;

    if((poids_fort_crc != buffer[5]) || (poids_faible_crc != buffer[6])){//problème avec le code CRC = trame corrompue

        return PB;
    }

    return OK;
}

/*le chef d'orchestre = celui qui envoie l'image complète*/
int send_image(char* file_name){
    
    uint8_t seq=0;
    
    // diagnostic mémoire
    Serial.printf("Heap avant alloc : %d\n", ESP.getFreeHeap());

    //lecture du fichier, récupération num_states et num_leds et retourne les data
    uint8_t* data = read_fichier(file_name, &num_states, &num_leds);

    // diagnostic après lecture
    Serial.printf("Heap après alloc : %d\n", ESP.getFreeHeap());
    Serial.printf("num_states=%d, num_leds=%d\n", num_states, num_leds);

    if (data==NULL){//verif allocation dynamique ok
        Serial.println("Echec allocation memoire !");
        return PB;
    }

    uint32_t data_size = num_states * num_leds * 3;
    uint8_t *data_end = data + data_size; //pointeur de fin


    //envoi trame start + acquitement
    int tentatives = 0;
    int ack = PB;
    while(tentatives < 3){
        send_start(seq, num_leds, num_states);//envoi trame de start

        ack = receive_ack(seq);
        if(ack == OK){tentatives = 5;} //pour sortir de la boucle (on aurait pu mettre un break)

        tentatives++;
    }

    if(ack == PB){
        return PB;
    }

    seq+=1;

    //on actualise le pointeur data et on envoie la trame au STM32
    uint8_t *curr = data;

    while(curr < data_end){
        
        vTaskDelay(1); // reset watchdog pour éviter reboot

        //envoi data + acquitement
        tentatives = 0;
        while(tentatives < 3){

            send_DATA(seq, curr, num_leds, num_states); //envoi de la trame de donnée

            ack = receive_ack(seq);
            if(ack == OK) break; //pour sortir de la boucle (on aurait pu mettre un break)
            
            tentatives++;
        }

        if(ack == PB){
            free(data); // libération mémoire avant de quitter
            return PB;
        }

        curr += (8*num_leds*3);
        seq+=1;

        }


    //envoi trame end + acquitement
    tentatives = 0;
    while(tentatives < 3){
        send_end(seq);//envoi trame de fin

        ack = receive_ack(seq);
        if(ack == OK){tentatives = 5;} //pour sortir de la boucle (on aurait pu mettre un break)

        tentatives++;
    }

    if(ack == PB){
        free(data); // libération mémoire avant de quitter
        return PB;
    }
        
    free(data); //alloué dynamiquement par read_file
    
    return OK;
}