#include "sl_system_init.h"
#include "app.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

#include "stdio.h"
// Định nghĩa chân cho UART và LED
#define BSP_TXPORT gpioPortA
#define BSP_RXPORT gpioPortA
#define BSP_TXPIN 5
#define BSP_RXPIN 6
#define BSP_ENABLE_PORT gpioPortD
#define BSP_ENABLE_PIN 4

#define BSP_GPIO_LEDS
#define BSP_GPIO_LED0_PORT gpioPortD
#define BSP_GPIO_LED0_PIN 2
#define BSP_GPIO_LED1_PORT gpioPortD
#define BSP_GPIO_LED1_PIN 3
#define BSP_GPIO_PB0_PORT gpioPortB
#define BSP_GPIO_PB0_PIN 0
#define BSP_GPIO_PB1_PORT gpioPortB
#define BSP_GPIO_PB1_PIN 1

#define BUFLEN 50
uint32_t oversampiling=0;

// Hàm khởi tạo GPIO
void initGPIO(void)
{
  // Cấu hình chân TX, RX cho UART
  GPIO_PinModeSet(BSP_TXPORT, BSP_TXPIN, gpioModePushPull, 1);
  GPIO_PinModeSet(BSP_RXPORT, BSP_RXPIN, gpioModeInput, 0);
  // Kích hoạt VCOM cho giao tiếp UART với PC
  GPIO_PinModeSet(BSP_ENABLE_PORT, BSP_ENABLE_PIN, gpioModePushPull, 1);
}

// Hàm khởi tạo USART0
void initUSART0(void)
{
  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
  init.enable = usartEnable;
  init.baudrate = 115200;
  init.oversampling = usartOVS16;
  init.databits = usartDatabits8;
  init.parity = usartNoParity;
  init.stopbits = usartStopbits1;

  USART_InitAsync(USART0, &init);

  // Định tuyến chân TX, RX cho USART0
  GPIO->USARTROUTE[0].TXROUTE = (BSP_TXPORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (BSP_TXPIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE = (BSP_RXPORT << _GPIO_USART_RXROUTE_PORT_SHIFT) | (BSP_RXPIN << _GPIO_USART_RXROUTE_PIN_SHIFT);

  // Bật tín hiệu RX và TX
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;
  oversampiling=init.oversampling;
}

void UART_print_float(USART_TypeDef *usart, int data) {
  char buffer[20]; 
  sprintf(buffer,"%d\n", data); //convert int to string type
	int i=sizeof(buffer);
  for (int j = 0; j <i; j++)
       USART_Tx(USART0, buffer[j]); //transmit

}

int main(void)
{
  sl_system_init();
  app_init();


  uint8_t buffer[BUFLEN];
  uint32_t refReq=40*10^6; //Frequency of board
  uint32_t clock_div=5303;//Calculate from formula of USART
  uint8_t i=0;
  uint32_t br=0;
  uint32_t timeTruyen=0;
 // uint32_t baud_rate=USART_BaudrateGet(USART0);

  // Khởi tạo GPIO và USART0
  initUSART0();

  while (1) {
    sl_system_process_action();
    app_process_action();

    // Zero out buffer
       for (i = BUFLEN; i > 0; --i)
         buffer[i] = 0;

       // Receive BUFLEN characters unless a new line is received first
       do
       {
         // Wait for a character
         buffer[i] = USART_Rx(USART0);

         // Exit loop on new line
         if (buffer[i] != '\r')
           i++;
         else
           break;
       }
       while (i < BUFLEN);
    i--;//length of bufffer
    br=USART_BaudrateCalc(refReq,clock_div, false, usartOVS16);
    timeTruyen=((i*10)/br)*1000;//convert to ms
    UART_print_float(USART0,timeTruyen,i);

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    sl_power_manager_sleep();
#endif
  }
}
