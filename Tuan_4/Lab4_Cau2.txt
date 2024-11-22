#include "sl_system_init.h"
#include "app.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_ldma.h"
#include "em_emu.h"
#include "em_timer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BSP_TXPORT gpioPortA
#define BSP_RXPORT gpioPortA
#define BSP_TXPIN 5
#define BSP_RXPIN 6
#define RX_LDMA_CHANNEL 0
#define TX_LDMA_CHANNEL 1

#define BUFFER_SIZE_10 10
#define BUFFER_SIZE_100 100
#define BUFFER_SIZE_1000 1000

uint8_t outbuf[BUFFER_SIZE_1000];
uint8_t inbuf[1];
LDMA_Descriptor_t ldmaTXDescriptor;
LDMA_TransferCfg_t ldmaTXConfig;
volatile bool dma_done = false;

void initGPIO(void) {
  GPIO_PinModeSet(BSP_TXPORT, BSP_TXPIN, gpioModePushPull, 1);
  GPIO_PinModeSet(BSP_RXPORT, BSP_RXPIN, gpioModeInput, 0);
}

void initUSART0(void) {
  USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

  GPIO->USARTROUTE[0].TXROUTE = (BSP_TXPORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
          | (BSP_TXPIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].RXROUTE = (BSP_RXPORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
          | (BSP_RXPIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

  USART_InitAsync(USART0, &init);
}

void initLDMA(void) {
  LDMA_Init_t ldmaInit = LDMA_INIT_DEFAULT;
  LDMA_Init(&ldmaInit);
}

void LDMA_IRQHandler() {
  uint32_t flags = LDMA_IntGet();
  LDMA_IntClear(flags);
  if (flags & (1 << TX_LDMA_CHANNEL)) {
      dma_done = true;
  }
}

void startDMA(uint8_t *data, uint32_t length) {
  dma_done = false;
  ldmaTXDescriptor = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(data, &(USART0->TXDATA), length);
  ldmaTXConfig = (LDMA_TransferCfg_t)LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_USART0_TXBL);
  LDMA_StartTransfer(TX_LDMA_CHANNEL, &ldmaTXConfig, &ldmaTXDescriptor);
}

//void initTimer(void) {
//  CMU_ClockEnable(cmuClock_TIMER0, true);
//  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
//  TIMER_Init(TIMER0, &timerInit);
//}

//uint32_t measureDMA(uint8_t *data, uint32_t length) {
//  TIMER_CounterSet(TIMER0, 0);
//  TIMER_Enable(TIMER0, true);
//  startDMA(data, length);
//  while (!dma_done);
//  TIMER_Enable(TIMER0, false);
//  return TIMER_CounterGet(TIMER0);
//}

void initTimer(void)
{
  // Initialize the timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  TIMER_Init(TIMER0, &timerInit);
  TIMER_TopSet(TIMER0, 0xFFFFFFFF);
}

uint32_t calculatePeriod(uint32_t numClk)
{
  uint32_t timerClockMHz = CMU_ClockFreqGet(cmuClock_TIMER0) / 1000000;
  // Convert the count between edges to a period in microseconds
  return (numClk / timerClockMHz);
}
uint32_t measureDMA(uint8_t *data, uint32_t length) {
  uint32_t numClk = 0;
  uint32_t elapsedTime = 0;
  TIMER_CounterSet( TIMER0, 0 );
  TIMER_Enable(TIMER0, true);
  startDMA(data, length);
  while (!dma_done);
  TIMER_Enable(TIMER0, false);
  numClk = TIMER_CounterGet(TIMER0);
  elapsedTime = calculatePeriod(numClk);
  return elapsedTime;
}

void generateRandomData(uint8_t *buffer, uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
      buffer[i] = 'a' + (rand() % 26);
  }
}

int main(void) {
  sl_system_init();
  initGPIO();
  initUSART0();
  initLDMA();
  initTimer();

  while (1) {
      if (USART_StatusGet(USART0) & USART_STATUS_RXDATAV) {
          inbuf[0] = USART_Rx(USART0);

          uint32_t length = 0;
          if (inbuf[0] == '1') {
              length = BUFFER_SIZE_10;
          } else if (inbuf[0] == '2') {
              length = BUFFER_SIZE_100;
          } else if (inbuf[0] == '3') {
              length = BUFFER_SIZE_1000;
          } else {
              continue;
          }

          generateRandomData(outbuf, length);
          uint32_t dmaTime = measureDMA(outbuf, length);

          char result[50];
          sprintf(result, "\r\nDMA Time: %lu us\r\n", dmaTime);
          startDMA((uint8_t *)result, strlen(result));
      }
  }
}