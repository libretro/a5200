#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include "config.h"
#include "atari.h"

#define CART_NONE		 0
#define CART_5200_32	 4
#define CART_5200_EE_16	 6
#define CART_5200_40	 7
#define CART_5200_NS_16	16
#define CART_5200_8		19
#define CART_5200_4		20
#define CART_5200_64    71
#define CART_5200_128   72
#define CART_5200_256   73
#define CART_5200_512   74

#define CART_5200_40_ALT	99 

#define CART_CANT_OPEN    -1 /* Can't open cartridge image file */
#define CART_BAD_FORMAT   -2 /* Unknown cartridge format */

#define CART_MAX_SIZE (1024 * 1024)

struct cart_info_t
{
	const char *md5;
	int type;
	int keys_debounced;
	float joy_digital_range;
	float joy_analog_range;
	const char *name;
};

extern struct cart_info_t cart_info;

int CART_Insert(const uint8_t *data, size_t size);
void CART_Remove(void);
void CART_Start(void);

UBYTE CART_GetByte(UWORD addr);
void CART_PutByte(UWORD addr, UBYTE byte);
void CART_BountyBob1(UWORD addr);
void CART_BountyBob2(UWORD addr);

/* addr must be $bfxx in 5200 mode only. */
UBYTE CART_5200SuperCartGetByte(UWORD addr);

/* addr must be $bfxx in 5200 mode only. */
void CART_5200SuperCartPutByte(UWORD addr, UBYTE value);

#endif /* _CARTRIDGE_H_ */
