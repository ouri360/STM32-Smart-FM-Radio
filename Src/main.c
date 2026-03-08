/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "gpio.h"
#include "stdio.h"
#include "adc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "menu.h"
#include "RDA_5807.h"
#include "eeprom.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// INITIALISATION DES VARIABLES (Partie 1)
uint8_t page = 0;
uint8_t reg_volume = 0;
uint8_t reg_station = 0;
uint8_t bass_boost = 1;
uint8_t volume_act;
uint8_t RSSI;
uint16_t raw_battery_adc = 0;
uint8_t hauteur_remplissage;
uint16_t freq_lue = 0; //Pour vérification selection de station (page 2)

char freq_str[16]; // Pour stocker la fréquence complète
uint16_t current_freq;

char battery_act[16];

// la variable hi2c1 est définie en extern dans i2c.h
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;

// Variables pour la sauvegarde EEPROM temporisée
uint32_t temps_derniere_action = 0; // Stocke le moment du dernier appui
uint8_t sauvegarde_en_attente = 0;  // Flag : 1 si on doit sauvegarder, 0 sinon
uint8_t volume_sauvegarde = 0;      // Garde en mémoire le dernier volume écrit dans l'EEPROM
uint16_t freq_sauvegardee = 0;      // Garde en mémoire la dernière fréquence écrite dans l'EEPROM

