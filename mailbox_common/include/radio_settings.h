/*
 * radio_settings.h
 *
 * Header to hold radio settings which must be
 * consistent between the base station build and the
 * sensor build.
 *
 *  Created on: Apr 23, 2016
 *      Author: Matthew Eshleman
 */

#ifndef RADIO_SETTINGS_H_
#define RADIO_SETTINGS_H_


#define LORA_BW_7800	0	//7.8kHz
#define LORA_BW_10400	1	//10.4kHz
#define LORA_BW_15600	2	//15.6kHz
#define LORA_BW_20800	3	//20.8kHz
#define LORA_BW_31250	4	//31.25kHz
#define LORA_BW_41700	5	//41.7kHz
#define LORA_BW_62500	6	//62.5kHz
#define LORA_BW_125000	7	//125kHz
#define LORA_BW_250000	8	//250kHz
#define LORA_BW_500000	9	//500kHz


// DEFINES ////////////////////////////////////////////////////////////////////
/* Set this flag to '1' to use the LoRa modulation or to '0' to use FSK modulation */
#define USE_MODEM_LORA  1
#define USE_MODEM_FSK   !USE_MODEM_LORA

#define RF_FREQUENCY                                916700000 // 916.7 kHz
//#define RF_FREQUENCY                                868700000 // 868.7 kHz
//#define RF_FREQUENCY                                433700000 // 433.7 kHz
#define TX_OUTPUT_POWER                             14        // 14 dBm. Max 14 for inAir4 and inAir9, and 20(17) for inAir9B
#if USE_MODEM_LORA == 1
#define LORA_BANDWIDTH                              LORA_BW_500000
#define LORA_SPREADING_FACTOR                       12		// SF7..SF12
#define LORA_CODINGRATE                             1       // 1=4/5, 2=4/6, 3=4/7, 4=4/8
#define LORA_PREAMBLE_LENGTH                        8       // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5       // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_FHSS_ENABLED                           false
#define LORA_NB_SYMB_HOP                            4
#define LORA_IQ_INVERSION_ON                        false
#define LORA_CRC_ENABLED                            false
#elif USE_MODEM_FSK == 1
#define FSK_FDEV                                    25000     // Hz
#define FSK_DATARATE                                19200     // bps
#define FSK_BANDWIDTH                               50000     // Hz
#define FSK_AFC_BANDWIDTH                           83333     // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false
#define FSK_CRC_ENABLED                             true

#else
#error "Please define a modem in the compiler options."
#endif




#endif /* RADIO_SETTINGS_H_ */
