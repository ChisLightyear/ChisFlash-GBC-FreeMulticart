#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include "ass/arrow.c"
#include <gb/hardware.h>
#include <gb/drawing.h>
#include <string.h>
#include "ass/sound.h"
#include <gbdk/console.h>
#include <gbdk/font.h>
#include <stdint.h>
#include "common.h"

#ifndef MENU_BUILD_ZH
#define MENU_BUILD_ZH 0
#endif

#if MENU_BUILD_ZH
#define MENU_BUILD_EN 0
#else
#define MENU_BUILD_EN 1
#endif

// MBC type information
typedef struct {
    uint8_t code;        // 类型代码
    const char* main_type; // 主要类型名称
    const char* full_desc; // 完整描述
} MbcInfo;

// ROM大小信息结构�?
typedef struct {
    uint8_t code;        // 类型代码
    const char* desc;    // 描述字符�?
} RomSizeInfo;

// RAM大小信息结构�?
typedef struct {
    uint8_t code;        // 类型代码
    const char* desc;    // 描述字符�?
} RamSizeInfo;

typedef struct {
    uint8_t cart_save_mode;
    uint8_t ui_language;
    uint16_t rom_count;
} FixedMenuConfig;

// 函数原型声明
void turn_off_lcd(void);
void move_arrow(int direction);
void into_game(void);
const char* get_mbc_type_str(uint8_t mbc_type);
#if MENU_BUILD_EN
const char* get_mbc_full_description(uint8_t mbc_type);
#endif
const char* get_rom_size_str(uint8_t rom_size); 
const char* get_ram_size_str(uint8_t ram_size); 
void show_detail_page(void);                     
void return_to_main_menu(void);                 
void clear_screen(uint8_t start_y, uint8_t end_y);
void display_page(uint8_t page);
#if MENU_BUILD_EN
void read_game_name(char* game_name, uint16_t rom_bank);
#endif
uint8_t load_fixed_directory(void);

static uint8_t game_uses_sram_map(uint8_t index);
#if MENU_BUILD_ZH
static uint8_t load_game_name_codes(uint8_t index, uint16_t* out_codes, uint8_t max_codes);
static uint8_t load_font_glyph_bitmap(uint16_t codepoint, uint8_t* out_bitmap);
static void display_page_chinese(uint8_t page);
static void show_detail_page_chinese(void);
static void draw_ui_code_text(const uint16_t* codes, uint8_t code_count, uint8_t x, uint8_t y);
static void draw_ui_ascii_text(uint8_t x, uint8_t y, const char* text);
static void draw_ui_ascii_value(uint8_t x, uint8_t y, const char* value);
static void draw_ui_page_text(uint8_t x, uint8_t y, uint8_t current, uint8_t total);
static void show_loading_screen_chinese(void);
static void show_no_config_screen_chinese(void);
static void handle_main_menu_input_chinese(uint8_t keys);
static void handle_detail_page_input_chinese(uint8_t keys);
static void run_main_chinese(void);
static void draw_scaled_ascii_char(uint8_t x, uint8_t y, char chr, uint8_t size_px);
static void draw_scaled_ascii_text(uint8_t x, uint8_t y, const char* text, uint8_t size_px);
static void draw_scaled_glyph_bitmap(uint8_t x, uint8_t y, const uint8_t* glyph_bitmap, uint8_t size_px);
static void draw_name_code_slice_scaled(const uint16_t* codes, uint8_t code_count, uint8_t visible_chars, uint8_t x, uint8_t y, uint8_t size_px, uint8_t char_gap_px);
#endif
#if MENU_BUILD_EN
static void display_page_english(uint8_t page);
static void show_detail_page_english(void);
static void return_to_main_menu_english(void);
static void show_loading_screen_english(void);
static void show_no_config_screen_english(void);
static void handle_main_menu_input_english(uint8_t keys);
static void handle_detail_page_input_english(uint8_t keys);
static void run_main_english(void);
static uint8_t load_game_english_name(uint8_t index, char* out_name, uint8_t max_chars);
static void load_game_menu_name_ascii(uint8_t index, char* out_name, uint8_t max_chars);
void update_scroll(void);
#endif
static uint8_t use_chinese_menu_layout(void);
static uint8_t get_menu_games_per_page(void);
static uint8_t get_games_on_page(uint8_t page);
static uint8_t get_menu_row_top_px(uint8_t row);
static uint8_t get_menu_arrow_y_px(uint8_t row);
static uint8_t get_detail_page_step(void);
static void move_arrow_wrapped(int direction);
static void change_page_safe(int direction);
static void change_detail_selection_by_step(int direction);
static void sync_menu_selection_from_game_index(void);

// 全局常数

#define GAMES_PER_PAGE 16   // 每页显示的程序数�?

#define DELAY_PRESS 60      // 按键后延�?
#define ARROW_START_X 16    // 箭头起始X坐标�? * 4 = 32，向右移4个字符）
#define ARROW_START_Y 24    // 箭头起始Y坐标
#define ARROW_Y_OFFSET 8    // 箭头Y轴偏移量
#define DETAILS_OFFSET_X 1  // 信息页整体X偏移�?
#define VERSION_NUM "1.04"  // 版本�?
#define PAGE_STEP 5        // 上下键切换程序的步长

// 全局常量定义
#define MAX_GAMES 99                // 支持的最大程序数�?
#define GAME_NAME_LEN 17            // 程序名称长度
#define MAX_SRAM_BANKS 64
#define MAX_GAME_SRAM_BANKS 8U

// 当前状态，数字代表处于不同状�?
#define MAIN_MENU_STATE 0     // 主菜单状�?
#define DETAIL_PAGE_STATE 1   // 详细信息页状�?
uint16_t GameROMBanks[MAX_GAMES];  // 动态存储有效程序的bank地址 
uint8_t GameMBCs[MAX_GAMES];       // 存储每个程序的MBC类型
uint8_t GameHasBattery[MAX_GAMES]; // 固定目录中的电池标记
uint8_t GameRAMBankMaps[MAX_GAMES][MAX_GAME_SRAM_BANKS]; // 每个程序的sram映射（bank ID�?
uint8_t GameRAMBanks[MAX_GAMES]; // 存储每个程序的RAM数量
uint8_t GameRAMSizes[MAX_GAMES]; // 存储每个程序的原始RAM大小代码
uint8_t GameROMSizes[MAX_GAMES]; // 存储每个程序的原始ROM大小代码
uint16_t GameROMBanksNeeded[MAX_GAMES]; // 存储每个程序所需的ROM bank数量
uint8_t GAME_COUNT = 0;          // 实际检测到的程序数�?

uint8_t TOTAL_PAGES = 0;         // 总页数（根据实际程序数计算）
uint8_t game_index = 0;         // 当前选中程序索引
int arrow_index = 0;        // 箭头显示索引

uint8_t current_page = 0;   // 当前页码
uint8_t current_state = MAIN_MENU_STATE; // 当前状�?
// 新增全局变量用于滚动控制
int8_t scroll_offset = 0;      // 当前滚动偏移�?
uint8_t scroll_timer = 0;       // 滚动计时�?
uint8_t scroll_speed = 2;      // 滚动速度（帧数，越少越快�?
uint8_t scroll_delay = 60;
const char* scroll_text = NULL; // 需要滚动的文本
int8_t scroll_text_length = 0; // 文本长度
uint8_t scroll_enabled = 0;     // 是否启用滚动
// 全局滚动状态变�?
uint8_t scroll_state = 0;     // 0:向右滚动到末�?1:暂停在末�?2:向左滚动到开�?3:暂停在开�?
uint8_t pause_timer = 0;      // 暂停计时�?
// 读取MBC类型
#define FIXED_MAP_CONFIG_OFFSET 0x020000UL
#define FIXED_MAP_ROM_DIR_OFFSET 0x040000UL
#define FIXED_MAP_FONT_OFFSET 0x0C0000UL
#define FIXED_MAP_HEADER_SIZE 64U
#define FIXED_MAP_ROM_ENTRY_SIZE 128U
#define FIXED_MAP_ROM_NAME_CODE_OFFSET 14U
#define FIXED_MAP_ROM_NAME_CODE_COUNT 48U
#define FIXED_MAP_ROM_NAME_ASCII_LEN 16U
#define FIXED_MAP_ROM_SRAM_MAP_OFFSET 110U
#define FIXED_MAP_MAX_LINEAR_BANK 0x07FFU
#define FIXED_MAP_FLAG_VALID 0x01U
#define FIXED_FONT_CANVAS_PX 8U
#define FIXED_FONT_GLYPH_BYTES 8U
#define FIXED_FONT_TABLE_ENTRY_SIZE 4U
#define MENU_LANG_EN 0U
#define MENU_LANG_ZH 1U
#define CHINESE_MENU_NAME_X 40U
#define CHINESE_DETAIL_NAME_X 8U
#define CHINESE_MENU_VISIBLE_CHARS 15U
#define CHINESE_DETAIL_VISIBLE_CHARS 18U
#define CHINESE_MENU_TITLE_WIDTH_PX 112U
#define CHINESE_DETAIL_TITLE_WIDTH_PX 144U
#define MENU_RUNTIME_ROM_BANK 1U

#ifndef CHINESE_DISPLAY_FONT_PX
#define CHINESE_DISPLAY_FONT_PX 12
#endif

#if (CHINESE_DISPLAY_FONT_PX != 12)
#error "Chinese menu build is 12px only."
#endif

#define CHINESE_MENU_FONT_PX 12U
#define CHINESE_DETAIL_FONT_PX 12U
#define CHINESE_MENU_ROW_STEP_PX 12U
#define CHINESE_MENU_GAMES_PER_PAGE 10U
#define CHINESE_MENU_CHAR_GAP_PX 1U
#define CHINESE_DETAIL_CHAR_GAP_PX 1U

uint8_t mbc_type = 0;
uint32_t GameEntryAddrs[MAX_GAMES];
FixedMenuConfig gMenuConfig = { 0, MENU_BUILD_ZH ? MENU_LANG_ZH : MENU_LANG_EN, 0 };
#if MENU_BUILD_ZH
uint8_t gAsciiGlyphRows[95][8];
uint8_t gAsciiGlyphReady = 0;
uint8_t gFixedFontReady = 0;
uint16_t gFixedFontGlyphCount = 0;
uint16_t gFixedFontGlyphBytes = 0;
uint16_t gFixedFontCanvasWidth = 0;
uint32_t gFixedFontTableBase = 0;
uint32_t gFixedFontBitmapBase = 0;
#endif


