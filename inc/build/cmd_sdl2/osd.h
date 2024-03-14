
#ifndef _OSD_H_
#define _OSD_H_

#include "build/cmd_sdl2/main.h"
#include "build/cmd_sdl2/config.h"
#include "build/cmd_sdl2/error.h"
#include "build/cmd_sdl2/fileio.h"

#define osd_input_update sdl_input_update

#define GG_ROM      "./ggenie.bin"
#define AR_ROM      "./areplay.bin"
#define SK_ROM      "./sk.bin"
#define SK_UPMEM    "./sk2chip.bin"
#define CD_BIOS_US  "./bios_CD_U.bin"
#define CD_BIOS_EU  "./bios_CD_E.bin"
#define CD_BIOS_JP  "./bios_CD_J.bin"
#define MD_BIOS     "./bios_MD.bin"
#define MS_BIOS_US  "./bios_U.sms"
#define MS_BIOS_EU  "./bios_E.sms"
#define MS_BIOS_JP  "./bios_J.sms"
#define GG_BIOS     "./bios.gg"

unsigned int crc32(unsigned int crc, const unsigned char* buffer, unsigned int len);

#endif /* _OSD_H_ */
