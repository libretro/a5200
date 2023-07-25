#ifndef MZPOKEYSND_H_
#define MZPOKEYSND_H_

#include "atari.h"

int MZPOKEYSND_Init(uint32_t freq17,
                        int playback_freq,
                        UBYTE num_pokeys,
                        int flags,
                        int quality
                       );

#endif /* MZPOKEYSND_H_ */
