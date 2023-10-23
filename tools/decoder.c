#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/** python3 plot.py

plot.py:

import numpy  as np
import matplotlib.pyplot as plt
data = np.loadtxt('data.txt')


x = data[:, 0]
y = data[:, 1]
plt.plot(x, y,'r--')
plt.show()


********************************************/

#define FSK_BUFF_SIZE           256
#define FSK_BUFF_HEADER_RANGE   100
uint8_t FSK_BitBuffer[FSK_BUFF_SIZE];
volatile int BitIdx;
#define FSK_DATA_SIZE   112

uint16_t Zeros;
uint16_t Ones;

uint16_t ValidData;

#define CRC16_MASK_CCITT          0x1021  // CRC-CCITT mask (ISO 3309, used in X25, HDLC)
#define CRC16_MASK_ISO_11785      0x8408  // ISO 11785 animal tags
#define CRC16_MASK_CRC16          0xA001  // standard CRC16 mask (used in ARC files)


//https://forum.arduino.cc/t/how-to-calculate-crc-ccitt-from-iso-fdx-b-microchip-data/445418/8?page=2
uint8_t DataBlock[10];

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


unsigned int crc_11785(uint8_t *data, unsigned int length)
{
    return __crc16(0, data, length, CRC16_MASK_ISO_11785);
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

int main( int argc, char argv[] ){
    char *pName = "raw_data.txt";
    char Line[1600];
    int i, l;
    char * ptr;
    FILE * fIn;
    FILE * fView;
    uint32_t sample;

    fIn = fopen ( pName, "r" );
    if ( fIn == NULL ){
        printf( "File not found: %s\n", pName );
        exit(1);
    }
    fView = fopen("data.txt", "w" );

    for (l=0 ; l<5 ; l++){
        fgets(Line, 1600, fIn);
        ptr = Line;
        Zeros = 0;
        Ones  = 0;
        BitIdx=0;
        for (i=0 ; i<1500 ; i++){
            //printf("%c", *ptr++ );
            sample = (uint32_t)*ptr++;

            if ( sample == '1' ){
                Ones++;
            }
            else{
                if ( Ones==0 ){
                    Zeros++;
                }
                else{
                    // Store the sequence to bit buffer
                    // Divide by 16 (Nb of clock cycles)
                    for ( i=Zeros+8 ; i>=16 ; i-=16 ){
                        //FSK_BitBuffer[BitIdx++]=0;
                        printf("0");
                        FSK_BitBuffer[BitIdx++]=0;
                    }
                    for ( i=Ones+8 ; i>=16 ; i-=16 ){
                        //FSK_BitBuffer[BitIdx++]=1;
                        printf("1");
                        FSK_BitBuffer[BitIdx++]=1;
                    }
                    // next sequence
                    Zeros = 1;
                    Ones  = 0;
                }
                if ( BitIdx > (FSK_DATA_SIZE+50) ){ // TODO
                    break;
                }
            }
        }
        printf("\n");

        // Search header '01111110'
        uint8_t *pBitBuffer = FSK_BitBuffer;
        ValidData = 0;
        for ( i=0 ; i<FSK_BUFF_HEADER_RANGE ; i++ ){
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
            printf("Id64 = %lu\n", Id64 );

            // extract Country code
            uint32_t Code = 0UL;
            for ( i=0 ; i<10 ; i++ ){
                if ( pBitBuffer[0] == 1 )
                    Code |= (1UL<<i);
                pBitBuffer++;
            }
            printf("Code = %u\n", Code );
        }
