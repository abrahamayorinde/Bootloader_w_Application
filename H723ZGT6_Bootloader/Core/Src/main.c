/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "etx_ota_update.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAJOR 0	//BOOTLOADER MAJOR VERSION NUMBER
#define MINOR 2 //BOOTLOADER MINOR VERSION NUMBER
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
const uint8_t BL_Version[2] = { MAJOR, MINOR};
uint8_t RxBuffer[1033];
uint8_t uart_callback;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void  goto_application(void);
static uint32_t received_data = 0;
static uint32_t remaining_data = 0;
static uint16_t next_data_packet_size = 0;
extern HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,
                                        uint16_t data_len, bool is_first_block );
static int upload_complete = 0;
static volatile uint32_t reset_handler_address = (ETX_APP_FLASH_ADDR + 4);
typedef void (application_t)(void);
//static volatile void (*app_reset_handler)(void);
typedef struct
{
    uint32_t		stack_addr;     // Stack Pointer
    application_t*	func_p;        // Program Counter
} JumpStruct;
HAL_StatusTypeDef ex = HAL_ERROR;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	 	 switch(RxBuffer[PACKET_TYPE_INDEX])
	 	 {
	        case ETX_OTA_PACKET_TYPE_CMD:
	        	if(RxBuffer[PACKET_CMD_INDEX] == ETX_OTA_CMD_START)
	        	{
	        		printf("Start packet transmission received.\n");
	        		HAL_UART_Receive_IT(&huart2, RxBuffer, ETX_OTA_HEADER_PACKET_SIZE);
	        	}
	        	if(RxBuffer[PACKET_CMD_INDEX] == ETX_OTA_CMD_END)
	        	{
	        		printf("End of transmission packet received.\n");
	        		upload_complete = 1;
	        		remaining_data = 0;
	        		received_data = 0;
	        		next_data_packet_size = 0;
	        	}
	            break;

	        case ETX_OTA_PACKET_TYPE_DATA:

	        	printf("Data packet received.\n");
	            received_data = (RxBuffer[3]<<8) | (RxBuffer[2]);
	            //function to save received buffer data to flash//
	            ex = write_data_to_flash_app( &(RxBuffer[4]), received_data, has_download_begun() );

	            if(received_data == next_data_packet_size)
	            {
	                printf("Received the expected packet size.\n");
	                remaining_data -= next_data_packet_size;
	            }
	            else
	            {
	                printf("Did not receive the expected packet size.\n");
	                remaining_data -= received_data;
	            }

	            if(remaining_data > ETX_OTA_DATA_MAX_SIZE)
	            {
	                next_data_packet_size = ETX_OTA_DATA_MAX_SIZE;
	            }
	            else
	            {
	                next_data_packet_size = remaining_data;
	            }

	            //write_data_to_flash();
	            if(remaining_data == 0)
	            {
	                HAL_UART_Receive_IT(&huart2, RxBuffer, ETX_OTA_COMMAND_PACKET_SIZE);

	            }
	            else
	            {
	                HAL_UART_Receive_IT(&huart2, RxBuffer, next_data_packet_size + DATA_PACKET_OVERHEAD);
	            }

	            break;

	        case ETX_OTA_PACKET_TYPE_HEADER:

	        	printf("Header packet received.\n");

	            uint32_t total_data_size = ( (RxBuffer[7]<<24) | (RxBuffer[6]<<16) | (RxBuffer[5]<<8) | (RxBuffer[4]) );
	            remaining_data = total_data_size;

	            if(remaining_data > ETX_OTA_DATA_MAX_SIZE)
	            {
	                next_data_packet_size = ETX_OTA_DATA_MAX_SIZE;
	            }
	            else
	            {
	                next_data_packet_size = remaining_data;
	            }

	            HAL_UART_Receive_IT(&huart2, RxBuffer, next_data_packet_size + DATA_PACKET_OVERHEAD);
	            break;

	        default:
	        	printf("Unnamed packet received.\n");
	            //HAL_UART_Receive_IT(&huart2, RxBuffer, 10);
	            break;
	    }

		DELAY_MS(200);
	    etx_ota_send_resp( ETX_OTA_ACK );

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
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, RxBuffer, ETX_OTA_COMMAND_PACKET_SIZE);
  printf("Starting Bootloader (%d.%d)\r\n", BL_Version[0], BL_Version[1]);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); /*Turn on Green User LED PB0*/
  printf("Starting Bootloader (%d.%d)\r\n", BL_Version[0], BL_Version[1]);

  /* Check the user push button is pressed for 3 seconds */
  GPIO_PinState OTA_Pin_state;
  uint32_t end_tick = HAL_GetTick() + 3000;

  printf("Press the User Button to trigger OTA update...\r\n");
  do
  {
	  OTA_Pin_state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

	  uint32_t current_tick = HAL_GetTick();

	  if( (OTA_Pin_state != GPIO_PIN_RESET) || (current_tick > end_tick) )
	  {
		  // Either timeout or the button is pressed //
		  break;
	  }

  }while(1);

  /* STart the Firmware/Application update */

  if(OTA_Pin_state == GPIO_PIN_SET)
  {
	  printf("Starting Firmware Download!!!\r\n");

		upload_complete = 0;
		remaining_data = 0;
		received_data = 0;
		next_data_packet_size = 0;

	  // OTA Request receive the data from the UART and flash //
	  /*
	  if( etx_ota_download_and_flash() != ETX_OTA_EX_OK)
	  {
		  // Error.  Do not process. //
		  printf("OTA Update: ERROR!!! HALT!!!\r\n");
		  while(1);
	  }
	  else
	  {
		  // Reset to load the new application //
		  printf("Firmware update is done!!! Rebooting...\r\n");
		  HAL_NVIC_SystemReset();
	  }
	  */
	  /*
	  while(upload_complete == 0)
	  {

	  }
	  if(upload_complete == 1)
	  {
		  // Reset to load the new application //
		  printf("Firmware update is done!!! Rebooting...\r\n");
		  upload_complete = 0;
		  HAL_NVIC_SystemReset();
	  }
	  */
		while(1)
		{

		}
  }

  //HAL_Delay(2000);

  goto_application();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  *
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PE1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 *  @brief Print the characters to UART (printf).
 *  @retval int
 */