// Variable pour la liste de stations (Page 2)
uint8_t station_en_cours_de_modif = 0; // Stocke l'index de la station sélectionnée (0, 1 ou 2)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Fonction "Callback" appelée automatiquement par le pilote radio pendant la recherche
void Animation_Recherche(void)
{
    static uint8_t compteur_anim = 0;
    compteur_anim++;

    // On ne met à jour l'écran qu'une fois sur 10
    if (compteur_anim % 10 == 0)
    {
        // On fait clignoter un petit carré en bas à droite de l'écran pendant la recherche
        ssd1306_InvertRectangle(110, 50, 118, 58);
        ssd1306_UpdateScreen();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  // INITIALISATION DES DRIVERS

  ssd1306_Init();
  RDA_Init(&hi2c1);

  // INITIALISATION DES VARIABLES (Partie 2)
  uint8_t etat_G;
  uint8_t etat_C;
  uint8_t etat_D;

  GPIO_PinState BTN_GAUCHE;
  GPIO_PinState BTN_CENTRE;
  GPIO_PinState BTN_DROITE;

  etat_G = 0;
  etat_C = 0;
  etat_D = 0;

  BTN_GAUCHE = GPIO_PIN_RESET;
  BTN_CENTRE = GPIO_PIN_RESET;
  BTN_DROITE = GPIO_PIN_RESET;

  uint16_t adc_vide = 3120;   	// Correspond à 3.2V
  uint16_t adc_pleine = 4095; 	// Correspond à 4.2V

  //Volume et fréquence par défaut (à lire dans EEPROM plus tard)
  //RDA_SetVolume(&hi2c1, volume_act);
  //RDA_Tune(&hi2c1, 10550);  //105.5 MHz

  // Charger les valeurs sauvegardées
  EEPROM_Load_State(&volume_act, &current_freq);

  // Initialisation des variable de suivis (pour sauvegarde temporisée)
  volume_sauvegarde = volume_act;
  freq_sauvegardee = current_freq;

  // Initialiser la puce radio avec ces valeurs
  RDA_SetVolume(&hi2c1, volume_act);
  RDA_Tune(&hi2c1, current_freq);
  RDA_ResetRDSName();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  //Recupère la qualité du signal radio dans un int (0 -> 127) pour affichage
	  RSSI = RDA_GetQuality(&hi2c1);


	  // --- RECUPERATION DE LA BATTERIE ---
	  // 1. Démarrage de la conversion ADC
	  HAL_ADC_Start(&hadc1);

	  // 1.1 On stock le statut du HAL_ADC_PollForCOnversion pour l'analyser
	  HAL_StatusTypeDef status_adc = HAL_ADC_PollForConversion(&hadc1, 10);

	  // 2. Attente de la fin de conversion (+ timeout de sécurité de 10ms)
	  if (status_adc == HAL_OK)
	  {
	      // 3. Récupération de la valeur de batterie actuelle
	      raw_battery_adc = HAL_ADC_GetValue(&hadc1);

	 	  // 4. Logique d'affichage de la batterie (bornes min et max)
	 	  if (raw_battery_adc < adc_vide) raw_battery_adc = adc_vide;
	 	  if (raw_battery_adc > adc_pleine) raw_battery_adc = adc_pleine;

	 	  // 5. Affichage de la batterie : A modifier selon la taille de la barre pour avoir le bon ratio (ici hauteur de 33 pixel)
	 	  hauteur_remplissage = 48 - (((raw_battery_adc - adc_vide) * 33) / (adc_pleine - adc_vide));

	 	  // 6. Construction chaine de caractère pour afficher la batterie en mV sur l'écran
	 	  snprintf(battery_act, sizeof(battery_act), "Bat:%d mV", raw_battery_adc);

	  }
	  else
	  {
	   // GESTION D'ERREUR HAL
	   // On affiche "E: " suivi du code (1=ERR, 2=BUSY, 3=TIMEOUT)
	   snprintf(battery_act, sizeof(battery_act), "E:%d", status_adc);

	   // On vide visuellement la jauge de batterie ("ERREUR")
	   hauteur_remplissage = 48;
	  }

	  // Arrêt de l'ADC pour économiser l'énergie
	  HAL_ADC_Stop(&hadc1);



	  // --- AFFICHAGE DE LA FREQUENCE ACTUELLE ---
	  // 1. Lecture de la fréquence actuelle dans une variable
	  current_freq = RDA_GetRealFrequency(&hi2c1);

	  // 2. Decomposition de la fréquence en partie entière et partie décimale
	  uint16_t mhz_part = current_freq / 100;
	  uint8_t decimal_part = (current_freq % 100) / 10;

	  // 3. Construction d'une chaine de caractères pour la fréquence actuelle en MHz (ex: "105.5 MHz")
	  snprintf(freq_str, sizeof(freq_str), "%d.%d", mhz_part, decimal_part);


	  // LECTURE RDS
	  // Cette fonction met à jour "rda_station_name"
	  RDA_UpdateRDSName(&hi2c1);


	  // Affichage du menu sur la page en cours
	  menu(page);


	  // Fonction Bass-Boost
	  if (bass_boost == 1)
	  {
		  RDA_SetBass(&hi2c1, TRUE);
	  }
	  else   RDA_SetBass(&hi2c1, FALSE);


	  //LECTURE DES ENTREES (Boutons)
	  BTN_GAUCHE = HAL_GPIO_ReadPin(LEFT_BTN_GPIO_Port, LEFT_BTN_Pin);
	  BTN_CENTRE = HAL_GPIO_ReadPin(CENTER_BTN_GPIO_Port, CENTER_BTN_Pin);
	  BTN_DROITE = HAL_GPIO_ReadPin(RIGHT_BTN_GPIO_Port, RIGHT_BTN_Pin);


	  //GESTION DES 3 FRONTS MONTANTS BOUTONS (Empêche le double input)
	  switch (etat_G)
	{
	  case 0:
		  if (BTN_GAUCHE == GPIO_PIN_RESET)
			  {
			  etat_G = 1;
			  }
		  break;
	  case 1:
		  if (BTN_GAUCHE == GPIO_PIN_SET)
		  {
			  etat_G = 0;
		  }
		  break;
	}

	  switch (etat_C)
	{
	  case 0:
		  if (BTN_CENTRE == GPIO_PIN_RESET)
			  {
			  etat_C = 1;
			  }
		  break;
	  case 1:
		  if (BTN_CENTRE == GPIO_PIN_SET)
		  {
			  etat_C = 0;
		  }
		  break;

	}

	  switch (etat_D)
	{
	  case 0:
		  if (BTN_DROITE == GPIO_PIN_RESET)
			  {
			  etat_D = 1;
			  }
		  break;
	  case 1:
		  if (BTN_DROITE == GPIO_PIN_SET)
		  {
			  etat_D = 0;
		  }
		  break;
	}


	  //GESTION DES COMMANDES BOUTONS
	  if (etat_C == 1)
	  {
		  switch(page)
		  {
		  case 0:
			  page = 1;
			  break;
		  case 1:
			  //VALIDER
			  switch(indice_menu_1)
			  {
			  case 0:
				  //ON/OFF Bass Boost
				  bass_boost = !(bass_boost);
				  break;
			  case 1:
				  //Recherche Station (Page 2)
				  page = 2;
				  indice_menu_1 = 0;
				  break;
			  case 2:
				  //Réglage Volume
				  reg_volume = !(reg_volume);
				  break;
			  case 3:
				  //Retour Page 0
				  page = 0;
				  indice_menu_1 = 0;
				  break;
			  }
			  break;
		  case 2:
			  //VALIDER
			  switch(indice_menu_1)
			  {
			  case 0:
				  //Station 1
				  station_en_cours_de_modif = indice_menu_1;
				  reg_station = !(reg_station);
				  break;
			  case 1:
				  //Station 2
				  station_en_cours_de_modif = indice_menu_1;
				  reg_station = !(reg_station);
				  break;
			  case 2:
				  //Station 3
				  station_en_cours_de_modif = indice_menu_1;
				  reg_station = !(reg_station);
				  break;
			  case 3:
				  //RECHERCHE AUTO

				  ssd1306_Fill(0);
				  ssd1306_SetCursor(10, 25);
				  ssd1306_WriteString("Recherche...", Font_11x18, 1);
				  ssd1306_UpdateScreen();

				  // 1. On augmente l'exigence de qualité du signal (ex: niveau 8 sur 15)
				  RDA_SetSeekThreshold(&hi2c1, 8);

				  // 2. On parcourt les 3 emplacements de la mémoire
				  for (uint8_t i = 0; i < 3; i++)
				  {
					  // On ne déclenche la recherche que si le "slot" est vide
					  if (EEPROM_Load_Station(i) == 0)
					  {
						  // Lancement du Seek :
						  // Param 2: seek_mode = 0 (Repart au début de la bande si on atteint la fin)
						  // Param 3: direction = 1 (Recherche vers le haut, SeekUp)
						  // Param 4: showfreq = Fonction Animation_Recherche
						  if (RDA_Seek(&hi2c1, 0, 1, Animation_Recherche) == RDA_OK)
						  {
							  // Une station a été trouvée, on récupère sa fréquence exacte
							  uint16_t freq_trouvee = RDA_GetRealFrequency(&hi2c1);

							  // On sauvegarde cette fréquence dans l'EEPROM au slot 'i'
							  EEPROM_Save_Station(i, freq_trouvee);
							  HAL_Delay(100);
						  }
						  else
						  {
							  // Le scan a fait tout le tour de la bande sans rien trouver
							  ssd1306_Fill(0);

							  ssd1306_SetCursor(30, 15);
							  ssd1306_WriteString("Aucune", Font_11x18, 1);
							  ssd1306_SetCursor(25, 35);
							  ssd1306_WriteString("Station", Font_11x18, 1);

							  ssd1306_UpdateScreen();

							  // Temps d'attente lecture
							  HAL_Delay(2000);

							  break; // On sort de la boucle for
						  }
					  }
				  }

				  // 3. On remet la sensibilité à son niveau normal (1)
				  RDA_SetSeekThreshold(&hi2c1, 1);

				  // Une fois la recherche terminée, on met à jour le système avec la dernière fréquence trouvée
				  current_freq = RDA_GetRealFrequency(&hi2c1);
				  temps_derniere_action = HAL_GetTick();
				  sauvegarde_en_attente = 1;

				  // On remonte le curseur du menu
				  indice_menu_1 = 0;
				  break;
			  case 4:
				  //Retour Page 1
				  page = 1;
				  indice_menu_1 = 3;
				  break;
			  case 5:
				  //Action du bouton RESET
				  EEPROM_Reset_Stations();
				  //On remonte le curseur tout en haut de la liste
				  indice_menu_1 = 0;
				  break;
			  }
			  break;
		  }
	  }
	  else if (etat_G == 1)
	  {
		  switch(page)
		  {
		  case 0:
			  //STATION PRECEDENTE
			  RDA_ManualDown(&hi2c1);
			  RDA_ResetRDSName();

			  //Flag sauvegarde en attente + temps
			  temps_derniere_action = HAL_GetTick();
			  sauvegarde_en_attente = 1;
			  break;
		  case 1:

			  //PRECEDENT
			  if (reg_volume == 0)
			  {
				  if (indice_menu_1 > 0)
				  {
					  indice_menu_1 = indice_menu_1 - 1;
				  }
			  }
			  else //Volume DOWN
				  {
				  	  if (volume_act > 0)
				  	  {
				  		  volume_act = volume_act - 1;
					  	  RDA_SetVolume(&hi2c1, volume_act);

					  	  temps_derniere_action = HAL_GetTick();
					  	  sauvegarde_en_attente = 1;
				  	  }
				  }
			  break;

		  case 2:
			  //PRECEDENT
			  if (reg_station == 0)
			  {
				  if (indice_menu_1 > 0)
				  {
					  indice_menu_1 = indice_menu_1 - 1;
				  }
			  }
			  else //Sauvegarder station en cours dans slot
			  {
				  // SAUVEGARDER
				  EEPROM_Save_Station(station_en_cours_de_modif, current_freq);
			  }
			  break;
		  }
	  }
	  else if (etat_D == 1)
	  {
		  switch(page)
		  {
		  case 0:
			  //STATION SUIVANTE
			  RDA_ManualUp(&hi2c1);
			  RDA_ResetRDSName();

			  temps_derniere_action = HAL_GetTick();
			  sauvegarde_en_attente = 1;
			  break;
		  case 1:

			  if (reg_volume == 0)
			  {
				  //SUIVANT
				  if (indice_menu_1 < 3)
				  {
					  indice_menu_1 = indice_menu_1 + 1;
				  }
			  }
			  else //Volume UP
			  {
				  if (volume_act < 15)
				  {
					  volume_act = volume_act + 1;
					  RDA_SetVolume(&hi2c1, volume_act);

					  temps_derniere_action = HAL_GetTick();
					  sauvegarde_en_attente = 1;
				  }
			  }
			  break;

		  case 2:
			  //SUIVANT
			  if (reg_station == 0)
			  {
				  if (indice_menu_1 < 5)
				  {
					  indice_menu_1 = indice_menu_1 + 1;
				  }
			  }
			  else //Selectionner la station
			  {
				  // SÉLECTIONNER
				  freq_lue = EEPROM_Load_Station(station_en_cours_de_modif);

				  // On ne change la radio que si une station était bien sauvegardée
				  if (freq_lue != 0)
				  {
					  current_freq = freq_lue;
					  RDA_Tune(&hi2c1, current_freq);
					  RDA_ResetRDSName();

					  // On sauvegarde la station actuelle dans l'EEPROM (Flag : 1)
					  temps_derniere_action = HAL_GetTick();
					  sauvegarde_en_attente = 1;

					  // RETOUR A LA PAGE PRINCIPALE
					  page = 0;             // Retour à l'écran d'accueil
					  indice_menu_1 = 0;    // Remise à zéro du curseur
					  reg_station = 0;      // On sort du mode de sélection
				  }
			  }
		  break;
		  }
	  }



	  // GESTION DE LA SAUVEGARDE EEPROM (Flag "Sauvegarde en attente")
	  if (sauvegarde_en_attente == 1)
	  {
		  // Si le temps actuel moins le temps de la dernière action est supérieur à 3000 ms (3 secondes)
		  if ((HAL_GetTick() - temps_derniere_action) > 3000)
		  {
			  // On vérifie si les valeurs ont réellement changé par rapport à l'EEPROM
			  if ((volume_act != volume_sauvegarde) || (current_freq != freq_sauvegardee))
			  {
				  EEPROM_Save_State(volume_act, current_freq); // Écriture dans l'EEPROM I2C

				  // Mise à jour des variables de référence
				  volume_sauvegarde = volume_act;
				  freq_sauvegardee = current_freq;
			  }
			  // Flag = 0 --> Pas de sauvegarde en attente
			  sauvegarde_en_attente = 0;
		  }
	  }


	  // Delay de 150 ms entre chaque actualisation de la boucle while
	  HAL_Delay(150);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_LSE, RCC_MCODIV_1);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
