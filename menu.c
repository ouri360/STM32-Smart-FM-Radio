/*
 * menu.c
 *
 *  Created on: Jan 21, 2026
 *      Author: tromero
 */

#include "menu.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "ssd1306_symbols.h"
#include "RDA_5807.h"
#include "eeprom.h"
#include "stdio.h"

uint8_t indice_menu_1 = 0;

uint16_t adc_vide = 3120;   	// Correspond à 3.2V
uint16_t adc_pleine = 4095; 	// Correspond à 4.2V

// la variable hi2c1 est définie en extern dans i2c.h
extern I2C_HandleTypeDef hi2c1;

void menu(uint8_t page)
{
	// POUR FAIRE CLIGNOTER
	  //ssd1306_InvertRectangle(52, 7, 79, 39);
	  //ssd1306_UpdateScreen();
	  //HAL_Delay(500);


	switch(page) {

	case 0:

		  //Afficher page 0
		  ssd1306_Fill(0);

		  //ICONE PILE
		  //ssd1306_FillRectangle(1, 0, 4, 2, 1);
		  //ssd1306_DrawRectangle(0, 3, 5, 9, 1);

		  //BARRE PILE
		  ssd1306_DrawRectangle(0, 15, 5, 48, 1);
		  ssd1306_FillRectangle(0, hauteur_remplissage, 5, 48, 1);		//SE MODIFIE AVEC CODE SELON LA BATTERIE RESTANTE :
		  	  	  	  	  	  	  	  	  	  	  	  					//y1 = 15 --> BATTERIE PLEINE, y1 = 48 --> BATTERIE VIDE
		  //ICONE PILE (Avec FONT modifiée)
		  ssd1306_SetCursor(0, 1);
		  if (raw_battery_adc - adc_vide >= 900)		// >= 90% BATTERIE
		  {
			  ssd1306_WriteString("e", Symbols_7x10, 1);
		  }
		  else if (raw_battery_adc - adc_vide >= 750)	// >= 75% BATTERIE
		  {
			  ssd1306_WriteString("d", Symbols_7x10, 1);
		  }
		  else if (raw_battery_adc - adc_vide >= 500) 	// >= 50% BATTERIE
		  {
			  ssd1306_WriteString("c", Symbols_7x10, 1);
		  }
		  else if (raw_battery_adc - adc_vide >= 250) 	// >= 25% BATTERIE
		  {
			  ssd1306_WriteString("b", Symbols_7x10, 1);
		  }
		  else 											// < 25% BATTERIE
		  {
			  ssd1306_WriteString("a", Symbols_7x10, 1);
		  }

		  //ICONE RESEAU
		  if (RSSI >= 45)
		  {
			  ssd1306_FillRectangle(116, 6, 119, 9, 1);
			  ssd1306_FillRectangle(120, 3, 123, 9, 1);
			  ssd1306_FillRectangle(124, 0, 127, 9, 1);
		  }
		  else if (RSSI >= 35)
		  {
			  ssd1306_FillRectangle(116, 6, 119, 9, 1);
			  ssd1306_FillRectangle(120, 3, 123, 9, 1);
			  ssd1306_DrawRectangle(124, 0, 127, 9, 1);
		  }
		  else if (RSSI >= 15)
		  {
			  ssd1306_FillRectangle(116, 6, 119, 9, 1);
			  ssd1306_DrawRectangle(120, 3, 123, 9, 1);
			  ssd1306_DrawRectangle(124, 0, 127, 9, 1);
		  }
		  else
		  {
			  ssd1306_DrawRectangle(116, 6, 119, 9, 1);
			  ssd1306_DrawRectangle(120, 3, 123, 9, 1);
			  ssd1306_DrawRectangle(124, 0, 127, 9, 1);
		  }

		  //MOT : STATION (OU NOM RDS)
		  ssd1306_SetCursor(40, 8);
		  ssd1306_WriteString(rda_station_name, Font_6x8, 1);

		  //MOT : FREQUENCE ACTUELLE
		  ssd1306_SetCursor(40, 18);
		  ssd1306_WriteString(freq_str, Font_11x18, 0);

		  //MOT : MHz
		  ssd1306_SetCursor(58, 38);
		  ssd1306_WriteString("MHz", Font_6x8, 1);

		  //LIGNE DU MENU EN BAS
		  ssd1306_Line(0, 54, 127, 54, 1);
		  ssd1306_Line(42, 54, 42, 64, 1);
		  ssd1306_Line(84, 54, 84, 64, 1);

		  //MOT : PRECEDENT
		  ssd1306_SetCursor(8, 56);
		  ssd1306_WriteString("Prec.", Font_6x8, 1);

		  //MOT : MENU
		  ssd1306_SetCursor(52, 56);
		  ssd1306_WriteString("Menu", Font_6x8, 1);

		  //MOT : SUIVANT
		  ssd1306_SetCursor(94, 56);
		  ssd1306_WriteString("Suiv.", Font_6x8, 1);

		  ssd1306_UpdateScreen();
		break;

	case 1:

		//Afficher page 1
		ssd1306_Fill(0);

		//Case à coché bassboost
		ssd1306_DrawRectangle(0, 0, 10, 10, 1);
		if (bass_boost == 1)
		{
			ssd1306_FillRectangle(2, 2, 8, 8, 1);
		}
		else
		{
			ssd1306_FillRectangle(2, 2, 8, 8, 0);
		}
		ssd1306_SetCursor(12, 2);
		ssd1306_WriteString("Bass Boost", Font_6x8, 1);

		//Recherche station
		ssd1306_SetCursor(0, 13);
		ssd1306_WriteString("Recherche Station", Font_6x8, 1);

		//Volume
		ssd1306_SetCursor(0, 23);
		ssd1306_WriteString("Volume", Font_6x8, 1);

		//Retour
		ssd1306_SetCursor(45, 23);
		ssd1306_WriteString("Retour", Font_6x8, 1);


		// DESSIN DU HAUT-PARLEUR
		// Base rectangulaire du haut-parleur
		ssd1306_FillRectangle(0, 35, 4, 39, 1);

		// Cône du haut-parleur
		for (int i = 0; i < 4; i++) {
			ssd1306_Line(4 + i, 35 - i, 4 + i, 39 + i, 1);
		}

		for (int i = 0; i < volume_act; i++) {
			ssd1306_Line(15 + i*2, 34, 15 + i*2, 39, 1);
		}

		//Affichage de la batterie en mV
		ssd1306_SetCursor(60, 40);
		ssd1306_WriteString(battery_act, Font_6x8, 1);

		//LIGNE DU MENU EN BAS
		ssd1306_Line(0, 54, 127, 54, 1);
		ssd1306_Line(42, 54, 42, 64, 1);
		ssd1306_Line(84, 54, 84, 64, 1);

		//MOT : PRECEDENT
		ssd1306_SetCursor(8, 56);
		if (reg_volume == 1)
		{
			ssd1306_WriteString("Vol -", Font_6x8, 1);
		}
		else
		{
			ssd1306_WriteString("Prec.", Font_6x8, 1);
		}

		//MOT : SELECT
		ssd1306_SetCursor(46, 56);
		ssd1306_WriteString("Select", Font_6x8, 1);

		//MOT : SUIVANT
		ssd1306_SetCursor(94, 56);
		if (reg_volume == 1)
		{
			ssd1306_WriteString("Vol +", Font_6x8, 1);
		}
		else
		{
			ssd1306_WriteString("Suiv.", Font_6x8, 1);
		}


		// Gestion de la selection
		switch(indice_menu_1)
		{
		case 0:
			ssd1306_InvertRectangle(0, 0, 100, 10);
			break;
		case 1:
			ssd1306_InvertRectangle(0, 10, 100, 20);
			break;
		case 2:
			ssd1306_InvertRectangle(0, 20, 40, 30);
			break;
		case 3:
			ssd1306_InvertRectangle(40, 20, 80, 30);
			break;
		}

		ssd1306_UpdateScreen();
		break;

	case 2:

		//Afficher page 2
		ssd1306_Fill(0);

		// Buffer temporaire local (détruit à la fin de la fonction, libérant la RAM)
		char station_str[21];

		// Boucle pour générer les 3 lignes de stations
		for (uint8_t i = 0; i < 3; i++)
		{
			uint16_t freq_lue = EEPROM_Load_Station(i);

			// Calcul dynamique de la position verticale (Y) : 2, puis 13, puis 24
			uint8_t pos_y = 2 + (i * 11);

			if (freq_lue == 0)
			{
				// Cas où aucune station n'est sauvegardée à cet emplacement
				snprintf(station_str, sizeof(station_str), "Station %d - Vide -", i + 1);
			}
			else
			{
				// Décomposition de la fréquence
				uint16_t mhz_part = freq_lue / 100;
				uint8_t decimal_part = (freq_lue % 100) / 10;
				snprintf(station_str, sizeof(station_str), "Station %d  %d.%d MHz", i + 1, mhz_part, decimal_part);
			}

			ssd1306_SetCursor(0, pos_y);
			ssd1306_WriteString(station_str, Font_6x8, 1);
		}

		//Recherche Auto
		ssd1306_SetCursor(0, 35); // Légèrement décalé pour aérer l'interface
		ssd1306_WriteString("Rech. Auto", Font_6x8, 1);

		//Retour
		ssd1306_SetCursor(66, 35);
		ssd1306_WriteString("Retour", Font_6x8, 1);

		//Reset
		ssd1306_SetCursor(0, 45);
		ssd1306_WriteString("Reset", Font_6x8, 1);

		switch(indice_menu_1)
		{
		case 0:
			ssd1306_InvertRectangle(0, 0, 120, 10);
			break;
		case 1:
			ssd1306_InvertRectangle(0, 10, 120, 20);
			break;
		case 2:
			ssd1306_InvertRectangle(0, 20, 120, 31);
			break;
		case 3:
			ssd1306_InvertRectangle(0, 32, 62, 42);
			break;
		case 4:
			ssd1306_InvertRectangle(64, 32, 120, 42);
			break;
		case 5:
			ssd1306_InvertRectangle(0, 43, 60, 53);
			break;
		}

		//LIGNE DU MENU EN BAS
		ssd1306_Line(0, 54, 127, 54, 1);
		ssd1306_Line(42, 54, 42, 64, 1);
		ssd1306_Line(84, 54, 84, 64, 1);

		//MOT : PRECEDENT
		ssd1306_SetCursor(8, 56);
		if (reg_station == 0)
		{
			ssd1306_WriteString("Prec.", Font_6x8, 1);
		}
		else
		{
			ssd1306_WriteString("Sauv.", Font_6x8, 1);
		}
		//MOT : SELECT
		ssd1306_SetCursor(46, 56);
		ssd1306_WriteString("Select", Font_6x8, 1);

		//MOT : SUIVANT
		ssd1306_SetCursor(94, 56);
		if (reg_station == 0)
		{
			ssd1306_WriteString("Suiv.", Font_6x8, 1);
		}
		else
		{
			ssd1306_WriteString("Ecout.", Font_6x8, 1);
		}

		ssd1306_UpdateScreen();
		break;
	}
}
