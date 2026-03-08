/*
 * eeprom.h
 *
 *  Created on: 4 mars 2026
 *      Author: Ouri
 */

#ifndef INC_EEPROM_H_
#define INC_EEPROM_H_

#include "main.h"

/* Constantes et adresses EEPROM */
// Adresse I2C sur 8 bits

#define EEPROM_ADDR 0xA0

// Cartographie mémoire
#define MEM_ADDR_VOLUME    0x00
#define MEM_ADDR_FREQ_MSB  0x01
#define MEM_ADDR_FREQ_LSB  0x02

/* Fonctions */
void EEPROM_Save_State(uint8_t vol, uint16_t freq);
void EEPROM_Load_State(uint8_t *vol, uint16_t *freq);
void EEPROM_Save_Station(uint8_t station_index, uint16_t freq);
uint16_t EEPROM_Load_Station(uint8_t station_index);
void EEPROM_Reset_Stations(void);

#endif /* INC_EEPROM_H_ */
