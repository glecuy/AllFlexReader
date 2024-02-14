
#ifndef BT_CLASSIC_SPP_H_
#define BT_CLASSIC_SPP_H_

typedef void (* bt_classic_cb_t)(const uint8_t *buffer, int size);

void bt_classic_setup(void);

bool bt_classic_send( uint8_t * pData, uint16_t len );

bool bt_classic_spp_open(void);

void bt_classic_register_RX_cb( bt_classic_cb_t cb);

#endif // BT_CLASSIC_SPP_H_
