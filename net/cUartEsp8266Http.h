// cUartEsp8266Http.h - stm32f769i disco uart5 esp8266  http
#pragma once
#include "cEsp8266Http.h"

UART_HandleTypeDef UartHandle;
#define CIRC_BUF_SIZE 1024
static DMA_HandleTypeDef hdma_tx;
static DMA_HandleTypeDef hdma_rx;
static volatile bool mTxDone = false;
//{{{
void HAL_UART_TxCpltCallback (UART_HandleTypeDef *huart) {
  mTxDone = true;
  }
//}}}

class cUartEsp8266Http : public cEsp8266Http {
public:
  cUartEsp8266Http() : cEsp8266Http() {}
  virtual ~cUartEsp8266Http() {}

protected:
  //{{{
  virtual void initialiseComms() {
   //  init uart5

    __UART5_FORCE_RESET();
    __UART5_RELEASE_RESET();

    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();
    __UART5_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // PC12 - UART5 tx pin configuration
    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin       = GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init (GPIOC, &GPIO_InitStruct);

    // PD2 - UART5 rx pin configuration
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init (GPIOD, &GPIO_InitStruct);

    //{{{  config tx DMA
    hdma_tx.Instance                 = DMA1_Stream7;
    hdma_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.Mode                = DMA_NORMAL;
    hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init (&hdma_tx);

    __HAL_LINKDMA(&UartHandle, hdmatx, hdma_tx);

    HAL_NVIC_SetPriority (DMA1_Stream7_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ (DMA1_Stream7_IRQn);
    //}}}
    //{{{  config rx DMA
    hdma_rx.Instance                 = DMA1_Stream0;
    hdma_rx.Init.Channel             = DMA_CHANNEL_4;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.Mode                = DMA_CIRCULAR; // DMA_NORMAL;
    hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH; // DMA_PRIORITY_LOW;
    //hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init (&hdma_rx);

    __HAL_LINKDMA(&UartHandle, hdmarx, hdma_rx);

    HAL_NVIC_SetPriority (DMA1_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ (DMA1_Stream0_IRQn);
    //}}}

    // NVIC for UART5
    HAL_NVIC_SetPriority (UART5_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ (UART5_IRQn);

    // 8 Bits, One Stop bit, Parity = None, RTS,CTS flow control
    UartHandle.Instance        = UART5;
    UartHandle.Init.BaudRate   = 921600;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;
    HAL_UART_Init (&UartHandle);

    // setup dma rx with circular buffer
    mRxPtr = 0;
    HAL_UART_Receive_DMA (&UartHandle, (uint8_t*)UART_DMA, CIRC_BUF_SIZE);
    }
  //}}}
  //{{{
  virtual void sendCommand (const std::string& command) {
    //debug ("sendC:" + command);
    std::string fullCommand = command + "\r\n";
    HAL_UART_Transmit_DMA (&UartHandle, (uint8_t*)(fullCommand.c_str()), fullCommand.size());
    while (!mTxDone) { vTaskDelay (1); }
    mTxDone = false;
    }
  //}}}
  //{{{
  virtual uint8_t readChar() {
    uint8_t ch = 0xFF;

    int timeout = 0;
    while (mRxPtr == (CIRC_BUF_SIZE - UartHandle.hdmarx->Instance->NDTR) % CIRC_BUF_SIZE) { timeout++; }
    if (mRxPtr != (CIRC_BUF_SIZE - UartHandle.hdmarx->Instance->NDTR) % CIRC_BUF_SIZE) {
      ch = *((uint8_t*)(UART_DMA + mRxPtr));
      mRxPtr = (mRxPtr+1) % CIRC_BUF_SIZE;
      }
    else
      debug ("rxErr " + dec (UartHandle.hdmarx->Instance->NDTR));

    return ch;
    }
  //}}}
  //{{{
  virtual bool readChars (uint8_t* content, DWORD bytesLeft, DWORD& numRead) {
    *content = readChar();
    numRead = 1;
    return true;
    }
  //}}}

private:
  unsigned int mRxPtr = 0;
  };