// 内存映射地址定义
volatile uint8_t* ROM_BANK_LO_ADDR = (volatile uint8_t *)0x2000;    // 映射ROM bank[7:0]，写�?x00不会被自动转换成 0x01. 
volatile uint8_t* ROM_BANK_HI_ADDR = (volatile uint8_t *)0x3000;    // 映射ROM bank[8]，只使用输入数据A0�?                
volatile uint8_t* RAM_BANK_ADDR = (volatile uint8_t *)0x4000;   // 映射RAM bank[4:0]                                   
volatile uint8_t* GAME_EN_ADDR = (volatile uint8_t *)0xa000;    // 合卡使能寄存�?                                     
volatile uint8_t* GAME_SEL_ADDR = (volatile uint8_t *)0xb000;   // 程序选择寄存�?                                     
volatile uint8_t* ROM_INFO_ADDR = (volatile uint8_t *)0x4134;   // 程序信息地址                                        
volatile uint8_t* MBC_TYPE_ADDR = (volatile uint8_t *)0x4147;   // MBC 类型地址                                         
volatile uint8_t* ROM_SIZE_ADDR = (volatile uint8_t *)0x4148;   // ROM 大小地址                                         
volatile uint8_t* RAM_SIZE_ADDR = (volatile uint8_t *)0x4149;   // RAM 大小地址        
volatile uint8_t* GB_HRAM_ADDR = (volatile uint8_t *)0xFF80;    // HRAM 空间                                          
volatile uint8_t* GB_WRAM_Hi_ADDR = (volatile uint8_t *)0xD000; // WRAM 高段空间


// =============================================================================
// 存档拼接与管理结构体
// =============================================================================