#ifdef __GNUC__
	/* With GCC, a small printf (option LD Linker->Libraries->Small printf
	 * set to 'Yes') calls __io_putchar()
	 */
int _write(int file, char*ptr, int len)
#else
int fputc(int ch, FILE *f)
#endif /*(__GNUC__)*/
{
	/* Place your implementation of fputc here*/
	/* e.g write a character to the UART3 and Loop until the end of transmission */
	int i = 0;
	for(i = 0; i < len; i++)
	{
		ITM_SendChar((*ptr++));
	}
	return len;

}

static void goto_application (void)
{
	printf("Jumping to Application \r\n");

	uint32_t *addr = (uint32_t*)(ETX_APP_FLASH_ADDR + 0x04);
	uint32_t read_value = *addr;

	if(read_value != 0xFFFFFFFF)
	{
		const void (*app_reset_handler)(void) = (void*)(*(volatile uint32_t *)(0x08040000 + 0x04));
		//app_reset_handler = (void *)*(volatile uint32_t *)reset_handler_address;
		//const JumpStruct* vector_p = (JumpStruct*)reset_handler_address;
		//__set_MSP((*(volatile uint32_t *) 0x08080000));

		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); //turn off the GREEN LED PB0

		//SCB->VTOR = (uint32_t) *((__IO uint32_t*)ETX_APP_FLASH_ADDR);

		app_reset_handler();	//call the application reset handler
		//asm("msr msp, %0; bx %1;" : : "r"(vector_p->stack_addr), "r"(vector_p->func_p));
	}
}
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
