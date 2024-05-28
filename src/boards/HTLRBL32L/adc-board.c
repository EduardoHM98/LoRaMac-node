/*!
 * \file      adc-board.c
 *
 * \brief     Target board ADC driver implementation
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
 */
#include <stdio.h>
#include "bluenrg_lpx.h"
#include "rf_driver_hal_adc.h"
#include "rf_driver_hal_rcc.h"
#include "board-config.h"
#include "adc-board.h"


ADC_HandleTypeDef AdcHandle;
ADC_ConfigChannelTypeDef xChannel;

void AdcMcuInit( Adc_t *obj, PinNames adcInput )
{
     AdcHandle.Instance = ADC;
    
    __HAL_RCC_ADCDIG_CLK_ENABLE();
    __HAL_RCC_ADCANA_CLK_ENABLE();

    HAL_ADC_DeInit( &AdcHandle );

    if( adcInput != NC )
    {
        GpioInit( &obj->AdcInput, adcInput, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    } 
}

void AdcMcuConfig( void )
{
    // Configure ADC
    AdcHandle.Instance = ADC;
    AdcHandle.Init.DataRatio = ADC_DS_RATIO_128;
    AdcHandle.Init.DataWidth = ADC_DS_DATA_WIDTH_16_BIT;
    AdcHandle.Init.InvertOutputBitMode = ADC_INVERT_OUTBIT_SING;
    AdcHandle.Init.OverrunMode = ADC_NEW_DATA_IS_KEPT;
    AdcHandle.Init.SampleRate = ADC_SAMPLE_RATE_28;
    AdcHandle.Init.SamplingMode = ADC_SAMPLING_AT_END;
    AdcHandle.Init.SequenceLength = ADC_SEQ_LEN_01;
    HAL_ADC_Init( &AdcHandle ); 

    //channel config
    xChannel.ChannelType = ADC_CH_BATTERY_LEVEL_DETECTOR;
    xChannel.SequenceNumber = ADC_SEQ_POS_01;
    xChannel.VoltRange = ADC_VIN_RANGE_3V6;
    HAL_ADC_ConfigChannel(&AdcHandle, &xChannel);

}

uint16_t AdcMcuReadChannel( Adc_t *obj, uint32_t channel )
{
    uint16_t adcValue = 0;

    return adcValue;
 }
