
#ifndef FSK_DECODER_H_
#define FSK_DECODER_H_

void FSK_DecodeEnable(void);

void FSK_DecodeDisable(void);

void FSK_DumpBuffer(void);

void FSK_DecoderSetup(void);

void IRAM_ATTR FskDecoderLoop( void* p);


#endif // FSK_DECODER_H_
