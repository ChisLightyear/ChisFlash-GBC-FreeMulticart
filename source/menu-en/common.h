#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <gb/gb.h>

// 内存映射地址声明
extern volatile uint8_t* ROM_BANK_LO_ADDR;    // 映射ROM bank[7:0]
extern volatile uint8_t* ROM_BANK_HI_ADDR;    // 映射ROM bank[8]
extern volatile uint8_t* RAM_BANK_ADDR;       // 映射RAM bank[4:0]
extern volatile uint8_t* GAME_EN_ADDR;        // 合卡使能寄存器
extern volatile uint8_t* GAME_SEL_ADDR;       // 程序选择寄存器
extern volatile uint8_t* ROM_INFO_ADDR;       // 程序信息地址
extern volatile uint8_t* MBC_TYPE_ADDR;       // MBC 类型地址
extern volatile uint8_t* ROM_SIZE_ADDR;       // ROM 大小地址
extern volatile uint8_t* RAM_SIZE_ADDR;       // RAM 大小地址
extern volatile uint8_t* GB_HRAM_ADDR;        // HRAM 空间
extern volatile uint8_t* GB_WRAM_Hi_ADDR;     // WRAM 高段空间


void switch_rom_bank(uint16_t full_bank) NONBANKED; // 切换程序bank
void switch_ram_bank(uint8_t ram_bank) NONBANKED; // 切换RAM bank
void enable_ram_access(void) NONBANKED; // 启用SRAM访问
void disable_ram_access(void) NONBANKED; // 禁用SRAM访问
void erase_sector(uint32_t sector_address); // 擦除单扇区
void erase_nor_range(uint32_t start_addr, uint32_t total_size, uint32_t sector_size); // 擦除容量
unsigned char* get_rom_name(uint16_t start_addr); // 获取程序名

#endif
