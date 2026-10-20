#include <gb/gb.h>
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include <gbdk/console.h>
#include "ass/sound.h"
#include <gbdk/console.h>



// 切换ROM bank函数（要完整bank号）
void switch_rom_bank(uint16_t full_bank) NONBANKED {
    uint8_t rom_bank_high = (full_bank >> 8) & 0xFF;
    uint8_t rom_bank_low = full_bank & 0xFF;
    *ROM_BANK_LO_ADDR = rom_bank_low;
    *ROM_BANK_HI_ADDR = rom_bank_high;
}  

// 启用SRAM访问
void enable_ram_access(void) NONBANKED {
    // 向0x0000地址写入0x0A启用SRAM访问
    *((volatile uint8_t *)0x0000) = 0x0A;  // MBC5只关心低位是A
}

// 禁用SRAM访问
void disable_ram_access(void) NONBANKED {
    // 向0x0000地址写入0x00禁用SRAM访问
    *((volatile uint8_t *)0x0000) = 0x00;
}

// 切换RAM bank函数（要完整bank号）
void switch_ram_bank(uint8_t ram_bank) NONBANKED {
    // 仅切换RAM bank，不处理SRAM访问权限
    // SRAM访问权限应通过enable_ram_access和disable_ram_access函数单独控制
    *RAM_BANK_ADDR = ram_bank;
}  
