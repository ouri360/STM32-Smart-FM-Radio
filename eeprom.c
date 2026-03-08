/*
 * eeprom.c
 *
 *  Created on: 4 mars 2026
 *      Author: Ouri
 */

#include "eeprom.h"

// Importation du handle I2C
extern I2C_HandleTypeDef hi2c1;

/* Fonctions */


/**
  * @brief  Sauvegarde le volume et la fréquence dans l'EEPROM
  * @param  vol: Volume actuel (0-15)
  * @param  freq: Fréquence actuelle en MHz * 100 (ex: 10550)
  */
void EEPROM_Save_State(uint8_t vol, uint16_t freq)
{
    uint8_t data_vol = vol;
    uint8_t freq_msb = (freq >> 8) & 0xFF; // Extraction des 8 bits de poids fort
    uint8_t freq_lsb = freq & 0xFF;        // Extraction des 8 bits de poids faible

    // Écriture du volume
    HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, MEM_ADDR_VOLUME, I2C_MEMADD_SIZE_16BIT, &data_vol, 1, 100);
    HAL_Delay(5); // Délai de la puce EEPROM (Page Write Time)

    // Écriture de la fréquence
    HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, MEM_ADDR_FREQ_MSB, I2C_MEMADD_SIZE_16BIT, &freq_msb, 1, 100);
    HAL_Delay(5);

    HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, MEM_ADDR_FREQ_LSB, I2C_MEMADD_SIZE_16BIT, &freq_lsb, 1, 100);
    HAL_Delay(5);
}


/**
  * @brief  Charge le volume et la fréquence depuis l'EEPROM
  * @param  vol: Pointeur vers la variable de volume
  * @param  freq: Pointeur vers la variable de fréquence
  */
void EEPROM_Load_State(uint8_t *vol, uint16_t *freq)
{
    uint8_t data_vol = 0;
    uint8_t freq_msb = 0;
    uint8_t freq_lsb = 0;

    // Lecture séquentielle des adresses
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, MEM_ADDR_VOLUME, I2C_MEMADD_SIZE_16BIT, &data_vol, 1, 100);
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, MEM_ADDR_FREQ_MSB, I2C_MEMADD_SIZE_16BIT, &freq_msb, 1, 100);
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, MEM_ADDR_FREQ_LSB, I2C_MEMADD_SIZE_16BIT, &freq_lsb, 1, 100);

    // Vérification : si la mémoire est vierge (0xFF) on applique les valeurs par défaut
    if (data_vol == 0xFF || freq_msb == 0xFF)
    {
        *vol = 15;      // Volume par défaut
        *freq = 10550;  // 105.5 MHz par défaut
    }
    else
    {
        *vol = data_vol;
        *freq = (freq_msb << 8) | freq_lsb; // Reconstitution du mot de 16 bits
    }
}


/**
  * @brief  Sauvegarde une fréquence dans un "slot" (Station 1, 2, 3...)
  * @param  station_index: 0 pour Station 1, 1 pour Station 2, etc.
  * @param  freq: Fréquence à sauvegarder
  */
void EEPROM_Save_Station(uint8_t station_index, uint16_t freq)
{
    // Calcul de l'adresse de base pour cet index
    uint8_t addr_msb = 0x10 + (station_index * 2);
    uint8_t addr_lsb = addr_msb + 1;

    uint8_t freq_msb = (freq >> 8) & 0xFF;
    uint8_t freq_lsb = freq & 0xFF;

    HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, addr_msb, I2C_MEMADD_SIZE_16BIT, &freq_msb, 1, 100);
    HAL_Delay(5);
    HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, addr_lsb, I2C_MEMADD_SIZE_16BIT, &freq_lsb, 1, 100);
    HAL_Delay(5);
}


/**
  * @brief  Charge une fréquence depuis un "slot" de station
  * @param  station_index: 0 pour Station 1, 1 pour Station 2, etc.
  * @retval Fréquence lue, ou 0 si l'emplacement est vide (0xFF)
  */
uint16_t EEPROM_Load_Station(uint8_t station_index)
{
    uint8_t addr_msb = 0x10 + (station_index * 2);
    uint8_t addr_lsb = addr_msb + 1;

    uint8_t freq_msb = 0;
    uint8_t freq_lsb = 0;

    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, addr_msb, I2C_MEMADD_SIZE_16BIT, &freq_msb, 1, 100);
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, addr_lsb, I2C_MEMADD_SIZE_16BIT, &freq_lsb, 1, 100);

    // Si la case mémoire est vierge, on retourne 0
    if (freq_msb == 0xFF && freq_lsb == 0xFF)
    {
        return 0;
    }
    return (freq_msb << 8) | freq_lsb;
}

/**
  * @brief  Efface toutes les stations sauvegardées (remet à 0)
  */
void EEPROM_Reset_Stations(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        // En écrivant 0, la fonction d'affichage de la page 2 affichera "- Vide -"
        EEPROM_Save_Station(i, 0);
    }
}
