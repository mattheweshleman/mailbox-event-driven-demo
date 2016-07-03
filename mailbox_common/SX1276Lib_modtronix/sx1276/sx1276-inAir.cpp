/*

Description: -

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainers: Miguel Luis, Gregory Cristian and Nicolas Huguenin
*/
#include "myDebug.h"
#include "sx1276-inAir.h"

const RadioRegisters_t SX1276inAir::RadioRegsInit[] = 
{                                                 
    { MODEM_FSK , REG_LNA                , 0x23 },
    { MODEM_FSK , REG_RXCONFIG           , 0x1E },
    { MODEM_FSK , REG_RSSICONFIG         , 0xD2 },
    { MODEM_FSK , REG_PREAMBLEDETECT     , 0xAA },
    { MODEM_FSK , REG_OSC                , 0x07 },
    { MODEM_FSK , REG_SYNCCONFIG         , 0x12 },
    { MODEM_FSK , REG_SYNCVALUE1         , 0xC1 },
    { MODEM_FSK , REG_SYNCVALUE2         , 0x94 },
    { MODEM_FSK , REG_SYNCVALUE3         , 0xC1 },
    { MODEM_FSK , REG_PACKETCONFIG1      , 0xD8 },
    { MODEM_FSK , REG_FIFOTHRESH         , 0x8F },
    { MODEM_FSK , REG_IMAGECAL           , 0x02 },
    { MODEM_FSK , REG_DIOMAPPING1        , 0x00 },
    { MODEM_FSK , REG_DIOMAPPING2        , 0x30 },
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },  
};

SX1276inAir::SX1276inAir( void ( *txDone )( ), void ( *txTimeout ) ( ), void ( *rxDone ) ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ), 
                            void ( *rxTimeout ) ( ), void ( *rxError ) ( ), void ( *fhssChangeChannel ) ( uint8_t channelIndex ), void ( *cadDone ) ( bool ChannelActivityDetected ),
                            PinName mosi, PinName miso, PinName sclk, PinName nss, PinName reset,
                            PinName dio0, PinName dio1, PinName dio2, PinName dio3, PinName dio4, PinName dio5,
                            PinName antSwitch )
                            : SX1276( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone, mosi, miso, sclk, nss, reset, dio0, dio1, dio2, dio3, dio4, dio5),
                            antSwitch( NC ),
                            fake( NC )
{
    Reset( );
    
    boardConnected = BOARD_UNKNOWN;
    DetectBoardType( );
    
    RxChainCalibration( );
    
    IoInit( );
    
    SetOpMode( RF_OPMODE_SLEEP );
    
    IoIrqInit( dioIrq );
    
    RadioRegistersInit( );

    SetModem( MODEM_FSK );

    this->settings.State = IDLE ;
}

SX1276inAir::SX1276inAir( void ( *txDone )( ), void ( *txTimeout ) ( ), void ( *rxDone ) ( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ), 
                            void ( *rxTimeout ) ( ), void ( *rxError ) ( ), void ( *fhssChangeChannel ) ( uint8_t channelIndex ), void ( *cadDone ) ( bool ChannelActivityDetected ) ) 
                        #if( (defined (TARGET_NUCLEO_L152RE) || defined (TARGET_NUCLEO_F401RE)) && defined(SHIELD_SHD3I_INAIR9))
                        //For SHD3I with BOARD_INAIR4 in imod3, on Nucleo board
                        :   SX1276( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone,
                                    D11/*MOSI*/, D12/*MISO*/, D13/*SCLK*/, D7/*CS*/, A5/*RST*/, D2/*DIO0*/, D8, D4, A4, PC_3/*FAKE!!!*/, D3/*DIO5*/ ),
                        #elif( defined (TARGET_K64F) && defined(SHIELD_SHD3I_INAIR9) ) /* Use pin PTC0 as fake pin */
                        //For SHD3I with BOARD_INAIR4 in imod3, on FRDM-K64F board
                        :   SX1276( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone,
                                    D11/*MOSI*/, D12/*MISO*/, D13/*SCLK*/, D7/*CS*/, A5/*RST*/, D2/*DIO0*/, D8, D4, A4, PTC0/*FAKE!!!*/, D3/*DIO5*/ ),
                        #elif( defined (TARGET_KL25Z) && defined(SHIELD_SHD3I_INAIR9) ) /* Use pin PTD7 as fake pin - only A & D pins have interrupts */
                        //For SHD3I with BOARD_INAIR4 in imod3, on FRDM-KL25Z board
                        :   SX1276( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone,
                                    D11/*MOSI*/, D12/*MISO*/, D13/*SCLK*/, D7/*CS*/, A5/*RST*/, D2/*DIO0*/, D8, D4, A4, PTD7/*FAKE!!!*/, D3/*DIO5*/ ),
                        #elif ( defined ( TARGET_NZ32ST1L ) )
                        //For NZ32ST1L board with BOARD_INAIR4 in imod2
                        :   SX1276( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone,
                                    PB_5/*MOSI*/, PB_4/*MISO*/, PB_3/*SCLK*/, PC_8/*CS*/, PA_9/*RST*/, PB_0/*DIO0*/, PB_1, PC_6, PA_10, PC_3/*FAKE!!!*/, PC_13/*DIO5*/ ),
                        #else
                        :   SX1276( txDone, txTimeout, rxDone, rxTimeout, rxError, fhssChangeChannel, cadDone, D11, D12, D13, D10, A0, D2, D3, D4, D5, D8, D9 ),
                        #endif
                            antSwitch( NC ),
                            fake( NC )

