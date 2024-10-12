#include "ezflash.h"
#include "tonc.h"

// Inspired by Afksa
// https://github.com/afska/gba-flashcartio
#define BOOTLOADER_PAGE_SECTION 0x8000
#define PSRAM_PAGE_SECTION 0x200
#define S98WS512PE0_FLASH_PAGE_MAX 0x200
#define ROM_HEADER_CHECKSUM *(volatile unsigned short*)(0x8000000 + 188)

#undef EWRAM_CODE
#define EWRAM_CODE __attribute__((section(".ewram.ezflash")))

EWRAM_CODE void KnockKernelForPage(unsigned short page) {
    *(volatile unsigned short *) 0x9fe0000 = 0xd200;
    *(volatile unsigned short *) 0x8000000 = 0x1500;
    *(volatile unsigned short *) 0x8020000 = 0xd200;
    *(volatile unsigned short *) 0x8040000 = 0x1500;
    *(volatile unsigned short *) 0x9880000 = page;
    *(volatile unsigned short *) 0x9fc0000 = 0x1500;
}

// Credit: Lorenzooone
EWRAM_CODE void EnableOdeRtc() {
    *(volatile unsigned short *) 0x9FE0000 = 0xD200;
    *(volatile unsigned short *) 0x8000000 = 0x1500;
    *(volatile unsigned short *) 0x8020000 = 0xD200;
    *(volatile unsigned short *) 0x8040000 = 0x1500;
    *(volatile unsigned short *) 0x9880000 = 0x8002; //Kernel mode
    *(volatile unsigned short *) 0x9FC0000 = 0x1500;
    *(volatile unsigned short *) 0x9FE0000 = 0xD200;
    *(volatile unsigned short *) 0x8000000 = 0x1500;
    *(volatile unsigned short *) 0x8020000 = 0xD200;
    *(volatile unsigned short *) 0x8040000 = 0x1500;
    *(volatile unsigned short *) 0x96A0000 = 0x0001; // Enable clock
    *(volatile unsigned short *) 0x9FC0000 = 0x1500;
}

EWRAM_CODE bool probeRom(unsigned short expected, unsigned short page) {
    KnockKernelForPage(page);
    if (expected != ROM_HEADER_CHECKSUM) return false;
    return true;
}

EWRAM_CODE bool detect() {
    unsigned short savedImeValue = REG_IME;
    REG_IME = 0;
    const unsigned short checksumCompliment = ROM_HEADER_CHECKSUM;

    if (probeRom(checksumCompliment, BOOTLOADER_PAGE_SECTION)) {
        REG_IME = savedImeValue;
        return false;
    }

    if (probeRom(checksumCompliment, PSRAM_PAGE_SECTION)) {
        REG_IME = savedImeValue;
        return true;
    }

    for (int i = 0; i < S98WS512PE0_FLASH_PAGE_MAX; i++) {
        if (!probeRom(checksumCompliment, i)) continue;
        REG_IME = savedImeValue;
        return true;
    }

    REG_IME = savedImeValue;
    return false; // Hardware has failed out of spec
}
