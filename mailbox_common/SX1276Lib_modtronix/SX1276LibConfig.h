/*
Description: - Library Configuration

*/
#ifndef __SX1276LIBCONFIG_H__
#define __SX1276LIBCONFIG_H__

//Use the SHD3I shield, with the BOARD_INAIR4 module assembled in iMod port 3. The following pins are used:
//SCLK=D13, MISO=D12, MOSI=D11
//DIO0=D2, DIO1=D8, DIO2=D4, DIO3=A4, DIO5=D3  DIO4=Not Used
//NSS(CS)=D7, Reset=A5
#ifndef SHIELD_SHD3I_INAIR9
#define SHIELD_SHD3I_INAIR9
#endif

//This library uses Arduino pins, which are not available on TARGET_NZ32ST1L
#if ( defined ( TARGET_NZ32ST1L ) )
#define A0 NC
#define A3 NC
#define A4 NC
#define D0 NC
#define D1 NC
#define D2 NC
#define D3 NC
#define D4 NC
#define D5 NC
#define D6 NC
#define D7 NC
#define D8 NC
#define D9 NC
#define D10 NC
#define D11 NC
#define D12 NC
#define D13 NC
#endif

#endif //__SX1276LIBCONFIG_H__