// 头部结构体（固定63字节�?
static uint16_t read_le16(const uint8_t* ptr) {
    return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

static uint32_t read_le32(const uint8_t* ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

static uint8_t nor_read_linear_byte(uint32_t linear_addr) {
    uint16_t bank = (uint16_t)(linear_addr >> 14);
    uint16_t offset = (uint16_t)(linear_addr & 0x3FFFU);

    if (bank == 0U) {
        return *((volatile uint8_t*)offset);
    }

    switch_rom_bank(bank);
    return *((volatile uint8_t*)(0x4000U + offset));
}

static void nor_read_linear_bytes(uint32_t linear_addr, uint8_t* buffer, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = nor_read_linear_byte(linear_addr + i);
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
}

static uint8_t rom_size_code_from_bank_count(uint16_t rom_bank_count) {
    switch (rom_bank_count) {
        case 2:   return 0x00;
        case 4:   return 0x01;
        case 8:   return 0x02;
        case 16:  return 0x03;
        case 32:  return 0x04;
        case 64:  return 0x05;
        case 128: return 0x06;
        case 256: return 0x07;
        case 512: return 0x08;
        default:  return 0x09;
    }
}

static uint8_t ram_size_code_from_bank_count(uint8_t ram_bank_count) {
    switch (ram_bank_count) {
        case 0: return 0x00;
        case 1: return 0x02;
        case 4: return 0x03;
        case 8: return 0x05;
        case 16: return 0x04;
        default: return 0x06;
    }
}

static void clear_fixed_directory_cache(void) {
    memset(GameROMBanks, 0, sizeof(GameROMBanks));
    memset(GameMBCs, 0, sizeof(GameMBCs));
    memset(GameHasBattery, 0, sizeof(GameHasBattery));
    memset(GameRAMBankMaps, 0xFF, sizeof(GameRAMBankMaps));
    memset(GameRAMBanks, 0, sizeof(GameRAMBanks));
    memset(GameRAMSizes, 0, sizeof(GameRAMSizes));
    memset(GameROMSizes, 0, sizeof(GameROMSizes));
    memset(GameROMBanksNeeded, 0, sizeof(GameROMBanksNeeded));
    memset(GameEntryAddrs, 0, sizeof(GameEntryAddrs));
    GAME_COUNT = 0;
    TOTAL_PAGES = 0;
    current_page = 0;
    game_index = 0;
    arrow_index = 0;
    gMenuConfig.cart_save_mode = 0;
    gMenuConfig.ui_language = MENU_BUILD_ZH ? MENU_LANG_ZH : MENU_LANG_EN;
    gMenuConfig.rom_count = 0;
#if MENU_BUILD_ZH
    gFixedFontReady = 0;
    gFixedFontGlyphCount = 0;
    gFixedFontGlyphBytes = 0;
    gFixedFontCanvasWidth = 0;
    gFixedFontTableBase = 0;
    gFixedFontBitmapBase = 0;
#endif
}

static uint8_t use_chinese_menu_layout(void) {
    return MENU_BUILD_ZH;
}

static uint8_t get_menu_games_per_page(void) {
    if (use_chinese_menu_layout()) {
        return CHINESE_MENU_GAMES_PER_PAGE;
    }
    return GAMES_PER_PAGE;
}

static uint8_t get_games_on_page(uint8_t page) {
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t start_index = (uint8_t)(page * games_per_page);

    if (start_index >= GAME_COUNT) {
        return 0U;
    }

    if ((uint16_t)start_index + games_per_page > GAME_COUNT) {
        return (uint8_t)(GAME_COUNT - start_index);
    }

    return games_per_page;
}

static uint8_t get_menu_row_top_px(uint8_t row) {
    if (use_chinese_menu_layout()) {
        return (uint8_t)(8U + row * CHINESE_MENU_ROW_STEP_PX);
    }
    return (uint8_t)((row + 1U) * 8U);
}

static uint8_t get_menu_arrow_y_px(uint8_t row) {
    if (use_chinese_menu_layout()) {
        return (uint8_t)(16U + get_menu_row_top_px(row));
    }
    return (uint8_t)(ARROW_START_Y + row * ARROW_Y_OFFSET);
}

static uint8_t get_detail_page_step(void) {
    if (use_chinese_menu_layout()) {
        return get_menu_games_per_page();
    }
    return PAGE_STEP;
}

static void move_arrow_wrapped(int direction) {
    int new_index = arrow_index + direction;
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t games_on_current_page = get_games_on_page(current_page);

    if (GAME_COUNT == 0U || games_on_current_page == 0U) {
        return;
    }

    if (new_index >= 0 && new_index < games_on_current_page) {
        arrow_index = new_index;
        game_index = (uint8_t)(current_page * games_per_page + (uint8_t)arrow_index);
    } else if (direction > 0 && (uint8_t)(current_page + 1U) < TOTAL_PAGES) {
        current_page++;
        arrow_index = 0;
        game_index = (uint8_t)(current_page * games_per_page);
        display_page(current_page);
    } else if (direction < 0 && current_page > 0U) {
        current_page--;
        games_on_current_page = get_games_on_page(current_page);
        if (games_on_current_page == 0U) {
            arrow_index = 0;
            game_index = (uint8_t)(current_page * games_per_page);
        } else {
            arrow_index = (int)(games_on_current_page - 1U);
            game_index = (uint8_t)(current_page * games_per_page + games_on_current_page - 1U);
        }
        display_page(current_page);
    } else {
        return;
    }

        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
    play_move_sound();
    delay(DELAY_PRESS);
}

static void change_page_safe(int direction) {
    static uint8_t saved_arrow_idx = 0;
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t games_on_this_page;
    int8_t new_page;

    if (GAME_COUNT == 0U || TOTAL_PAGES == 0U) {
        return;
    }

    saved_arrow_idx = (uint8_t)arrow_index;
    new_page = (int8_t)current_page + direction;
    if (new_page < 0 || (uint8_t)new_page >= TOTAL_PAGES) {
        return;
    }

    current_page = (uint8_t)new_page;
    games_on_this_page = get_games_on_page(current_page);
    if (games_on_this_page == 0U) {
        return;
    }

    if (saved_arrow_idx >= games_on_this_page) {
        arrow_index = (int)(games_on_this_page - 1U);
    } else {
        arrow_index = (int)saved_arrow_idx;
    }

    game_index = (uint8_t)(current_page * games_per_page + (uint8_t)arrow_index);
    display_page(current_page);
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
    play_page_sound();
    delay(DELAY_PRESS * 2);
}

static void change_detail_selection_by_step(int direction) {
    uint8_t detail_step = get_detail_page_step();
    uint16_t new_index;

    if (GAME_COUNT == 0U) {
        return;
    }

    if (direction < 0) {
        new_index = (uint16_t)game_index + GAME_COUNT - (detail_step % GAME_COUNT);
        game_index = (uint8_t)(new_index % GAME_COUNT);
    } else {
        new_index = (uint16_t)game_index + detail_step;
        game_index = (uint8_t)(new_index % GAME_COUNT);
    }

    sync_menu_selection_from_game_index();
    play_move_sound();
    show_detail_page();
    delay(DELAY_PRESS);
}

static void sync_menu_selection_from_game_index(void) {
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t games_on_page;

    if (GAME_COUNT == 0U || games_per_page == 0U) {
        current_page = 0U;
        arrow_index = 0;
        game_index = 0U;
        return;
    }

    if (game_index >= GAME_COUNT) {
        game_index = (uint8_t)(GAME_COUNT - 1U);
    }

    current_page = (uint8_t)(game_index / games_per_page);
    arrow_index = (int)(game_index % games_per_page);
    games_on_page = get_games_on_page(current_page);
    if (games_on_page == 0U) {
        arrow_index = 0;
    } else if ((uint8_t)arrow_index >= games_on_page) {
        arrow_index = (int)(games_on_page - 1U);
        game_index = (uint8_t)(current_page * games_per_page + (uint8_t)arrow_index);
    }
}

static uint8_t game_uses_sram_map(uint8_t index) {
    return (index < MAX_GAMES) && (GameRAMBanks[index] > 0U);
}

uint8_t load_fixed_directory(void) {
    uint8_t config_header[FIXED_MAP_HEADER_SIZE];
    uint8_t dir_header[FIXED_MAP_HEADER_SIZE];
    uint8_t entry_data[FIXED_MAP_ROM_ENTRY_SIZE];
    uint16_t entry_count;
    uint16_t entry_size;
    uint32_t entry_base;

    clear_fixed_directory_cache();
    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET, config_header, FIXED_MAP_HEADER_SIZE);
    if (memcmp(config_header, "MBCXCFG", 7) != 0) {
        switch_rom_bank(MENU_RUNTIME_ROM_BANK);
        return 0;
    }

    gMenuConfig.cart_save_mode = config_header[10];
    gMenuConfig.ui_language = MENU_BUILD_ZH ? MENU_LANG_ZH : MENU_LANG_EN;
    gMenuConfig.rom_count = read_le16(config_header + 14);

    nor_read_linear_bytes(FIXED_MAP_ROM_DIR_OFFSET, dir_header, FIXED_MAP_HEADER_SIZE);
    if (memcmp(dir_header, "MBCXROM", 7) != 0) {
        switch_rom_bank(MENU_RUNTIME_ROM_BANK);
        return 0;
    }

    entry_count = read_le16(dir_header + 10);
    entry_size = read_le16(dir_header + 12);
    entry_base = read_le32(dir_header + 16);

    if (entry_size < 14U || entry_size > FIXED_MAP_ROM_ENTRY_SIZE) {
        entry_size = FIXED_MAP_ROM_ENTRY_SIZE;
    }
    if (entry_base < FIXED_MAP_ROM_DIR_OFFSET || entry_base >= FIXED_MAP_FONT_OFFSET) {
        entry_base = FIXED_MAP_ROM_DIR_OFFSET + FIXED_MAP_HEADER_SIZE;
    }
    if (entry_count > MAX_GAMES) {
        entry_count = MAX_GAMES;
    }

    for (uint16_t i = 0; i < entry_count; i++) {
        uint32_t entry_addr = entry_base + (uint32_t)i * entry_size;
        uint32_t rom_offset_bank;
        uint16_t rom_bank_count;
        uint8_t ram_bank_count;
        uint8_t has_battery;

        memset(entry_data, 0xFF, sizeof(entry_data));
        nor_read_linear_bytes(entry_addr, entry_data, entry_size);

        if ((entry_data[13] & FIXED_MAP_FLAG_VALID) == 0U) {
            continue;
        }

        rom_offset_bank = read_le32(entry_data + 4);
        rom_bank_count = read_le16(entry_data + 8);
        ram_bank_count = entry_data[10];
        has_battery = entry_data[12];

        if (rom_offset_bank > FIXED_MAP_MAX_LINEAR_BANK || rom_bank_count == 0U) {
            continue;
        }

        GameROMBanks[GAME_COUNT] = (uint16_t)rom_offset_bank;
        GameROMBanksNeeded[GAME_COUNT] = rom_bank_count;
        GameRAMBanks[GAME_COUNT] = (ram_bank_count > MAX_GAME_SRAM_BANKS) ? MAX_GAME_SRAM_BANKS : ram_bank_count;
        GameMBCs[GAME_COUNT] = entry_data[11];
        GameHasBattery[GAME_COUNT] = has_battery ? 1U : 0U;
        GameROMSizes[GAME_COUNT] = rom_size_code_from_bank_count(rom_bank_count);
        GameRAMSizes[GAME_COUNT] = ram_size_code_from_bank_count(GameRAMBanks[GAME_COUNT]);
        GameEntryAddrs[GAME_COUNT] = entry_addr;

        for (uint8_t j = 0; j < MAX_GAME_SRAM_BANKS; j++) {
            GameRAMBankMaps[GAME_COUNT][j] = 0xFF;
        }
        if (game_uses_sram_map(GAME_COUNT)) {
            for (uint8_t j = 0; j < GameRAMBanks[GAME_COUNT]; j++) {
                uint8_t mapped_bank = entry_data[FIXED_MAP_ROM_SRAM_MAP_OFFSET + j];
                if (mapped_bank < MAX_SRAM_BANKS || mapped_bank == 0xFE || mapped_bank == 0xFF) {
                    GameRAMBankMaps[GAME_COUNT][j] = mapped_bank;
                }
            }
        }

        GAME_COUNT++;
    }

    gMenuConfig.rom_count = GAME_COUNT;
    if (GAME_COUNT > 0) {
        uint8_t games_per_page = get_menu_games_per_page();
        TOTAL_PAGES = (uint8_t)((GAME_COUNT + games_per_page - 1U) / games_per_page);
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return (GAME_COUNT > 0);
}

#if MENU_BUILD_ZH
static void ensure_ascii_glyph_cache(void) {
    uint8_t tile_data[16];

    if (gAsciiGlyphReady) {
        return;
    }

    mode(M_TEXT_OUT);
    gotoxy(0, 0);
    for (uint8_t code = 32; code <= 126; code++) {
        putchar((char)code);
        get_bkg_data(get_bkg_tile_xy(0, 0), 1, tile_data);
        for (uint8_t row = 0; row < 8; row++) {
            gAsciiGlyphRows[code - 32][row] = tile_data[row * 2] | tile_data[row * 2 + 1];
        }
    }
    putchar(' ');
    gAsciiGlyphReady = 1;
}

static void clear_drawing_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    uint8_t x2;
    uint8_t y2;

    if (width == 0U || height == 0U) {
        return;
    }

    x2 = (uint8_t)(x + width - 1U);
    y2 = (uint8_t)(y + height - 1U);
    color(WHITE, WHITE, SOLID);
    box(x, y, x2, y2, M_FILL);
    color(BLACK, WHITE, SOLID);
}

static void draw_ascii_char(uint8_t x, uint8_t y, char chr) {
    uint8_t glyph_index = (chr >= 32 && chr <= 126) ? (uint8_t)(chr - 32) : (uint8_t)('?' - 32);

    ensure_ascii_glyph_cache();
    for (uint8_t sy = 0; sy < 8; sy++) {
        uint8_t glyph_row = gAsciiGlyphRows[glyph_index][sy];
        for (uint8_t sx = 0; sx < 8; sx++) {
            if ((glyph_row & (0x80U >> sx)) != 0U) {
                box(x + sx, y + sy, x + sx, y + sy, M_FILL);
            }
        }
    }
}

static void draw_ascii_text(uint8_t x, uint8_t y, const char* text) {
    uint8_t draw_x = x;

    if (text == NULL) {
        return;
    }

    while (*text != '\0') {
        draw_ascii_char(draw_x, y, *text);
        draw_x = (uint8_t)(draw_x + 8U);
        text++;
    }
}

static void draw_scaled_ascii_char(uint8_t x, uint8_t y, char chr, uint8_t size_px) {
    uint8_t glyph_index = (chr >= 32 && chr <= 126) ? (uint8_t)(chr - 32) : (uint8_t)('?' - 32);

    ensure_ascii_glyph_cache();
    for (uint8_t sy = 0; sy < 8; sy++) {
        uint8_t glyph_row = gAsciiGlyphRows[glyph_index][sy];
        uint8_t y0 = (uint8_t)((sy * size_px) / 8U);
        uint8_t y1 = (uint8_t)((((uint16_t)(sy + 1U) * size_px) + 7U) / 8U);
        if (y1 <= y0) {
            y1 = y0 + 1U;
        }

        for (uint8_t sx = 0; sx < 8; sx++) {
            if ((glyph_row & (0x80U >> sx)) == 0U) {
                continue;
            }

            {
                uint8_t x0 = (uint8_t)((sx * size_px) / 8U);
                uint8_t x1 = (uint8_t)((((uint16_t)(sx + 1U) * size_px) + 7U) / 8U);
                if (x1 <= x0) {
                    x1 = x0 + 1U;
                }
                box(x + x0, y + y0, x + x1 - 1U, y + y1 - 1U, M_FILL);
            }
        }
    }
}

static void draw_scaled_ascii_text(uint8_t x, uint8_t y, const char* text, uint8_t size_px) {
    uint8_t draw_x = x;

    if (text == NULL) {
        return;
    }

    while (*text != '\0') {
        draw_scaled_ascii_char(draw_x, y, *text, size_px);
        draw_x = (uint8_t)(draw_x + size_px);
        text++;
    }
}

static void draw_fixed_glyph_bitmap(uint8_t x, uint8_t y, const uint8_t* glyph_bitmap) {
    for (uint8_t sy = 0; sy < FIXED_FONT_CANVAS_PX; sy++) {
        uint8_t row_bits = glyph_bitmap[sy];
        for (uint8_t sx = 0; sx < FIXED_FONT_CANVAS_PX; sx++) {
            if ((row_bits & (0x80U >> sx)) != 0U) {
                box(x + sx, y + sy, x + sx, y + sy, M_FILL);
            }
        }
    }
}

static void draw_scaled_glyph_bitmap(uint8_t x, uint8_t y, const uint8_t* glyph_bitmap, uint8_t size_px) {
    for (uint8_t sy = 0; sy < FIXED_FONT_CANVAS_PX; sy++) {
        uint8_t row_bits = glyph_bitmap[sy];
        uint8_t y0 = (uint8_t)((sy * size_px) / FIXED_FONT_CANVAS_PX);
        uint8_t y1 = (uint8_t)((((uint16_t)(sy + 1U) * size_px) + (FIXED_FONT_CANVAS_PX - 1U)) / FIXED_FONT_CANVAS_PX);
        if (y1 <= y0) {
            y1 = y0 + 1U;
        }

        for (uint8_t sx = 0; sx < FIXED_FONT_CANVAS_PX; sx++) {
            if ((row_bits & (0x80U >> sx)) == 0U) {
                continue;
            }

            {
                uint8_t x0 = (uint8_t)((sx * size_px) / FIXED_FONT_CANVAS_PX);
                uint8_t x1 = (uint8_t)((((uint16_t)(sx + 1U) * size_px) + (FIXED_FONT_CANVAS_PX - 1U)) / FIXED_FONT_CANVAS_PX);
                if (x1 <= x0) {
                    x1 = x0 + 1U;
                }
                box(x + x0, y + y0, x + x1 - 1U, y + y1 - 1U, M_FILL);
            }
        }
    }
}

static void draw_missing_glyph(uint8_t x, uint8_t y) {
    box(x, y, x + FIXED_FONT_CANVAS_PX - 1U, y + FIXED_FONT_CANVAS_PX - 1U, M_NOFILL);
}

static uint8_t load_game_name_codes(uint8_t index, uint16_t* out_codes, uint8_t max_codes) {
    uint8_t entry_data[FIXED_MAP_ROM_ENTRY_SIZE];
    uint8_t out_count = 0;

    if (index >= GAME_COUNT || out_codes == NULL || max_codes == 0U || GameEntryAddrs[index] == 0U) {
        return 0;
    }

    memset(entry_data, 0xFF, sizeof(entry_data));
    nor_read_linear_bytes(GameEntryAddrs[index], entry_data, FIXED_MAP_ROM_ENTRY_SIZE);

    for (uint8_t i = 0; i < FIXED_MAP_ROM_NAME_CODE_COUNT && out_count < max_codes; i++) {
        uint16_t code = read_le16(entry_data + FIXED_MAP_ROM_NAME_CODE_OFFSET + i * 2U);
        if (code == 0U) {
            break;
        }
        out_codes[out_count++] = code;
    }

    return out_count;
}
#endif

#if MENU_BUILD_EN
static uint8_t load_game_english_name(uint8_t index, char* out_name, uint8_t max_chars) {
    uint8_t entry_data[FIXED_MAP_ROM_ENTRY_SIZE];
    uint8_t out_count = 0;

    if (index >= GAME_COUNT || out_name == NULL || max_chars < 2U || GameEntryAddrs[index] == 0U) {
        return 0;
    }

    memset(entry_data, 0xFF, sizeof(entry_data));
    nor_read_linear_bytes(GameEntryAddrs[index], entry_data, FIXED_MAP_ROM_ENTRY_SIZE);

    for (uint8_t i = 0; i < FIXED_MAP_ROM_NAME_ASCII_LEN && out_count < (uint8_t)(max_chars - 1U); i++) {
        uint8_t ch = entry_data[FIXED_MAP_ROM_NAME_CODE_OFFSET + i];

        if (ch == 0U || ch == 0xFFU) {
            break;
        }

        out_name[out_count++] = (ch >= 0x20U && ch <= 0x7EU) ? (char)ch : '?';
    }

    out_name[out_count] = '\0';
    return out_count;
}

static void load_game_menu_name_ascii(uint8_t index, char* out_name, uint8_t max_chars) {
    if (out_name == NULL || max_chars == 0U) {
        return;
    }

    out_name[0] = '\0';
    if (load_game_english_name(index, out_name, max_chars) > 0U) {
        return;
    }

    if (index < GAME_COUNT && GameROMBanks[index] != 0U) {
        read_game_name(out_name, GameROMBanks[index]);
        return;
    }

    strcpy(out_name, "UNKNOWN");
}
#endif

#if MENU_BUILD_ZH
static uint8_t load_font_glyph_bitmap(uint16_t codepoint, uint8_t* out_bitmap) {
    uint8_t header[FIXED_MAP_HEADER_SIZE];
    uint8_t entry[FIXED_FONT_TABLE_ENTRY_SIZE];
    int16_t left = 0;
    int16_t right;

    if (codepoint < 0x80U || out_bitmap == NULL) {
        return 0;
    }

    if (gFixedFontReady == 0U) {
        nor_read_linear_bytes(FIXED_MAP_FONT_OFFSET, header, FIXED_MAP_HEADER_SIZE);
        if (memcmp(header, "MBCXFNT", 7) != 0) {
            return 0;
        }

        gFixedFontGlyphCount = read_le16(header + 10);
        gFixedFontCanvasWidth = read_le16(header + 12);
        gFixedFontGlyphBytes = read_le16(header + 14);
        gFixedFontTableBase = read_le32(header + 16);
        gFixedFontBitmapBase = read_le32(header + 20);
        gFixedFontReady = 1U;
    }

    if (gFixedFontGlyphCount == 0U ||
        gFixedFontCanvasWidth != FIXED_FONT_CANVAS_PX ||
        gFixedFontGlyphBytes < FIXED_FONT_GLYPH_BYTES) {
        return 0;
    }

    right = (int16_t)gFixedFontGlyphCount - 1;
    while (left <= right) {
        int16_t mid = (int16_t)((left + right) >> 1);
        nor_read_linear_bytes(gFixedFontTableBase + (uint32_t)mid * FIXED_FONT_TABLE_ENTRY_SIZE, entry, FIXED_FONT_TABLE_ENTRY_SIZE);

        {
            uint16_t mid_code = read_le16(entry);
            if (mid_code == codepoint) {
                uint16_t glyph_index = read_le16(entry + 2);
                nor_read_linear_bytes(gFixedFontBitmapBase + (uint32_t)glyph_index * gFixedFontGlyphBytes, out_bitmap, FIXED_FONT_GLYPH_BYTES);
                return 1;
            }
            if (mid_code < codepoint) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
    }

    return 0;
}

static void draw_name_code_slice(const uint16_t* codes, uint8_t code_count, uint8_t visible_chars, uint8_t x, uint8_t y, uint8_t char_gap_px) {
    uint8_t glyph_bitmap[FIXED_FONT_GLYPH_BYTES];
    uint8_t draw_count = 0;

    if (codes == NULL) {
        return;
    }

    while (draw_count < visible_chars && draw_count < code_count) {
        uint16_t code = codes[draw_count];
        uint8_t draw_x = (uint8_t)(x + draw_count * (FIXED_FONT_CANVAS_PX + char_gap_px));

        if (code < 0x80U) {
            draw_ascii_char(draw_x, y, (char)code);
        } else if (load_font_glyph_bitmap(code, glyph_bitmap)) {
            draw_fixed_glyph_bitmap(draw_x, y, glyph_bitmap);
        } else {
            draw_missing_glyph(draw_x, y);
        }

        draw_count++;
    }
}

static void display_page_chinese(uint8_t page) {
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t start_index = (uint8_t)(page * games_per_page);
    uint8_t end_index = start_index + games_per_page;
    static const uint16_t title_codes[] = {0x6E38, 0x620F, 0x5217, 0x8868};
    static const uint16_t empty_codes[] = {0x6682, 0x65E0, 0x6E38, 0x620F};
    static const uint16_t hint_codes[] = {0x8BF7, 0x70E7, 0x5F55, 0x5408, 0x5361, 0x83DC, 0x5355};

    mode(M_DRAWING);
    clear_drawing_rect(0, 0, 160U, 144U);
    draw_ui_code_text(title_codes, sizeof(title_codes) / sizeof(title_codes[0]), 8U, 4U);
    draw_ui_ascii_value(104U, 4U, VERSION_NUM);
    draw_ui_page_text(104U, 20U, (uint8_t)(page + 1U), TOTAL_PAGES);
    if (GAME_COUNT == 0 || TOTAL_PAGES == 0) {
        draw_ui_page_text(104U, 20U, 0U, 0U);
        draw_ui_code_text(empty_codes, sizeof(empty_codes) / sizeof(empty_codes[0]), 24U, 56U);
        draw_ui_code_text(hint_codes, sizeof(hint_codes) / sizeof(hint_codes[0]), 12U, 76U);
        wait_vbl_done();
        return;
    }

    if (end_index > GAME_COUNT) {
        end_index = GAME_COUNT;
    }

    for (uint8_t i = start_index; i < end_index; i++) {
        uint8_t row = (uint8_t)(i - start_index);
        uint8_t row_y = get_menu_row_top_px(row);
        char prefix[5];
        uint16_t codes[FIXED_MAP_ROM_NAME_CODE_COUNT];
        uint8_t code_count;
        uint8_t prefix_y = (uint8_t)(row_y + 2U);
        uint8_t visible_chars = (uint8_t)(CHINESE_MENU_TITLE_WIDTH_PX / (CHINESE_MENU_FONT_PX + CHINESE_MENU_CHAR_GAP_PX));

        clear_drawing_rect(0, row_y, 160U, CHINESE_MENU_FONT_PX);
        sprintf(prefix, "%02u.", i + 1U);
        draw_scaled_ascii_text(16U, prefix_y, prefix, 8U);

        code_count = load_game_name_codes(i, codes, FIXED_MAP_ROM_NAME_CODE_COUNT);
        if (code_count == 0U) {
            draw_scaled_ascii_text(CHINESE_MENU_NAME_X, row_y, "UNKNOWN", CHINESE_MENU_FONT_PX);
        } else {
            draw_name_code_slice_scaled(
                codes,
                code_count,
                visible_chars,
                CHINESE_MENU_NAME_X,
                row_y,
                CHINESE_MENU_FONT_PX,
                CHINESE_MENU_CHAR_GAP_PX);
        }
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    wait_vbl_done();
}

static void show_detail_page_chinese(void) {
    uint16_t bank_addr;
    uint8_t rom_size;
    uint8_t ram_size;
    const char* mbc_desc;
    const char* rom_size_desc;
    const char* ram_size_desc;
    char line[24];
    uint16_t codes[FIXED_MAP_ROM_NAME_CODE_COUNT];
    uint8_t code_count;
    static const uint16_t title_codes[] = {0x6E38, 0x620F, 0x8BE6, 0x60C5};
    static const uint16_t name_codes[] = {0x540D, 0x79F0};
    static const uint16_t mbc_codes[] = {0x7C7B, 0x578B};
    static const uint16_t rom_codes[] = {0x5BB9, 0x91CF};
    static const uint16_t ram_codes[] = {0x5B58, 0x6863};
    static const uint16_t bank_codes[] = {0x5730, 0x5740};
    static const uint16_t enter_codes[] = {0x8FDB, 0x5165};
    static const uint16_t back_codes[] = {0x8FD4, 0x56DE};

    HIDE_SPRITES;
    if (game_index >= GAME_COUNT) {
        play_cancel_sound();
        current_state = MAIN_MENU_STATE;
        return_to_main_menu();
        delay(DELAY_PRESS);
        return;
    }

    bank_addr = GameROMBanks[game_index];
    if (bank_addr == 0U) {
        play_cancel_sound();
        current_state = MAIN_MENU_STATE;
        return_to_main_menu();
        delay(DELAY_PRESS);
        return;
    }

    mbc_type = GameMBCs[game_index];
    rom_size = GameROMSizes[game_index];
    ram_size = GameRAMSizes[game_index];
    mbc_desc = get_mbc_type_str(mbc_type);
    rom_size_desc = get_rom_size_str(rom_size);
    ram_size_desc = get_ram_size_str(ram_size);
    scroll_enabled = 0;
    scroll_text = NULL;
    scroll_text_length = 0;
    scroll_offset = 0;
    scroll_timer = 0;
    scroll_state = 0;
    pause_timer = 0;

    mode(M_DRAWING);
    clear_drawing_rect(0, 0, 160U, 144U);
    draw_ui_code_text(title_codes, sizeof(title_codes) / sizeof(title_codes[0]), 8U, 4U);
    draw_ui_page_text(104U, 4U, (uint8_t)(game_index + 1U), GAME_COUNT);
    draw_ui_code_text(name_codes, sizeof(name_codes) / sizeof(name_codes[0]), 8U, 28U);

    code_count = load_game_name_codes(game_index, codes, FIXED_MAP_ROM_NAME_CODE_COUNT);
    if (code_count == 0U) {
        draw_scaled_ascii_text(CHINESE_DETAIL_NAME_X, 40U, "UNKNOWN", CHINESE_DETAIL_FONT_PX);
    } else {
        uint8_t visible_chars = (uint8_t)(CHINESE_DETAIL_TITLE_WIDTH_PX / (CHINESE_DETAIL_FONT_PX + CHINESE_DETAIL_CHAR_GAP_PX));
        draw_name_code_slice_scaled(
            codes,
            code_count,
            visible_chars,
            CHINESE_DETAIL_NAME_X,
            40U,
            CHINESE_DETAIL_FONT_PX,
            CHINESE_DETAIL_CHAR_GAP_PX);
    }

    draw_ui_code_text(mbc_codes, sizeof(mbc_codes) / sizeof(mbc_codes[0]), 8U, 60U);
    draw_ui_ascii_value(56U, 62U, (char*)mbc_desc);

    draw_ui_code_text(rom_codes, sizeof(rom_codes) / sizeof(rom_codes[0]), 8U, 76U);
    draw_ui_ascii_value(56U, 78U, rom_size_desc);

    draw_ui_code_text(ram_codes, sizeof(ram_codes) / sizeof(ram_codes[0]), 8U, 92U);
    draw_ui_ascii_value(56U, 94U, ram_size_desc);

    draw_ui_code_text(bank_codes, sizeof(bank_codes) / sizeof(bank_codes[0]), 8U, 108U);
    sprintf(line, "%u", bank_addr);
    draw_ui_ascii_value(56U, 110U, line);

    draw_ui_ascii_value(8U, 128U, "A");
    draw_ui_code_text(enter_codes, sizeof(enter_codes) / sizeof(enter_codes[0]), 20U, 126U);
    draw_ui_ascii_value(84U, 128U, "B");
    draw_ui_code_text(back_codes, sizeof(back_codes) / sizeof(back_codes[0]), 96U, 126U);

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    wait_vbl_done();
}

static void draw_name_code_slice_scaled(
    const uint16_t* codes,
    uint8_t code_count,
    uint8_t visible_chars,
    uint8_t x,
    uint8_t y,
    uint8_t size_px,
    uint8_t char_gap_px
) {
    uint8_t glyph_bitmap[FIXED_FONT_GLYPH_BYTES];
    uint8_t draw_count = 0;

    if (codes == NULL) {
        return;
    }

    while (draw_count < visible_chars && draw_count < code_count) {
        uint16_t code = codes[draw_count];
        uint8_t draw_x = (uint8_t)(x + draw_count * (size_px + char_gap_px));

        if (code < 0x80U) {
            draw_scaled_ascii_char(draw_x, y, (char)code, size_px);
        } else if (load_font_glyph_bitmap(code, glyph_bitmap)) {
            draw_scaled_glyph_bitmap(draw_x, y, glyph_bitmap, size_px);
        } else {
            box(draw_x, y, draw_x + size_px - 1U, y + size_px - 1U, M_NOFILL);
        }

        draw_count++;
    }
}

static void draw_ui_code_text(const uint16_t* codes, uint8_t code_count, uint8_t x, uint8_t y) {
    uint8_t visible_chars = (uint8_t)(CHINESE_DETAIL_TITLE_WIDTH_PX / (CHINESE_MENU_FONT_PX + CHINESE_MENU_CHAR_GAP_PX));
    draw_name_code_slice_scaled(
        codes,
        code_count,
        visible_chars,
        x,
        y,
        CHINESE_MENU_FONT_PX,
        CHINESE_MENU_CHAR_GAP_PX);
}

static void draw_ui_ascii_text(uint8_t x, uint8_t y, const char* text) {
    draw_scaled_ascii_text(x, y, text, CHINESE_MENU_FONT_PX);
}

static void draw_ui_ascii_value(uint8_t x, uint8_t y, const char* value) {
    draw_scaled_ascii_text(x, y, value, 8U);
}

static void draw_ui_page_text(uint8_t x, uint8_t y, uint8_t current, uint8_t total) {
    char page_text[16];
    sprintf(page_text, "%02u/%02u", current, total);
    draw_ui_ascii_value(x, y, page_text);
}

static void show_loading_screen_chinese(void) {
    mode(M_DRAWING);
    clear_drawing_rect(0, 0, 160U, 144U);
    draw_ui_ascii_text(28U, 56U, "LOADING");
    wait_vbl_done();
}

static void show_no_config_screen_chinese(void) {
    mode(M_DRAWING);
    clear_drawing_rect(0, 0, 160U, 144U);
    draw_ui_ascii_text(16U, 48U, "NO CONFIG");
    draw_ui_ascii_text(16U, 64U, "FLASH MENU ROM");
    wait_vbl_done();
}

static void handle_main_menu_input_chinese(uint8_t keys) {
    if (GAME_COUNT == 0U) {
        return;
    }

    if (keys & J_DOWN) {
        move_arrow_wrapped(1);
    } else if (keys & J_UP) {
        move_arrow_wrapped(-1);
    } else if (keys & J_LEFT) {
        change_page_safe(-1);
    } else if (keys & J_RIGHT) {
        change_page_safe(1);
    } else if (keys & J_A) {
        play_detail_sound();
        current_state = DETAIL_PAGE_STATE;
        show_detail_page_chinese();
        delay(DELAY_PRESS * 2);
    }
}

static void handle_detail_page_input_chinese(uint8_t keys) {
    if (keys & J_A) {
        play_confirm_sound();
        delay(DELAY_PRESS * 7);
        into_game();
    } else if (keys & J_B) {
        play_cancel_sound();
        current_state = MAIN_MENU_STATE;
        return_to_main_menu();
        delay(DELAY_PRESS);
    } else if (keys & J_LEFT) {
        if (game_index == 0U) {
            game_index = (uint8_t)(GAME_COUNT - 1U);
        } else {
            game_index--;
        }
        sync_menu_selection_from_game_index();
        play_move_sound();
        show_detail_page_chinese();
        delay(DELAY_PRESS);
    } else if (keys & J_RIGHT) {
        if (game_index == (uint8_t)(GAME_COUNT - 1U)) {
            game_index = 0U;
        } else {
            game_index++;
        }
        sync_menu_selection_from_game_index();
        play_move_sound();
        show_detail_page_chinese();
        delay(DELAY_PRESS);
    } else if (keys & J_UP) {
        change_detail_selection_by_step(-1);
    } else if (keys & J_DOWN) {
        change_detail_selection_by_step(1);
    }
}

static void run_main_chinese(void) {
    uint8_t config_test[7];

    ensure_ascii_glyph_cache();
    clear_screen(0, 17);
    show_loading_screen_chinese();
    SHOW_BKG;
    wait_vbl_done();
    delay(100);

    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET, config_test, 7);
    if (memcmp(config_test, "MBCXCFG", 7) == 0) {
        load_fixed_directory();
    } else {
        show_no_config_screen_chinese();
        while (1) {
        }
    }

    set_sprite_data(0, 1, arrow);
    set_sprite_tile(0, 0);
    move_sprite(0, ARROW_START_X, ARROW_START_Y);

    display_page_chinese(current_page);
    if (GAME_COUNT > 0U) {
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    } else {
        HIDE_SPRITES;
    }

    SHOW_BKG;

    while (1) {
        uint8_t keys = joypad();

        if (current_state == MAIN_MENU_STATE) {
            handle_main_menu_input_chinese(keys);
        } else if (current_state == DETAIL_PAGE_STATE) {
            handle_detail_page_input_chinese(keys);
        }

        delay(DELAY_PRESS);
    }
}
#endif

typedef struct t_metadata_header {
  uint8_t magic;                      // 魔术字（1字节�?
  uint8_t crc;                        // CRC校验�?字节�?
  uint8_t gamecnt;                    // 程序数量�?字节�?
  uint8_t all_sram_blkmap[MAX_SRAM_BANKS];   // SRAM block map, 60 bytes
} t_metadata_header;

// 程序信息结构体（每个35字节�?
typedef struct {
    char game_name[16];                  // 程序名称�?6字节�?
    uint8_t game_checksum[6];           // 校验和（6字节）从0x414A~0x414F获取
    uint8_t game_num;                  // 程序序号�?字节�?
    uint16_t romoffset;                  // ROM偏移量（2字节�?
    uint8_t rombanks;                    // ROM bank数量�?字节�?
    uint8_t rambanks;                    // RAM bank数量�?字节�?
    uint8_t game_sram_blkmap[MAX_GAME_SRAM_BANKS]; // 单程序SRAM块映射（8字节�?
} t_game_entry;  // 总共35字节�?6+6+1+2+1+1+8=35



// =============================================================================
// 屏幕与UI管理
// =============================================================================

// 等待V-Blank期间关闭LCD，以避免损坏硬件
void turn_off_lcd(void) {
    // 等待非V-Blank和V-Blank期间
    while (STAT_REG & 0x03);   // 等待离开V-Blank
    while (!(STAT_REG & 0x03)); // 等待进入V-Blank
    LCDC_REG &= ~0x80;         // 关闭LCD（清除LCDC寄存器第7位）
}

// 清除指定区域的屏�?
void clear_screen(uint8_t start_y, uint8_t end_y) {
    // 清除整个屏幕
    for (uint8_t y = start_y; y <= end_y; y++) {
        gotoxy(0, y);
        printf("                    "); // 20个空格清除一�?
    }
}

// // Nintendo Logo数据
// const uint8_t NINTENDO_LOGO_HEADER[] = {
//     0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
//     0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
//     0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E
// };

// =============================================================================
// MBC与内存管�?
// =============================================================================

// ROM大小信息�?
const RomSizeInfo rom_size_info[] = {
    {0x00, "32KB"},
    {0x01, "64KB"},
    {0x02, "128KB"},
    {0x03, "256KB"},
    {0x04, "512KB"},
    {0x05, "1MB"},
    {0x06, "2MB"},
    {0x07, "4MB"},
    {0x08, "8MB"},
    // 结束标记
    {0x09, "NULL"}
};

// RAM大小信息�?
const RamSizeInfo ram_size_info[] = {
    {0x00, "None"},
    {0x01, "2KB"},
    {0x02, "8KB"},
    {0x03, "32KB"},
    {0x04, "128KB"},
    {0x05, "64KB"},
    // 结束标记
    {0x06, "NULL"}
};

// 完整�?MBC 类型信息�?
const MbcInfo mbc_info_table[] = {
    {0x00, "ROM",      "ROM ONLY"},
    {0x01, "MBC1",     "MBC1"},
    {0x02, "MBC1",     "MBC1+RAM"},
    {0x03, "MBC1",     "MBC1+RAM+BATTERY"},
    {0x05, "MBC2",     "MBC2"},
    {0x06, "MBC2",     "MBC2+BATTERY"},
    {0x08, "ROM",      "ROM+RAM"},
    {0x09, "ROM",      "ROM+RAM+BATTERY"},
    {0x0B, "MMM01",    "MMM01"},
    {0x0C, "MMM01",    "MMM01+RAM"},
    {0x0D, "MMM01",    "MMM01+RAM+BATTERY"},
    {0x0F, "MBC3",     "MBC3+TIMER+BATTERY"},
    {0x10, "MBC30",    "MBC30+TIMER+RAM+BATTERY"},
    {0x11, "MBC3",     "MBC3"},
    {0x12, "MBC3",     "MBC3+RAM"},
    {0x13, "MBC3",     "MBC3+RAM+BATTERY"},
    {0x19, "MBC5",     "MBC5"},
    {0x1A, "MBC5",     "MBC5+RAM"},
    {0x1B, "MBC5",     "MBC5+RAM+BATTERY"},
    {0x1C, "MBC5",     "MBC5+RUMBLE"},
    {0x1D, "MBC5",     "MBC5+RUMBLE+RAM"},
    {0x1E, "MBC5",     "MBC5+RUMBLE+RAM+BATTERY"},
    {0x20, "MBC6",     "MBC6"},
    {0x22, "MBC7",     "MBC7+SENSOR+RUMBLE+RAM+BATTERY"},
    {0xFC, "POCKET CAMERA", "POCKET CAMERA"},
    {0xFD, "BANDAI TAMA5",  "BANDAI TAMA5"},
    {0xFE, "HuC3",     "HuC3"},
    {0xFF, "HuC1",     "HuC1+RAM+BATTERY"},
    // 结束标记
    {0xFF, NULL, NULL}
};


// =============================================================================
// 程序扫描与验�?
// =============================================================================

const char* get_mbc_type_str(uint8_t mbc_type) {
    const MbcInfo* info = mbc_info_table;
    while (info->main_type != NULL) {
        if (info->code == mbc_type) {
            return info->main_type;
        }
        info++;
    }
    return "UNKNOWN";
}

#if MENU_BUILD_EN
const char* get_mbc_full_description(uint8_t mbc_type) {
    const MbcInfo* info = mbc_info_table;
    while (info->main_type != NULL) {
        if (info->code == mbc_type) {
            return info->full_desc;
        }
        info++;
    }
    return "UNKNOWN MBC TYPE";
}
#endif

const char* get_rom_size_str(uint8_t rom_size) {
    const RomSizeInfo* info = rom_size_info;
    while (info->code != 0xFF) {
        if (info->code == rom_size) {
            return info->desc;
        }
        info++;
    }
    return rom_size_info[sizeof(rom_size_info) / sizeof(RomSizeInfo) - 1].desc;
}

// 获取RAM大小字符�?
const char* get_ram_size_str(uint8_t ram_size) {
    const RamSizeInfo* info = ram_size_info;
    while (info->code != 0xFF) {  // 遍历直到结束标记
        if (info->code == ram_size) {
            return info->desc;
        }
        info++;
    }
    return ram_size_info[sizeof(ram_size_info)/sizeof(RamSizeInfo)-1].desc; // 返回最后一个未知�?
}

// 从ROM中读取程序名�?
#if MENU_BUILD_EN
void read_game_name(char* game_name, uint16_t rom_bank) {
    // 切换到指定的ROM bank
    switch_rom_bank(rom_bank);

    // 从ROM_INFO_ADDR读取程序名称
    volatile uint8_t* info_ptr = ROM_INFO_ADDR;

    for (uint8_t x = 0; x < GAME_NAME_LEN - 1; x++) {
        game_name[x] = *info_ptr++;
    }
    game_name[GAME_NAME_LEN - 1] = '\0';
    
    // 清理程序名称中的非打印字符和空格
    for (uint8_t x = 0; x < GAME_NAME_LEN - 1; x++) {
        if (game_name[x] < 0x20 || game_name[x] > 0x7E) {
            game_name[x] = ' ';
        }
    }
    
    // 移除名称末尾的空�?
    int8_t end = GAME_NAME_LEN - 2;
    while (end >= 0 && game_name[end] == ' ') {
        game_name[end] = '\0';
        end--;
    }
    
    // 如果名称为空，使用默认名�?
    if (end < 0) {
        sprintf(game_name, "GAME%02u", rom_bank & 0xFF);
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
}
#endif

// 显示指定页的程序信息
#if MENU_BUILD_EN
static void display_page_english(uint8_t page) {
    char game_name_text[GAME_NAME_LEN];
    uint8_t start_index = (uint8_t)(page * GAMES_PER_PAGE);
    uint8_t end_index = start_index + GAMES_PER_PAGE;

    mode(M_TEXT_OUT);
    clear_screen(0, 17);

    gotoxy(4, 0);
    printf("MORI MENU EX");
    gotoxy(1, 17);
    printf("Ver%s", VERSION_NUM);
    gotoxy(14, 17);
    if (page + 1U < 10U) {
        printf("0%u/", page + 1U);
    } else {
        printf("%u/", page + 1U);
    }
    if (TOTAL_PAGES < 10U) {
        printf("0%u", TOTAL_PAGES);
    } else {
        printf("%u", TOTAL_PAGES);
    }

    if (GAME_COUNT == 0U) {
        gotoxy(14, 17);
        printf("00/00");
        gotoxy(3, 8);
        printf("No Game Found!");
        gotoxy(3, 9);
        printf("Check ChisMBCX");
        return;
    }

    if (end_index > GAME_COUNT) {
        end_index = GAME_COUNT;
    }

    for (uint8_t i = start_index; i < end_index; i++) {
        uint8_t display_row = (uint8_t)(i - start_index + 1U);

        if (GameROMBanks[i] == 0U) {
            break;
        }

        load_game_menu_name_ascii(i, game_name_text, sizeof(game_name_text));
        gotoxy(0, display_row);
        printf("                    ");
        gotoxy(2, display_row);
        if (i + 1U < 10U) {
            printf("0%u.%s", i + 1U, game_name_text);
        } else {
            printf("%u.%s", i + 1U, game_name_text);
        }
    }
}

static void show_loading_screen_english(void) {
    clear_screen(0, 17);
    gotoxy(4, 8);
    printf("LOADING ...");
    SHOW_BKG;
    wait_vbl_done();
}

static void show_no_config_screen_english(void) {
    clear_screen(0, 17);
    gotoxy(1, 7);
    printf("NO CONFIG FOUND");
    gotoxy(1, 8);
    printf("PLEASE FLASH ROM");
    wait_vbl_done();
}

static void show_detail_page_english(void) {
    uint16_t bank_addr;
    char game_name_text[GAME_NAME_LEN];
    uint8_t rom_size;
    uint8_t ram_size;
    const char* mbc_full_desc;
    const char* rom_size_desc;
    const char* ram_size_desc;

    mode(M_TEXT_OUT);
    HIDE_SPRITES;

    bank_addr = GameROMBanks[game_index];
    if (bank_addr == 0U) {
        play_cancel_sound();
        current_state = MAIN_MENU_STATE;
        return_to_main_menu();
        delay(DELAY_PRESS);
        return;
    }

    load_game_menu_name_ascii(game_index, game_name_text, sizeof(game_name_text));
    mbc_type = GameMBCs[game_index];
    rom_size = GameROMSizes[game_index];
    ram_size = GameRAMSizes[game_index];
    mbc_full_desc = get_mbc_full_description(mbc_type);
    rom_size_desc = get_rom_size_str(rom_size);
    ram_size_desc = get_ram_size_str(ram_size);

    scroll_text = mbc_full_desc;
    scroll_text_length = strlen(mbc_full_desc);
    scroll_enabled = (scroll_text_length > 16);
    scroll_state = 3U;
    scroll_offset = 0;
    scroll_timer = 0;
    pause_timer = scroll_delay;

    clear_screen(0, 17);

    gotoxy(4, 0);
    printf("GAME DETAILS");
    gotoxy(DETAILS_OFFSET_X + 0, 2);
    printf("Index:");
    gotoxy(DETAILS_OFFSET_X + 1, 3);
    if (game_index + 1U < 10U) {
        printf("0%u", game_index + 1U);
    } else {
        printf("%u", game_index + 1U);
    }

    gotoxy(DETAILS_OFFSET_X + 0, 4);
    printf("Name:");
    gotoxy(DETAILS_OFFSET_X + 1, 5);
    printf("%s", game_name_text);

    gotoxy(DETAILS_OFFSET_X + 0, 6);
    printf("MBC:");
    if (scroll_enabled) {
            update_scroll();
    } else {
        gotoxy(DETAILS_OFFSET_X + 1, 7);
        printf("%s", mbc_full_desc);
    }

    gotoxy(DETAILS_OFFSET_X + 0, 8);
    printf("ROM:");
    gotoxy(DETAILS_OFFSET_X + 1, 9);
    printf("%s", rom_size_desc);
    gotoxy(DETAILS_OFFSET_X + 0, 10);
    printf("RAM:");
    gotoxy(DETAILS_OFFSET_X + 1, 11);
    printf("%s", ram_size_desc);
    gotoxy(DETAILS_OFFSET_X + 0, 12);
    printf("BANK:");
    gotoxy(DETAILS_OFFSET_X + 1, 13);
    printf("%d", bank_addr);
    if (GameRAMBankMaps[game_index][0] != 0xFFU) {
        printf(", %d", GameRAMBankMaps[game_index][0]);
    }

    gotoxy(DETAILS_OFFSET_X + 0, 16);
    printf("A:Enter");
    gotoxy(DETAILS_OFFSET_X + 12, 16);
    printf("B:Back");
    gotoxy(1, 17);
    printf("MORI MENU");
    gotoxy(14, 17);
    if (game_index + 1U < 10U) {
        printf("0%u/%u", game_index + 1U, GAME_COUNT);
    } else {
        printf("%u/%u", game_index + 1U, GAME_COUNT);
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    wait_vbl_done();
}

static void return_to_main_menu_english(void) {
    mode(M_TEXT_OUT);
    HIDE_SPRITES;
    display_page_english(current_page);
    if (GAME_COUNT > 0U) {
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    }
}

static void handle_main_menu_input_english(uint8_t keys) {
    if (GAME_COUNT == 0U) {
        return;
    }

    if (keys & J_DOWN) {
        move_arrow_wrapped(1);
    } else if (keys & J_UP) {
        move_arrow_wrapped(-1);
    } else if (keys & J_LEFT) {
        change_page_safe(-1);
    } else if (keys & J_RIGHT) {
        change_page_safe(1);
    } else if (keys & J_A) {
        play_detail_sound();
        current_state = DETAIL_PAGE_STATE;
        show_detail_page_english();
        delay(DELAY_PRESS * 2);
    }
}

static void handle_detail_page_input_english(uint8_t keys) {
    if (keys & J_A) {
        play_confirm_sound();
        delay(DELAY_PRESS * 7);
        into_game();
    } else if (keys & J_B) {
        play_cancel_sound();
        current_state = MAIN_MENU_STATE;
        return_to_main_menu_english();
        delay(DELAY_PRESS);
    } else if (keys & J_LEFT) {
        if (game_index == 0U) {
            game_index = (uint8_t)(GAME_COUNT - 1U);
        } else {
            game_index--;
        }
        sync_menu_selection_from_game_index();
        play_move_sound();
        show_detail_page_english();
        delay(DELAY_PRESS);
    } else if (keys & J_RIGHT) {
        if (game_index == (uint8_t)(GAME_COUNT - 1U)) {
            game_index = 0U;
        } else {
            game_index++;
        }
        sync_menu_selection_from_game_index();
        play_move_sound();
        show_detail_page_english();
        delay(DELAY_PRESS);
    } else if (keys & J_UP) {
        change_detail_selection_by_step(-1);
    } else if (keys & J_DOWN) {
        change_detail_selection_by_step(1);
    }
}

static void run_main_english(void) {
    uint8_t config_test[7];

    current_state = MAIN_MENU_STATE;
    show_loading_screen_english();
    delay(100);

    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET, config_test, 7);
    if (memcmp(config_test, "MBCXCFG", 7) == 0) {
        load_fixed_directory();
    } else {
        show_no_config_screen_english();
        while (1) {
        }
    }

    set_sprite_data(0, 1, arrow);
    set_sprite_tile(0, 0);
    move_sprite(0, ARROW_START_X, ARROW_START_Y);

    display_page_english(current_page);
    if (GAME_COUNT > 0U) {
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    } else {
        HIDE_SPRITES;
    }

    SHOW_BKG;

    while (1) {
        uint8_t keys = joypad();

        if (current_state == MAIN_MENU_STATE) {
            handle_main_menu_input_english(keys);
        } else if (current_state == DETAIL_PAGE_STATE) {
            update_scroll();
            handle_detail_page_input_english(keys);
        }

        delay(DELAY_PRESS);
    }
}
#endif

void display_page(uint8_t page) {
#if MENU_BUILD_ZH
    display_page_chinese(page);
#else
    display_page_english(page);
#if 0
    mode(M_TEXT_OUT);
    // 清屏
    clear_screen(0,17);
    
    // 顶部居中标题
    gotoxy(4, 0);
    printf("MORI MENU EX");
    // 底部翻页和版�?
    gotoxy(1, 17);
    printf("Ver%s", VERSION_NUM);   
    gotoxy(14, 17); // 右下角位置（20-6=14列，17行）

    // 手动格式化页码显�?
    if (page + 1 < 10) {
        printf("0%d/", page + 1);
    } else {
        printf("%d/", page + 1);
    }
    if (TOTAL_PAGES < 10) {
        printf("0%d", TOTAL_PAGES);
    } else {
        printf("%d", TOTAL_PAGES);
    }

    // 如果没有找到任何程序
    if (GAME_COUNT == 0) {
        // 显示页码00/00
        gotoxy(14, 17);
        printf("00/00");
        gotoxy(3, 8);
        printf("No Game Found!");
        gotoxy(3, 9);
        printf("Check ChisMBCX");
        return;
    }

    char game_name_text[GAME_NAME_LEN];                // 存储当前要显示的程序名称字符�?
    uint8_t start_index = page * GAMES_PER_PAGE;       // 当前页面显示的第一个程序索�?
    uint8_t end_index = start_index + GAMES_PER_PAGE;  // 当前页面显示的最后一个程序索�?1
    
    if (end_index > GAME_COUNT) {
        end_index = GAME_COUNT;
    }
    
    // 遍历当前页的程序
    for (uint8_t i = start_index; i < end_index; i++) {
        // 使用ValidBanks数组获取bank地址
        uint16_t bank_addr = GameROMBanks[i];
        if (bank_addr == 0) break; // 无效bank，停止显�?
        load_game_menu_name_ascii(i, game_name_text, sizeof(game_name_text));
        
        // 计算显示位置在第几行（每�?个程序，每个占两行）
        uint8_t display_row = ((i - start_index) * 1) + 1; // 从第1行开�?
        
        // 先清除当前行的内�?
        gotoxy(0, display_row);
        printf("                    ");
        
        gotoxy(2, display_row);
        if (i + 1 < 10) {
            printf("0%d.%s", i + 1, game_name_text);
        } else {
            printf("%d.%s", i + 1, game_name_text);
        }
    }
#endif
#endif
}
// 箭头移动函数
inline void move_arrow(int direction) {
    int new_index = arrow_index + direction; // 计算新位�?    // 计算当前页的实际程序数量
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t games_on_current_page = get_games_on_page(current_page);
    // 边界检�?- 确保在每页内移动
    if (new_index >= 0 && new_index < games_on_current_page) {
        arrow_index = new_index; // 更新箭头位置
        game_index = (uint8_t)(current_page * games_per_page + (uint8_t)arrow_index);  // 更新程序索引
        
        // 移动精灵到新坐标（每个程序占1行）
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        
        play_move_sound();  // 播放移动音效
        delay(DELAY_PRESS); // 防抖延迟
    }
}

// 主菜单界面切换页�?
inline void change_page(int direction) {
    int8_t new_page;
    uint8_t saved_arrow_idx;
    uint8_t games_on_this_page;

    if (GAME_COUNT == 0U || TOTAL_PAGES == 0U) {
        return;
    }

    saved_arrow_idx = (uint8_t)arrow_index;
    new_page = (int8_t)current_page + direction;
    if (new_page < 0 || (uint8_t)new_page >= TOTAL_PAGES) {
        return;
    }

    games_on_this_page = get_games_on_page((uint8_t)new_page);
    if (games_on_this_page == 0U) {
        return;
    }

    current_page = (uint8_t)new_page;
    if (saved_arrow_idx >= games_on_this_page) {
        arrow_index = (int)(games_on_this_page - 1U);
    } else {
        arrow_index = (int)saved_arrow_idx;
    }

    game_index = (uint8_t)(current_page * get_menu_games_per_page() + (uint8_t)arrow_index);
    display_page(current_page);
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
    play_page_sound();
    delay(DELAY_PRESS * 2);
}

// 进入程序函数（保持原始机器码结构�?
void into_game(void) {
    disable_interrupts(); // 关闭中断
    turn_off_lcd();       // 关闭显示
    uint8_t ram_map_send_idx;
    
    // 将RAM切换到bank0，顺便打开RAM bank[6]（置高启用ram锁，此时禁止写入RAM�?
    // 0x40 = 0100 0000
    *RAM_BANK_ADDR = 0x40;

    uint8_t rom_bank_high = (GameROMBanks[game_index] >> 8) & 0xFF;
    uint8_t rom_bank_low = GameROMBanks[game_index] & 0xFF;
    *GAME_SEL_ADDR = rom_bank_low;
    *GAME_SEL_ADDR = rom_bank_high;

    // 2. 发送MBC类型
    *GAME_SEL_ADDR = GameMBCs[game_index];

    if (GameRAMBankMaps[game_index][0] != 0xFF && GameRAMBankMaps[game_index][0] != 0xFE) {
        *GAME_SEL_ADDR = 0xFE;
        for (ram_map_send_idx = 0; ram_map_send_idx < MAX_GAME_SRAM_BANKS; ram_map_send_idx++) {
            if (GameRAMBankMaps[game_index][ram_map_send_idx] == 0xFF || GameRAMBankMaps[game_index][ram_map_send_idx] == 0xFE) {
                break;
            }
            *GAME_SEL_ADDR = GameRAMBankMaps[game_index][ram_map_send_idx];
        }
    }
    
    // 4. 发送结束标�?(必须)
    *GAME_SEL_ADDR = 0xFF; // 表示RAM bank序列结束
    // 将机器码复制到HRAM (FF80~FFFE�?27字节)
    uint8_t hram_code[] = {
        0x3E, 0x01,       // ld a, $01
        0xEA, 0x00, 0xA0, // ld ($A000), a  
        0x00, 0x00,       // nop nop
        0x00, 0x00,       // nop nop

        // 2. 关闭ram锁，并触发重�?
        // 向ram_bank6�?0，关闭ram�?
        0x3E, 0x00,       // ld a, $00
        0xEA, 0x00, 0x40, // ld ($4000), a

        // 3. 弹出寄存器，恢复之前的状�?
        0xE1,             // pop hl
        0x31, 0xFE, 0xFF, // ld sp, $fffe

        // 4. 针对某些硬解机，不重启就�?x0100
        0xC3, 0x00, 0x01  // jp $0100

    };

    // 复制到HRAM
    for (uint8_t i = 0; i < sizeof(hram_code); i++) {
        GB_HRAM_ADDR[i] = hram_code[i];
    }

    // 设置LCDC寄存器，启用LCD显示
    uint8_t lcd_config = 0x91;
    LCDC_REG = lcd_config;

    // 执行HRAM中的代码
    void (*jump_to_hram)(void) = (void (*)(void))0xFF80;
    jump_to_hram();  // 实际执行跳转
}


// 更新滚动显示
#if MENU_BUILD_EN
void update_scroll(void) {
    if (scroll_enabled == 0) return;

    // 更新计时�?
    scroll_timer++;

    switch (scroll_state) {
        case 0: // 向右滚动（字符串向右移动）直到最后一个字符显示在最右侧
            if (scroll_timer >= scroll_speed) {
                scroll_timer = 0;
                scroll_offset++; // 增加偏移量使字符串向右移�?
                
                // 检查是否到达末尾（最后一个字符在屏幕最右侧�?
                // scroll_offset + 15 是屏幕上最右侧字符的索�?
                if (scroll_offset + 15 == scroll_text_length - 1) {
                    scroll_state = 1; // 进入暂停状�?
                    pause_timer = scroll_delay; // 暂停
                }
            }
            break;
            
        case 1: // 暂停在末�?
            if (pause_timer > 0) {
                pause_timer--;
            } else {
                scroll_state = 2; // 进入向左滚动状�?
            }
            break;
            
        case 2: // 向左滚动（字符串向左移动）直到第一个字符到达最左侧
            if (scroll_timer >= scroll_speed) {
                scroll_timer = 0;
                scroll_offset--; // 减小偏移量使字符串向左移�?
                
                // 检查是否到达开头（第一个字符在屏幕最左侧�?
                if (scroll_offset == 0) {
                    scroll_state = 3; // 进入暂停状�?
                    pause_timer = scroll_delay; // 暂停
                }
            }
            break;
            
        case 3: // 暂停在开�?
            if (pause_timer > 0) {
                pause_timer--;
            } else {
                scroll_state = 0; // 重新开始向右滚�?
                scroll_offset = 1; // 从第二个字符开�?
            }
            break;
    }
    
    // 显示逻辑
    gotoxy(DETAILS_OFFSET_X + 1, 7);
    for (uint8_t i = 0; i < 16; i++) {
        int16_t pos = scroll_offset + i;
        if (pos >= 0 && pos < scroll_text_length) {
            putchar(scroll_text[pos]);
        } else {
            putchar(' '); // 超出范围显示空格
        }
    }
}
#endif

// 显示详细信息页面
void show_detail_page(void) {
#if MENU_BUILD_ZH
    show_detail_page_chinese();
#else
    show_detail_page_english();
#if 0
    mode(M_TEXT_OUT);
    // 隐藏精灵
    HIDE_SPRITES;

    // 获取当前程序的bank地址
    uint16_t bank_addr = GameROMBanks[game_index];
    if (bank_addr == 0) {
        // 无效bank，返回主菜单
        play_cancel_sound();
        current_state = MAIN_MENU_STATE;
        return_to_main_menu();
        delay(DELAY_PRESS);
        return;
    }
    // 从固定映射条目加载英文名称，缺失时退�?ROM header
    char game_name_text[GAME_NAME_LEN];
    load_game_menu_name_ascii(game_index, game_name_text, sizeof(game_name_text));

    // 读取MBC类型、ROM大小和RAM大小
    mbc_type = GameMBCs[game_index];
    uint8_t rom_size = GameROMSizes[game_index];
    uint8_t ram_size = GameRAMSizes[game_index];
    
    const char* mbc_full_desc = get_mbc_full_description(mbc_type);
    const char* rom_size_desc = get_rom_size_str(rom_size);
    const char* ram_size_desc = get_ram_size_str(ram_size);

    // 设置滚动文本
    scroll_text = mbc_full_desc;
    scroll_text_length = strlen(mbc_full_desc);
    scroll_enabled = (scroll_text_length > 16); // 文本超过16字符才启用滚�?
    
    // 清除中间区域
    clear_screen(0,17); // 清除从第1行到�?6行的内容
    
    // 绘制详细信息�?
    // 顶部标题
    gotoxy(4, 0);
    printf("GAME DETAILS");

    // 显示程序名称和索�?
    gotoxy(DETAILS_OFFSET_X+0, 2);
    printf("Index:");
    gotoxy(DETAILS_OFFSET_X+1, 3);
    if (game_index + 1 < 10) {
        printf("0%d", game_index + 1); // 显示为两位数
    } else {
        printf("%d", game_index + 1);
    }

    // 显示完整�?5字节程序名称
    gotoxy(DETAILS_OFFSET_X+0, 4);
    printf("Name:");
    gotoxy(DETAILS_OFFSET_X+1, 5);
    printf("%s", game_name_text);
    
    // 显示MBC类型完整描述
    gotoxy(DETAILS_OFFSET_X+0, 6);
    printf("MBC:");
    // MBC描述将在update_scroll中显�?
    if (scroll_enabled) {
        scroll_state = 3;     // 初始状态：向右滚动
        scroll_offset = 0;     // 从第二个字符开�?
        scroll_timer = 0;      // 重置计时�?
        pause_timer = scroll_delay;       // 重置暂停计时�?
            update_scroll();
    }else{
        gotoxy(DETAILS_OFFSET_X+1, 7);
        printf("%s", mbc_full_desc); // 如果不启用滚动，直接显示完整描述
    }

    // 显示ROM大小
    gotoxy(DETAILS_OFFSET_X+0, 8);
    printf("ROM:");
    gotoxy(DETAILS_OFFSET_X+1, 9);
    printf("%s", rom_size_desc);  
    // 显示RAM大小
    gotoxy(DETAILS_OFFSET_X+0, 10);
    printf("RAM:");
    gotoxy(DETAILS_OFFSET_X+1, 11);
    printf("%s", ram_size_desc);   
    gotoxy(DETAILS_OFFSET_X+0, 12);
    printf("BANK:");
    gotoxy(DETAILS_OFFSET_X+1, 13);
    printf("%d", bank_addr);
    
    // 如果程序有RAM，显示RAM bank
    if (GameRAMBankMaps[game_index][0] != 0xFF) {
        printf(", %d", GameRAMBankMaps[game_index][0]);
    }   


    // 显示操作提示
    gotoxy(DETAILS_OFFSET_X+0, 16);
    printf("A:Enter");
    gotoxy(DETAILS_OFFSET_X+12, 16);
    printf("B:Back");
    
    // 底部翻页和版�?
    gotoxy(1, 17);
    printf("MORI MENU");   
    gotoxy(14, 17);
    if(game_index + 1 < 10) {
        printf("0%d/%d", game_index + 1, GAME_COUNT);
    } else {
        printf("%d/%d", game_index + 1, GAME_COUNT);
    }   
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    // 刷新屏幕
    wait_vbl_done();
#endif
#endif
}


// 返回到主菜单
void return_to_main_menu(void) {
#if MENU_BUILD_ZH
    HIDE_SPRITES;
    display_page(current_page);
    if (GAME_COUNT > 0U) {
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    }
#else
    return_to_main_menu_english();
#if 0
#if MENU_BUILD_EN
        mode(M_TEXT_OUT);
        clear_screen(0,17);
#endif
    
    // 隐藏精灵
    HIDE_SPRITES;
    // 重新显示主菜�?
    display_page(current_page);
    
    // 只有在有程序时才重新显示箭头
    if (GAME_COUNT > 0) {
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    }
#endif
#endif
}

// 主程序入�?
void main(void) {
    // ==================== 系统初始化阶�?====================
    
    // 1. 初始化字体系�?- 设置IBM字体用于文本显示
    font_init();
    font_set(font_load(font_ibm));
    
    // 2. 初始化声音系�?- 加载音效样本
    init_sound();
#if MENU_BUILD_ZH
    run_main_chinese();
    return;
#endif
    
    // 3. 确保ASCII字符缓存已加�?- 用于快速显示英文字�?
#if MENU_BUILD_ZH
    ensure_ascii_glyph_cache();
#endif
    
    // 4. 清除屏幕 - 准备显示加载界面
    clear_screen(0, 17);
    
    // 5. 显示加载提示（关键：先显示加载界面，再执行可能卡死的操作�?
    gotoxy(4, 8);                  // 光标移动到屏幕中间位�?
    printf("LOADING ...");         // 显示"LOADING..."文字
    SHOW_BKG;                      // 立即显示背景
    wait_vbl_done();               // 等待垂直同步，确保画面刷�?
    delay(100);                    // 给用户看到加载提示的时间
    
    // ==================== 游戏扫描阶段（带保护机制�?====================
    
    // 6. 扫描ROM中的游戏目录
    //    从配置区读取游戏列表，解析每个游戏的元数�?
    //    使用简单的检测：先检查配置区签名是否存在
    uint8_t config_test[7];
    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET, config_test, 7);
    
    if (memcmp(config_test, "MBCXCFG", 7) == 0) {
        // 配置区有效，继续扫描
        load_fixed_directory();
    } else {
        // 配置区无效，显示错误信息
        clear_screen(0, 17);
        gotoxy(1, 7);
        printf("NO CONFIG FOUND");
        gotoxy(1, 8);
        printf("PLEASE FLASH ROM");
        wait_vbl_done();
        // 进入死循环，防止崩溃
        while(1);
    }
    
    // ==================== 界面初始化阶�?====================
    
    // 7. 初始化箭头精灵（用于菜单选择�?
    set_sprite_data(0, 1, arrow);              // 加载箭头图形数据到精灵内�?
    set_sprite_tile(0, 0);                      // 设置精灵0使用�?个tile
    move_sprite(0, ARROW_START_X, ARROW_START_Y); // 移动箭头到初始位�?
    
    // 8. 显示游戏列表页面
    display_page(current_page);
    
    // 9. 根据游戏数量决定是否显示箭头
    if (GAME_COUNT > 0) {
        move_sprite(0, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    } else {
        // 如果没有游戏，隐藏精�?
        HIDE_SPRITES;
    }
    
    // 10. 显示背景层（游戏列表文字在背景层�?
    SHOW_BKG;


    // ==================== 主循环阶�?====================
    
    // 12. 主循�?- 无限循环处理用户输入
    while (1) {
        // 读取当前按键状�?
        uint8_t keys = joypad();
        
        // ==================== 状�?: 主菜单页�?====================
        if (current_state == MAIN_MENU_STATE) {
            // 主菜单状态下的按键处�?
            
            // 下方向键：移动箭头向下（选择下一个游戏）
            if (keys & J_DOWN) {
                if (GAME_COUNT > 0) { 
                    move_arrow_wrapped(1);  // 参数1表示向下移动
                }
            }
            // 上方向键：移动箭头向上（选择上一个游戏）
            else if (keys & J_UP) {
                if (GAME_COUNT > 0) { 
                    move_arrow_wrapped(-1); // 参数-1表示向上移动
                }
            }
            // 左方向键：切换到上一�?
            else if (keys & J_LEFT) {
                if (GAME_COUNT > 0) { 
                    change_page_safe(-1);
                }
            }
            // 右方向键：切换到下一�?
            else if (keys & J_RIGHT) {
                if (GAME_COUNT > 0) { 
                    change_page_safe(1);
                }
            }
            // A键：进入游戏详情页面
            else if (keys & J_A) {
                if (GAME_COUNT > 0) { 
                    play_detail_sound();          // 播放音效
                    current_state = DETAIL_PAGE_STATE; // 切换到详情页面状�?
                    show_detail_page();           // 显示详情页面内容
                    delay(DELAY_PRESS * 2);       // 按键延迟防重�?
                }
            }
        }
        // ==================== 状�?: 游戏详情页面 ====================
        else if (current_state == DETAIL_PAGE_STATE) {
#if MENU_BUILD_EN
            update_scroll();
#endif

            // 详情页面按键处理
            
            // A键：启动游戏
            if (keys & J_A) {
                play_confirm_sound();   // 播放确认音效
                delay(DELAY_PRESS * 7); // 延迟一下让音效播放�?
                into_game();            // 进入游戏（核心函数）
            }
            // B键：返回主菜�?
            else if (keys & J_B) {
                play_cancel_sound();              // 播放取消音效
                current_state = MAIN_MENU_STATE;  // 切换回主菜单状�?
                return_to_main_menu();            // 返回主菜单界�?
                delay(DELAY_PRESS);               // 按键延迟
            }
            // 左方向键：切换到上一个游戏详�?
            else if (keys & J_LEFT) {
                // 循环处理：如果已经是第一个，跳到最后一�?
                if (game_index == 0) {
                    game_index = GAME_COUNT - 1;
                } else {
                    game_index--;
                }
                // 更新页码和箭头索引（用于同步主菜单状态）
                sync_menu_selection_from_game_index();
                play_move_sound();          // 播放移动音效
                show_detail_page();         // 重新显示当前游戏详情
                delay(DELAY_PRESS);         // 按键延迟
            }
            // 右方向键：切换到下一个游戏详�?
            else if (keys & J_RIGHT) {
                // 循环处理：如果已经是最后一个，跳到第一�?
                if (game_index == GAME_COUNT - 1) {
                    game_index = 0;
                } else {
                    game_index++;
                }
                sync_menu_selection_from_game_index();
                play_move_sound();
                show_detail_page();
                delay(DELAY_PRESS);
            }
            // 上方向键：快速切换（跳过多个游戏�?
            else if (keys & J_UP) {
                change_detail_selection_by_step(-1);
            }
            // 下方向键：快速切换（跳过多个游戏�?
            else if (keys & J_DOWN) {
                change_detail_selection_by_step(1);
            }
        }

        // ==================== 循环结束 ====================
        
        // 按键延迟（防按键重复�?
        delay(DELAY_PRESS);
    }
}