/**
Bit and byte order analysis:

From "HDX Animal Identification protocol description" doc

01111110000101111011101100101011110000010000000101111100000000000000000111100111010010000111111100000
01111110  -> Header

00010111  -> 11101000
10111011  -> 11011101
00101011  -> 11010100
11000001  -> 10000011
000000..  -> 0b00000010000011110101001101110111101000 = 2211765736
......01  -> 10        ->
01111100  -> 00111110  -> 0b0011111010 = 250
00000000
00000001  -> animal application indicator
11100111
01001000  -> CRC16
01111111

************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "esp_timer.h"

#include "soc/gpio_reg.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


#define FSK_SIGNAL_INPUT_IO      (19) // Define the input for FSK signal
#define FSK_PROBE_OUTPUT_IO      (21)


#define GPIO_Set(x)             REG_WRITE(GPIO_OUT_W1TS_REG, 1<<x)
#define GPIO_Clear(x)           REG_WRITE(GPIO_OUT_W1TC_REG, 1<<x)
#define GPIO_IN_Read(x)         REG_READ(GPIO_IN_REG) & (1<<x)


volatile uint8_t FSK_DoDecode;
volatile uint32_t Period;
volatile uint8_t FSK_HeaderDetected;

#define FSK_BUFF_SIZE   256
#define FSK_BUFF_HEADER_RANGE   100
uint8_t FSK_BitBuffer[FSK_BUFF_SIZE];
volatile int BitIdx;
#define FSK_DATA_SIZE   112

uint16_t Zeros;
uint16_t Ones;

uint16_t ValidData;
uint8_t DataBlock[10];


inline uint32_t IRAM_ATTR clocks(){
    uint32_t ccount;
    asm volatile ( "rsr %0, ccount" : "=a" (ccount) );
    return ccount;
}

//uint32_t DBG_Buff[200];


void FSK_DecodeEnable(void){
    BitIdx = 0;
    FSK_DoDecode = 1;
}

void FSK_DecodeDisable(void){
    FSK_DoDecode = 0;
}

#define CRC16_MASK_CCITT          0x1021  // CRC-CCITT mask (ISO 3309, used in X25, HDLC)
#define CRC16_MASK_ISO_11785      0x8408  // ISO 11785 animal tags
#define CRC16_MASK_CRC16          0xA001  // standard CRC16 mask (used in ARC files)


// 16 bit generic crc
unsigned int __crc16(unsigned int crc, uint8_t *data, unsigned int length, unsigned int polynomial)
{
    unsigned int i, j, c;

    for(i= 0 ; i <  length ; ++i)
    {
        c= (unsigned int) data[i];
        c <<= 8;
        for(j= 0 ; j < 8 ; ++j)
        {
           if ((crc ^ c) & 0x8000)
               crc= (crc << 1) ^ polynomial;
           else
               crc <<= 1;
           c <<= 1;
        }
        crc &= 0xffff;
    }
    return crc;
}


// ccitt standard crc
unsigned int crc_ccitt(uint8_t *data, unsigned int length)
{
    return __crc16(0, data, length, CRC16_MASK_CCITT);
}


int isHeaderfound( uint8_t *data ){
    //printf("data[0] = %d\n", data[0] );
    if ( data[0] != 0 )
        return 0;
    if ( data[1] != 1 )
        return 0;
    if ( data[2] != 1 )
        return 0;
    if ( data[3] != 1 )
        return 0;
    if ( data[4] != 1 )
        return 0;
    if ( data[5] != 1 )
        return 0;
    if ( data[6] != 1 )
        return 0;
    if ( data[7] != 0 )
        return 0;
    return 1;
}



void FSK_DumpBuffer(void){
    int i;
    GPIO_Set(FSK_PROBE_OUTPUT_IO);
    //printf("Period = %lu\n", Period);
    //for ( i=0 ; i<120 ; i++ ){
    //    printf("%u", (unsigned short)FSK_BitBuffer[i] );
    //}
    //printf("\n");

    // Search header '01111110'
    uint8_t *pBitBuffer = FSK_BitBuffer;

    ValidData = 0;
    for ( i=0 ; i<BitIdx ; i++ ){
        if ( isHeaderfound( pBitBuffer ) ){
            ValidData = 1;
            break;
        }
        pBitBuffer++;
    }

    if ( ValidData ){
        uint8_t *ptr = DataBlock;
        uint8_t Byte;

        // check CRC
        // calculate crc for 64 bits of data 8 individual bytes
        pBitBuffer += 8; // Skip header
        for ( i=0 ; i<(64+16) ; i+=8 ){  // 64 data bit + 16bit CRC
            Byte = 0;
            for ( int bit=0 ; bit<8 ; bit++ ){
                //printf("%d",  pBitBuffer[i+8+bit] );
                Byte <<= 1;
                if ( pBitBuffer[i+bit] == 1 ){
                     Byte |= 1L;
                }
            }
            //printf("Byte = %02X\n", Byte );
            *ptr++ = Byte;
        }
        unsigned int x  = crc_ccitt(DataBlock, 10);
        if ( x != 0 ){  // Compare against 0 because buffer includes CRC (10 Bytes)
            printf("CRC error (x=%02X)\n", x );
            ValidData = 0;
        }
    }

    if ( ValidData ){
        // extract Data fields
        uint64_t Id64 = 0ULL;
        int i;
        for ( i=0 ; i<38 ; i++ ){
            if ( pBitBuffer[0] == 1 )
                Id64 |= (1ULL<<i);
            pBitBuffer++;
        }
        printf("National code = %llu -- ", Id64 );

        // extract Country code
        uint32_t Code = 0UL;
        for ( i=0 ; i<10 ; i++ ){
            if ( pBitBuffer[0] == 1 )
                Code |= (1UL<<i);
            pBitBuffer++;
        }
        printf("Country code = %lu\n", Code );
    }

    if ( ValidData == 0 ){
        printf("No valid data found \n" );
    }

    GPIO_Clear(FSK_PROBE_OUTPUT_IO);
}



void FSK_DecoderSetup(void){

    gpio_set_direction(FSK_PROBE_OUTPUT_IO, GPIO_MODE_OUTPUT);

    gpio_set_direction(FSK_SIGNAL_INPUT_IO, GPIO_MODE_INPUT);

}

/*------------------------------------------------------------------
This task runs on core 1 in a long critical section:

------------------------------------------------------------------*/
void IRAM_ATTR FskDecoderLoop( void* p) {

   printf("Start FSK decoder on Core 1\n");

    while( 1 ){
        uint32_t t, t0;
        register int level, prevlevel;
        register uint32_t sample;
         int i;

        // Enter a critical section (somewhat long)
        portDISABLE_INTERRUPTS();

        t0 = clocks();
        prevlevel = 0;
        while( FSK_DoDecode != 0 ) {
            level = GPIO_IN_Read(FSK_SIGNAL_INPUT_IO);
            if ( (level != prevlevel) && (prevlevel==0) )  {
                t = clocks();
                //GPIO_Set(FSK_PROBE_OUTPUT_IO);
                //GPIO_Clear(FSK_PROBE_OUTPUT_IO);

                sample = t - t0;
                //if ( BitIdx < 200 ) DBG_Buff[BitIdx++] = sample;

#if 0
                if ( sample > 1240 ){
                    GPIO_Set(FSK_PROBE_OUTPUT_IO);
                    if ( BitIdx < FSK_BUFF_SIZE ) FSK_BitBuffer[BitIdx++] = 1;
                }
                else{
                    GPIO_Clear(FSK_PROBE_OUTPUT_IO);
                    if ( BitIdx < FSK_BUFF_SIZE ) FSK_BitBuffer[BitIdx++] = 0;
                }
#else
                if ( sample > 1240 ){
                    //GPIO_Set(FSK_PROBE_OUTPUT_IO);
                    Ones++;
                }
                else{
                    //GPIO_Clear(FSK_PROBE_OUTPUT_IO);
                    if ( Ones==0 ){
                        Zeros++;
                    }
                    else{
                        GPIO_Set(FSK_PROBE_OUTPUT_IO);
                        // Store the sequence to bit buffer
                        // Divide by 16 (Nb of clock cycles)
                        for ( i=Zeros+8 ; i>=16 ; i-=16 ){
                            FSK_BitBuffer[BitIdx++]=0;
                        }
                        for ( i=Ones+8 ; i>=16 ; i-=16 ){
                            FSK_BitBuffer[BitIdx++]=1;
                        }
                        // next sequence
                        Zeros = 1;
                        Ones  = 0;
                        GPIO_Clear(FSK_PROBE_OUTPUT_IO);
                    }
                    if ( BitIdx > (FSK_DATA_SIZE+50) ){ // TODO
                        break;
                    }
                }
#endif

                t0 = t;
            }
            prevlevel = level;
        }

        portENABLE_INTERRUPTS();
    }
}
