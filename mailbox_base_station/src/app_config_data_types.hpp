/*
 * app_config_data_types.hpp
 *
 *  Created on: Mar 10, 2016
 *      Author: Matthew
 */

#ifndef APP_CONFIG_DATA_TYPES_HPP_
#define APP_CONFIG_DATA_TYPES_HPP_


typedef struct {
	uint32_t	frequency;		//Frequench

	union {
		struct {
			uint8_t	codingRate:4;	//1=4/5, 2=4/6, 3=4/7,  4=4/8
			uint8_t crcEnable:1;
			uint8_t fixLength:1;	//0=variable, 1=fixed
			uint8_t fshhEnable:1;
			uint8_t iqInversionEnable:1;
		} lora;
		uint32_t 	Val;
	} conf;

	uint16_t		rxTimeout;		//RX Timeout in ms
	uint8_t			bw;				//bandwidth, a LORA_BW_XX define
									//0=7.8 1=10.4 2=15.6 3=20.8 4=31.25 5=41.7 6=62.5 7=125 8=250 9=500 kHz
	uint8_t			sf;				//Speading factor, a value from 7 to 12
	uint8_t			power;			//Output power in dBm, a value from 1 to 20
	uint8_t			preambleLength;	//Length of preamble, default = 5
	uint8_t			numberSymHop;
	uint8_t			symbolTimeout;

} AppConfig;


typedef struct {
    union {
        struct {
            uint8_t running         :1;
            uint8_t rxedPacket      :1;
            uint8_t updateDisplay   :1;

        } bit;
        uint32_t    Val;
    } flags;

    uint16_t    rxOK;           //Successful packets received
    uint16_t    rxErr;          //Error packets received
    uint16_t    rxCount;        //Count received packets

    int16_t     RssiValue;      //RSSI value of last reception, or 0xfff if receive timeout
    int16_t     RssiValueSlave; //RSSI value of slave
    int8_t      SnrValue;       //Signal to noise ratio
    uint8_t     boardType;      //The board type, a BOARD_XXX define
} AppData;



#endif /* APP_CONFIG_DATA_TYPES_HPP_ */