{
    Reset( );
    
    boardConnected = BOARD_UNKNOWN;
    DetectBoardType( );
    
    RxChainCalibration( );
    
    IoInit( );
    
    SetOpMode( RF_OPMODE_SLEEP );
    IoIrqInit( dioIrq );
    
    RadioRegistersInit( );

    SetModem( MODEM_FSK );

    this->settings.State = IDLE ;
}

//-------------------------------------------------------------------------
//                      Board relative functions
//-------------------------------------------------------------------------
uint8_t SX1276inAir::DetectBoardType( void )
{
	//Detect board type NOT possible with inAir4, inAir9 and inAir9B boards. User has to call
	//the SetBoardType() function to set the board type!
	return ( boardConnected );
}

void SX1276inAir::IoInit( void )
{
    AntSwInit( );
    SpiInit( );
}

void SX1276inAir::RadioRegistersInit( ){
    uint8_t i = 0;
    for( i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        SetModem( RadioRegsInit[i].Modem );
        Write( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }    
}

void SX1276inAir::SpiInit( void )
{
    nss = 1;    
    spi.format( 8,0 );   
    //uint32_t frequencyToSet = 8000000;
    // TEST TEST
    uint32_t frequencyToSet = 1000000;   //DJH - Reduced speed to 1MHz
    #if( defined ( TARGET_NUCLEO_L152RE ) || defined (TARGET_NUCLEO_F401RE) || defined ( TARGET_LPC11U6X ) || defined (TARGET_K64F) || defined ( TARGET_NZ32ST1L ) )
        spi.frequency( frequencyToSet );
    #elif( defined ( TARGET_KL25Z ) ) //busclock frequency is halved -> double the spi frequency to compensate
        spi.frequency( frequencyToSet * 2 );
    #else
        #warning "Check the board's SPI frequency"
    #endif
    wait(0.1); 
}

void SX1276inAir::IoIrqInit( DioIrqHandler *irqHandlers )
{
    //TARGET_KL25Z board does not have pulldown resistors, seems like TARGET_K64F does have them
    #if( defined ( TARGET_NUCLEO_L152RE )  || defined (TARGET_NUCLEO_F401RE) ||  defined ( TARGET_LPC11U6X ) || defined (TARGET_K64F) || defined ( TARGET_NZ32ST1L ))
        dio0.mode(PullDown);
        dio1.mode(PullDown);   
        dio2.mode(PullDown);
        dio3.mode(PullDown); 
        dio4.mode(PullDown); 
    #endif
    dio0.rise( this, static_cast< TriggerInAir > ( irqHandlers[0] ) );
    dio1.rise( this, static_cast< TriggerInAir > ( irqHandlers[1] ) );
    dio2.rise( this, static_cast< TriggerInAir > ( irqHandlers[2] ) );
    //For SHD3I with BOARD_INAIR4 in imod3, on FRDM-KL25Z board. It uses A4 on FRDM-KL25Z board, which does not have interrupt
    #if( defined ( TARGET_KL25Z ) && defined(SHIELD_SHD3I_INAIR9) )
    //Nothing to be done
    #else
    dio3.rise( this, static_cast< TriggerInAir > ( irqHandlers[3] ) );
    #endif
    dio4.rise( this, static_cast< TriggerInAir > ( irqHandlers[4] ) );
}

void SX1276inAir::IoDeInit( void )
{
    //nothing
}

uint8_t SX1276inAir::GetPaSelect( uint32_t channel )
{
	if(boardConnected == BOARD_INAIR9B) {
		return RF_PACONFIG_PASELECT_PABOOST;
	}
	else {
		return RF_PACONFIG_PASELECT_RFO;
	}
}

void SX1276inAir::SetAntSwLowPower( bool status )
{
    if( isRadioActive != status )
    {
        isRadioActive = status;
    
        if( status == false )
        {
            AntSwInit( );
        }
        else
        {
            AntSwDeInit( );
        }
    }
}

void SX1276inAir::AntSwInit( void )
{
    //antSwitch = 0;
}

void SX1276inAir::AntSwDeInit( void )
{
    //antSwitch = 0;
}

void SX1276inAir::SetAntSw( uint8_t rxTx )
{
    if( this->rxTx == rxTx )
    {
        //no need to go further
        return;
    }

    this->rxTx = rxTx;

//    if( rxTx != 0 )
//    {
//        antSwitch = 1;
//    }
//    else
//    {
//        antSwitch = 0;
//    }
}

bool SX1276inAir::CheckRfFrequency( uint32_t frequency )
{
    //TODO: Implement check, currently all frequencies are supported
    return true;
}


void SX1276inAir::Reset( void )
{
    reset.output();
    reset = 0;
    wait_ms( 1 );
    reset.input();
    wait_ms( 6 );
}
    
void SX1276inAir::Write( uint8_t addr, uint8_t data )
{
    Write( addr, &data, 1 );
}

uint8_t SX1276inAir::Read( uint8_t addr )
{
    uint8_t data;
    Read( addr, &data, 1 );
    return data;
}

void SX1276inAir::Write( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    nss = 0;
    spi.write( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        spi.write( buffer[i] );
    }
    nss = 1;
}

void SX1276inAir::Read( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    nss = 0;
    spi.write( addr & 0x7F );
    for( i = 0; i < size; i++ )
    {
        buffer[i] = spi.write( 0 );
    }
    nss = 1;
}

void SX1276inAir::WriteFifo( uint8_t *buffer, uint8_t size )
{
    Write( 0, buffer, size );
}

void SX1276inAir::ReadFifo( uint8_t *buffer, uint8_t size )
{
    Read( 0, buffer, size );
}
