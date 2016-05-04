/*
USART2 example, with IRQ

I am using a CP2102 USB-to-USART converter.
Wiring connections:
	STM32F4 			CP2102
	PA2 (USART2 Tx) ->	Rx
	PA3 (USART2 Rx) ->	Tx
*/
#include <stdio.h>
#include "stm32f4xx.h"

#define MAX_STRLEN 50
volatile unsigned char received_string[MAX_STRLEN]; // this will hold the recieved string


/*the usart acept the command from RX when RX interrupt is trigger*/
unsigned char Receive_data;
uint8_t Receive_String_Ready = 0;


volatile uint32_t msTicks; /* counts 1ms timeTicks       */
void SysTick_Handler(void) {
	msTicks++;
}

//  Delays number of Systicks (happens every 1 ms)
static void Delay(__IO uint32_t dlyTicks){                                              
  uint32_t curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks);
}

void setSysTick(){
	// ---------- SysTick timer (1ms) -------- //
	if (SysTick_Config(SystemCoreClock / 1000)) {
		// Capture error
		while (1){};
	}
}

void setup_Periph(){
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	// Enable the APB1 periph clock for USART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	// Enable the GPIOA clock, used by pins PA2, PA3
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	// Enable the GPIOD clock, used by Pin PD14
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	// Setup the GPIO pins for Tx and Rx
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//Set up the GPIO while the data is sending
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed =GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// Connect PA2 and PA3 with the USART2 Alternate Function
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure);

	/* Enable the USART2 receive interrupt and configure
		the interrupt controller to jump to USART2_IRQHandler()
		if the USART2 receive interrupt occurs
	*/
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	// Finally enable the USART2 peripheral
	USART_Cmd(USART2, ENABLE);
}

void USART_puts(USART_TypeDef *USARTx, volatile char *str){
	while(*str){
		// Wait for the TC (Transmission Complete) Flag to be set
		// while(!(USARTx->SR & 0x040));
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		USART_SendData(USARTx, *str);
		*str++;
	}
}

//Interrupt
void USART2_IRQHandler(void){
	if(USART_GetITStatus(USART2, USART_IT_RXNE)){
		GPIO_ToggleBits(GPIOD, GPIO_Pin_14);

		Receive_data = USART_ReceiveData(USART2);
		static uint8_t cnt = 0;

		if(cnt < MAX_STRLEN){
			received_string[cnt] = Receive_data;
			if(received_string[cnt] == '\r'){
				Receive_String_Ready = 1;
				cnt = 0;
			}

			else{
				cnt++;
			}
		}

		else{
			Receive_String_Ready = 1;
			cnt = 0;
		}
		if(Receive_String_Ready){
			USART_puts(USART2, received_string);
			USART_puts(USART2, "\r\n");

			Receive_String_Ready = 0;
			for(int i = 0; i<MAX_STRLEN; i++){
				received_string[i] = 0;
			}
		}
	}
}

int main(void) {
	setSysTick();
	setup_Periph();

	USART_puts(USART2, "Hello World!\n\r");

	while(1){
		// Nothing done here since we are using interrupts
	}

	return 0;
}