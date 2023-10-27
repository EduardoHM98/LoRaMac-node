/*!
 * \file      uart-board.c
 *
 * \brief     Target board UART driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 * 
 * \author    Christian Lehmen ( HT Micron )
 */
#include "bluenrg_lpx.h"
#include "utilities.h"
#include "board.h"
#include "sysIrqHandlers.h"
#include "uart-board.h"
#include "rf_driver_hal_uart.h"
#include "rf_driver_hal_cortex.h"
#include "rf_driver_hal_rcc.h"
#include "rf_driver_hal_gpio.h"
#include "rf_device_hal_conf.h"
#include <stdio.h>


/*!
 * Number of times the UartPutBuffer will try to send the buffer before
 * returning ERROR
 */
#define TX_BUFFER_RETRY_COUNT                       10

static UART_HandleTypeDef UartHandle;
uint8_t RxData = 0;
uint8_t TxData = 0;

extern Uart_t Uart1;


#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}


void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(huart->Instance==USART1)
  {

    /* Peripheral clock enable */
    __HAL_RCC_USART_CLK_ENABLE();

  
    /**USART1 GPIO Configuration    
    PA9/AF0     ------> USART1_TX
    PA8/AF0     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  }

}


void UartMcuInit( Uart_t *obj, UartId_t uartId, PinNames tx, PinNames rx )
{
    obj->UartId = uartId;

    if( uartId == UART_USB_CDC )
    {
#if defined( USE_USB_CDC )
        UartUsbInit( obj, uartId, NC, NC );
#endif
    }
    else
    {
        __HAL_RCC_USART_FORCE_RESET( );
        __HAL_RCC_USART_RELEASE_RESET( );
        __HAL_RCC_USART_CLK_ENABLE( );

        //USART1 GPIO Configuration    
		//PA9/AF0     ------> USART1_TX
		//PA8/AF0     ------> USART1_RX
		
        GpioInit( &obj->Tx, tx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF0_USART1 );
        GpioInit( &obj->Rx, rx, PIN_ALTERNATE_FCT, PIN_PUSH_PULL, PIN_PULL_UP, GPIO_AF0_USART1 );
    }
    
}
  

#define ALTERNATIVE_UART
#ifndef ALTERNATIVE_UART
void UartMcuConfig( Uart_t *obj, UartMode_t mode, uint32_t baudrate, WordLength_t wordLength, StopBits_t stopBits, Parity_t parity, FlowCtrl_t flowCtrl )
{
    if( obj->UartId == UART_USB_CDC )
    {
#if defined( USE_USB_CDC )
        UartUsbConfig( obj, mode, baudrate, wordLength, stopBits, parity, flowCtrl );
#endif
    }
    else
    {
        UartHandle.Instance = USART1;
        UartHandle.Init.BaudRate = baudrate;

        if( mode == TX_ONLY )
        {
            if( obj->FifoTx.Data == NULL )
            {
                assert_param( LMN_STATUS_ERROR );
            }
            UartHandle.Init.Mode = UART_MODE_TX;
        }
        else if( mode == RX_ONLY )
        {
            if( obj->FifoRx.Data == NULL )
            {
                assert_param( LMN_STATUS_ERROR );
            }
            UartHandle.Init.Mode = UART_MODE_RX;
        }
        else if( mode == RX_TX )
        {
            if( ( obj->FifoTx.Data == NULL ) || ( obj->FifoRx.Data == NULL ) )
            {
                assert_param( LMN_STATUS_ERROR );
            }
            UartHandle.Init.Mode = UART_MODE_TX_RX;
        }
        else
        {
            assert_param( LMN_STATUS_ERROR );
        }

        if( wordLength == UART_8_BIT )
        {
            UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
        }
        else if( wordLength == UART_9_BIT )
        {
            UartHandle.Init.WordLength = UART_WORDLENGTH_9B;
        }

        switch( stopBits )
        {
        case UART_2_STOP_BIT:
            UartHandle.Init.StopBits = UART_STOPBITS_2;
            break;
        case UART_1_5_STOP_BIT:
            UartHandle.Init.StopBits = UART_STOPBITS_1_5;
            break;
        case UART_1_STOP_BIT:
        default:
            UartHandle.Init.StopBits = UART_STOPBITS_1;
            break;
        }

        if( parity == NO_PARITY )
        {
            UartHandle.Init.Parity = UART_PARITY_NONE;
        }
        else if( parity == EVEN_PARITY )
        {
            UartHandle.Init.Parity = UART_PARITY_EVEN;
        }
        else
        {
            UartHandle.Init.Parity = UART_PARITY_ODD;
        }

        if( flowCtrl == NO_FLOW_CTRL )
        {
            UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        }
        else if( flowCtrl == RTS_FLOW_CTRL )
        {
            UartHandle.Init.HwFlowCtl = UART_HWCONTROL_RTS;
        }
        else if( flowCtrl == CTS_FLOW_CTRL )
        {
            UartHandle.Init.HwFlowCtl = UART_HWCONTROL_CTS;
        }
        else if( flowCtrl == RTS_CTS_FLOW_CTRL )
        {
            UartHandle.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
        }

        UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

        if( HAL_UART_Init( &UartHandle ) != HAL_OK )
        {
            assert_param( LMN_STATUS_ERROR );
        }
        
        HAL_NVIC_SetPriority( USART1_IRQn, 0 );
        HAL_NVIC_EnableIRQ( USART1_IRQn );



        /* Enable the UART Data Register not empty Interrupt */
        HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
    }
}
#else
void MX_USART1_UART_Init(void){
    
  UartHandle.Instance = USART1;
  UartHandle.Init.BaudRate = 115200U;
  UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits = UART_STOPBITS_1;
  UartHandle.Init.Parity = UART_PARITY_NONE;
  UartHandle.Init.Mode = UART_MODE_TX_RX;
  UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  UartHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  UartHandle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&UartHandle) != HAL_OK)
  {
		printf("Error HAL_UART_Init\n");
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&UartHandle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
		printf("Error HAL_UARTEx_SetTxFifoThreshold\n");

  }
  if (HAL_UARTEx_SetRxFifoThreshold(&UartHandle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
		printf("Error HAL_UARTEx_SetRxFifoThreshold\n");

  }
  if (HAL_UARTEx_DisableFifoMode(&UartHandle) != HAL_OK)
  {
		printf("Error HAL_UARTEx_DisableFifoMode\n");

  }
}
#endif
void UartMcuDeInit( Uart_t *obj )
{
    if( obj->UartId == UART_USB_CDC )
    {
#if defined( USE_USB_CDC )
        UartUsbDeInit( obj );
#endif
    }
    else
    {
        __HAL_RCC_USART_FORCE_RESET( );
        __HAL_RCC_USART_RELEASE_RESET( );
        __HAL_RCC_USART_CLK_DISABLE( );

        GpioInit( &obj->Tx, obj->Tx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
        GpioInit( &obj->Rx, obj->Rx.pin, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    }
}

uint8_t UartMcuPutChar( Uart_t *obj, uint8_t data )
{
    if( obj->UartId == UART_USB_CDC )
    {
#if defined( USE_USB_CDC )
        return UartUsbPutChar( obj, data );
#else
        return 255; // Not supported
#endif
    }
    else
    {
        CRITICAL_SECTION_BEGIN( );
        TxData = data;

        if( IsFifoFull( &obj->FifoTx ) == false )
        {
            FifoPush( &obj->FifoTx, TxData );

            // Trig UART Tx interrupt to start sending the FIFO contents.
            __HAL_UART_ENABLE_IT( &UartHandle, UART_IT_TC );

            CRITICAL_SECTION_END( );
            return 0; // OK
        }
        CRITICAL_SECTION_END( );
        return 1; // Busy
    }
}

uint8_t UartMcuGetChar( Uart_t *obj, uint8_t *data )
{
    if( obj->UartId == UART_USB_CDC )
    {
#if defined( USE_USB_CDC )
        return UartUsbGetChar( obj, data );
#else
        return 255; // Not supported
#endif
    }
    else
    {
        CRITICAL_SECTION_BEGIN( );

        if( IsFifoEmpty( &obj->FifoRx ) == false )
        {
            *data = FifoPop( &obj->FifoRx );
            CRITICAL_SECTION_END( );
            return 0;
        }
        CRITICAL_SECTION_END( );
        return 1;
    }
}

uint8_t UartMcuPutBuffer( Uart_t *obj, uint8_t *buffer, uint16_t size )
{
    if( obj->UartId == UART_USB_CDC )
    {
#if defined( USE_USB_CDC )
        return UartUsbPutBuffer( obj, buffer, size );
#else
        return 255; // Not supported
#endif
    }
    else
    {
        uint8_t retryCount;
        uint16_t i;

        for( i = 0; i < size; i++ )
        {
            retryCount = 0;
            while( UartPutChar( obj, buffer[i] ) != 0 )
            {
                retryCount++;

                // Exit if something goes terribly wrong
                if( retryCount > TX_BUFFER_RETRY_COUNT )
                {
                    return 1; // Error
                }
            }
        }
        return 0; // OK
    }
}

uint8_t UartMcuGetBuffer( Uart_t *obj, uint8_t *buffer, uint16_t size, uint16_t *nbReadBytes )
{
    uint16_t localSize = 0;

    while( localSize < size )
    {
        if( UartGetChar( obj, buffer + localSize ) == 0 )
        {
            localSize++;
        }
        else
        {
            break;
        }
    }

    *nbReadBytes = localSize;

    if( localSize == 0 )
    {
        return 1; // Empty
    }
    return 0; // OK
}

void HAL_UART_TxCpltCallback( UART_HandleTypeDef *handle )
{
    if( IsFifoEmpty( &Uart1.FifoTx ) == false )
    {
        TxData = FifoPop( &Uart1.FifoTx );
        //  Write one byte to the transmit data register
        if(HAL_UART_Transmit_IT( &UartHandle, &TxData, 1 ) != HAL_OK){
            printf("error\n");
        }
    }

    if( Uart1.IrqNotify != NULL )
    {
        Uart1.IrqNotify( UART_NOTIFY_TX );
    }
}

void HAL_UART_RxCpltCallback( UART_HandleTypeDef *handle )
{
    if( IsFifoFull( &Uart1.FifoRx ) == false )
    {
        // Read one byte from the receive data register
        FifoPush( &Uart1.FifoRx, RxData );
    }

    if( Uart1.IrqNotify != NULL )
    {
        Uart1.IrqNotify( UART_NOTIFY_RX );
    }

    HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
}

void HAL_UART_ErrorCallback( UART_HandleTypeDef *handle )
{
    HAL_UART_Receive_IT( &UartHandle, &RxData, 1 );
}

void USART1_IRQHandler( void )
{
    HAL_UART_IRQHandler( &UartHandle );
}