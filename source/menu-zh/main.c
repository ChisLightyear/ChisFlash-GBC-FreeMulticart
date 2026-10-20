#include <gb/gb.h>
#include <gb/hardware.h>
#include <gbdk/console.h>
#include <gbdk/font.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ass/arrow.c"
#include "ass/sound.h"
#include "common.h"

#ifndef MENU_BUILD_ZH
#define MENU_BUILD_ZH 1
#endif

#define MAX_GAMES 99U
#define MAX_GAME_SRAM_BANKS 8U
#define MAX_SRAM_BANKS 64U

#define MENU_RUNTIME_ROM_BANK 1U
#define FIXED_MAP_CONFIG_OFFSET 0x020000UL
#define FIXED_MAP_ROM_DIR_OFFSET 0x040000UL
#define FIXED_MAP_FONT_OFFSET 0x0C0000UL
#define FIXED_MAP_HEADER_SIZE 64U
#define FIXED_MAP_ROM_ENTRY_SIZE 128U
#define FIXED_MAP_ROM_SRAM_MAP_OFFSET 110U
#define FIXED_MAP_MAX_LINEAR_BANK 0x07FFU
#define FIXED_MAP_FLAG_VALID 0x01U

#define MENU_LAYER_HEADER_OFFSET 0x00000040UL
#define MENU_LAYER_HEADER_MAGIC "MBCXLYR1"
#define MENU_LAYER_HEADER_SIZE 80U

#define MENU_SCREEN_TILES_W 20U
#define MENU_SCREEN_TILES_H 18U
#define MENU_TOP_BAR_TILES_H 2U
#define MENU_TOP_BAR_Y 0U
#define MENU_LIST_REGION_X 3U
#define MENU_LIST_REGION_Y 3U
#define MENU_LIST_REGION_TILES_W (MENU_SCREEN_TILES_W - MENU_LIST_REGION_X)
#define MENU_LIST_REGION_TILES_H 12U
#define MENU_BOTTOM_BAR_Y 16U
#define MENU_BOTTOM_BAR_TILES_H 2U
#define MENU_PAGE_LABEL_X 14U
#define MENU_PAGE_LABEL_Y 16U
#define MENU_PAGE_LABEL_TILES_W 6U
#define MENU_PAGE_LABEL_TILES_H 2U
#define MENU_PAGE_LABEL_RIGHT_MARGIN_PX 8U
#define MENU_PAGE_LABEL_BASELINE_Y 4U

#define MENU_TOP_BAR_TILEMAP_SIZE (MENU_SCREEN_TILES_W * MENU_TOP_BAR_TILES_H)
#define MENU_BOTTOM_BAR_TILEMAP_SIZE (MENU_SCREEN_TILES_W * MENU_BOTTOM_BAR_TILES_H)
#define MENU_LIST_TILEMAP_SIZE (MENU_SCREEN_TILES_W * MENU_SCREEN_TILES_H)
#define MENU_PAGE_LABEL_TILEMAP_SIZE (MENU_PAGE_LABEL_TILES_W * MENU_PAGE_LABEL_TILES_H)
#define MENU_DETAIL_TILEMAP_SIZE (MENU_SCREEN_TILES_W * MENU_SCREEN_TILES_H)
#define MENU_TILE_BYTES 16U

#define MENU_UI_MAX_TILES 84U
#define MENU_DYNAMIC_MAX_TILES 204U
#define MENU_UI_TILE_BASE 0U
#define MENU_PAGE_LABEL_TILE_CAP MENU_PAGE_LABEL_TILEMAP_SIZE
#define MENU_DYNAMIC_TILE_BASE 1U
#define MENU_ARROW_TILE_INDEX (MENU_DYNAMIC_TILE_BASE + MENU_DYNAMIC_MAX_TILES)
#define MENU_PAGE_LABEL_TILE_BASE (MENU_ARROW_TILE_INDEX + 1U)
#define MENU_INPUT_COOLDOWN_FRAMES 8U
#define MENU_CONFIRM_SOUND_WAIT_MS 420U

#define MAIN_MENU_STATE 0U
#define DETAIL_PAGE_STATE 1U

#define GAMES_PER_PAGE 8U
#define DETAIL_PAGE_VERTICAL_STEP 5U
#define ARROW_START_X 28U
#define ARROW_BASE_Y 43U
#define ARROW_Y_STEP 12U
#define MENU_COMPACT_GLYPH_COUNT 15U
#define MENU_COMPACT_GLYPH_BYTES_PER_CHAR 24U
#define MENU_COMPACT_LABEL_CHAR_COUNT 5U
#define MBC1_TYPE 0x01U
#define MBC5_TYPE 0x19U
#define MBC_DIALOG_X 6U
#define MBC_DIALOG_Y 6U
#define MBC_DIALOG_TILES_W 7U
#define MBC_DIALOG_TILES_H 5U
#define MBC_DIALOG_TILEMAP_SIZE (MBC_DIALOG_TILES_W * MBC_DIALOG_TILES_H)
#define MBC_DIALOG_PIXEL_W (MBC_DIALOG_TILES_W * 8U)
#define MBC_DIALOG_PIXEL_H (MBC_DIALOG_TILES_H * 8U)
#define MBC_DIALOG_OPTION_COUNT 2U
#define MBC_DIALOG_ARROW_X 60U
#define MBC_DIALOG_ARROW_Y0 76U
#define MBC_DIALOG_ARROW_Y1 90U

typedef struct {
    uint8_t cart_save_mode;
    uint8_t ui_language;
    uint16_t rom_count;
} FixedMenuConfig;

typedef struct {
    uint8_t version;
    uint8_t ui_tile_count;
    uint8_t dynamic_max_tile_count;
    uint8_t page_count;
    uint8_t rows_per_page;
    uint8_t reserved0[3];
    uint16_t game_count;
    uint16_t list_entry_words;
    uint32_t list_strings_offset;
    uint32_t ui_tile_data_offset;
    uint32_t top_bar_map_offset;
    uint32_t bottom_bar_static_map_offset;
    uint32_t page_label_map_offset;
    uint32_t list_tile_counts_offset;
    uint32_t list_tile_data_offset;
    uint32_t list_tilemap_offset;
    uint32_t detail_tile_counts_offset;
    uint32_t detail_tile_data_offset;
    uint32_t detail_tilemap_offset;
    uint32_t detail_top_bar_map_offset;
    uint32_t detail_bottom_bar_static_map_offset;
    uint32_t title_scroll_table_offset;
    uint32_t title_scroll_data_offset;
} MenuLayerHeader;

uint16_t GameROMBanks[MAX_GAMES];
uint16_t GameROMBanksNeeded[MAX_GAMES];
uint8_t GameMBCs[MAX_GAMES];
uint8_t GameMbcOverrides[MAX_GAMES];
uint8_t GameHasBattery[MAX_GAMES];
uint8_t GameRAMBanks[MAX_GAMES];
uint8_t GameRAMSizes[MAX_GAMES];
uint8_t GameROMSizes[MAX_GAMES];
uint8_t GameRAMBankMaps[MAX_GAMES][MAX_GAME_SRAM_BANKS];
uint32_t GameEntryAddrs[MAX_GAMES];

uint8_t GAME_COUNT = 0U;
uint8_t TOTAL_PAGES = 0U;
uint8_t game_index = 0U;
uint8_t current_page = 0U;
int arrow_index = 0;
uint8_t current_state = MAIN_MENU_STATE;
uint8_t gPageLabelTileBase = 0U;
uint8_t gCompactGlyphSizePx = 8U;
uint8_t gCompactGlyphBytesPerChar = 8U;
uint8_t gCompactGlyphRowBytes = 1U;
FixedMenuConfig gMenuConfig = {0U, MENU_BUILD_ZH ? 1U : 0U, 0U};
MenuLayerHeader gMenuLayer = {0U};
uint8_t gTopBarTileMap[MENU_TOP_BAR_TILEMAP_SIZE];
uint8_t gBottomBarTileMap[MENU_BOTTOM_BAR_TILEMAP_SIZE];
uint8_t gPageLabelTileMap[MENU_PAGE_LABEL_TILEMAP_SIZE];
uint8_t gCompactGlyphRows[MENU_COMPACT_GLYPH_COUNT * MENU_COMPACT_GLYPH_BYTES_PER_CHAR];
uint8_t gPageLabelTileData[MENU_PAGE_LABEL_TILEMAP_SIZE * MENU_TILE_BYTES];
uint8_t gPageTileData[MENU_DYNAMIC_MAX_TILES * MENU_TILE_BYTES];
uint8_t gListTileMap[MENU_LIST_TILEMAP_SIZE];
uint8_t gDetailTileMap[MENU_DETAIL_TILEMAP_SIZE];
uint8_t gDetailTopBarTileMap[MENU_TOP_BAR_TILEMAP_SIZE];
uint8_t gDetailBottomBarTileMap[MENU_BOTTOM_BAR_TILEMAP_SIZE];
uint8_t gCurrentDynamicTileCount = 0U;
uint8_t gInputCooldown = 0U;

volatile uint8_t* ROM_BANK_LO_ADDR = (volatile uint8_t *)0x2000;
volatile uint8_t* ROM_BANK_HI_ADDR = (volatile uint8_t *)0x3000;
volatile uint8_t* RAM_BANK_ADDR = (volatile uint8_t *)0x4000;
volatile uint8_t* GAME_EN_ADDR = (volatile uint8_t *)0xA000;
volatile uint8_t* GAME_SEL_ADDR = (volatile uint8_t *)0xB000;
volatile uint8_t* ROM_INFO_ADDR = (volatile uint8_t *)0x4134;
volatile uint8_t* MBC_TYPE_ADDR = (volatile uint8_t *)0x4147;
volatile uint8_t* ROM_SIZE_ADDR = (volatile uint8_t *)0x4148;
volatile uint8_t* RAM_SIZE_ADDR = (volatile uint8_t *)0x4149;
volatile uint8_t* GB_HRAM_ADDR = (volatile uint8_t *)0xFF80;
volatile uint8_t* GB_WRAM_Hi_ADDR = (volatile uint8_t *)0xD000;

static uint16_t read_le16(const uint8_t* ptr);
static uint32_t read_le32(const uint8_t* ptr);
static void nor_read_linear_bytes(uint32_t linear_addr, uint8_t* buffer, uint16_t length);
static uint8_t rom_size_code_from_bank_count(uint16_t rom_bank_count);
static uint8_t ram_size_code_from_bank_count(uint8_t ram_bank_count);
static void clear_fixed_directory_cache(void);
static uint8_t get_menu_games_per_page(void);
static uint8_t get_games_on_page(uint8_t page);
static uint8_t get_menu_arrow_y_px(uint8_t row);
static void sync_menu_selection_from_game_index(void);
static uint8_t game_uses_sram_map(uint8_t index);
static uint8_t load_menu_layer_header(void);
static uint8_t load_ui_tiles_to_vram(void);
static void load_blank_tile_to_vram(void);
static uint8_t load_compact_glyphs(void);
static uint8_t load_top_bar(void);
static uint8_t load_bottom_bar_static(void);
static uint8_t load_page_label(uint8_t current_value, uint8_t total_value);
static uint8_t load_list_page(uint8_t page);
static uint8_t load_detail_page(uint8_t index);
static uint8_t load_detail_top_bar(void);
static uint8_t load_detail_bottom_bar_static(void);
static void clear_list_region(void);
static void clear_detail_content_region(void);
static uint8_t load_page_content(uint8_t page);
static void show_no_config_screen(void);
static void show_no_menu_layers_screen(void);
static void handle_main_menu_input(uint8_t keys);
static void handle_detail_page_input(uint8_t keys);
static void move_arrow_wrapped(int direction);
static void change_page_safe(int direction);
static void change_detail_selection(int direction);
static void show_detail_page(void);
static void show_mbc_selection_dialog(void);
static void return_to_main_menu(void);
static void run_main_menu(void);
static void into_game(void);
static uint8_t get_active_game_mbc(uint8_t index);
static uint8_t get_mbc_dialog_option_for_type(uint8_t mbc_type);
static uint8_t get_mbc_type_for_dialog_option(uint8_t option);
static uint8_t compact_glyph_index_for_char(char ch);
static uint16_t compact_glyph_row_bits(const uint8_t* glyph_rows, uint8_t row);
static void clear_dialog_buffer(uint8_t tile_base, uint8_t* tile_data, uint8_t* tile_map);
static void plot_dialog_pixel(uint8_t x, uint8_t y, uint8_t* tile_data);
static void draw_dialog_border_pixels(uint8_t* tile_data);
static void draw_dialog_compact_text(const char* text, uint8_t x, uint8_t y, uint8_t* tile_data);
static uint8_t render_mbc_dialog(void);

static uint16_t read_le16(const uint8_t* ptr) {
    return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8);
}

static uint32_t read_le32(const uint8_t* ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

static void nor_read_linear_bytes(uint32_t linear_addr, uint8_t* buffer, uint16_t length) {
    while (length > 0U) {
        uint16_t bank = (uint16_t)(linear_addr >> 14);
        uint16_t offset = (uint16_t)(linear_addr & 0x3FFFU);
        uint16_t chunk = (uint16_t)(0x4000U - offset);
        const uint8_t* src;

        if (chunk > length) {
            chunk = length;
        }

        if (bank == 0U) {
            src = ((const uint8_t*)0x0000U) + offset;
        } else {
            switch_rom_bank(bank);
            src = ((const uint8_t*)0x4000U) + offset;
        }

        while (chunk > 0U) {
            *buffer++ = *src++;
            linear_addr++;
            length--;
            chunk--;
        }
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
}

static uint8_t rom_size_code_from_bank_count(uint16_t rom_bank_count) {
    switch (rom_bank_count) {
        case 2:   return 0x00U;
        case 4:   return 0x01U;
        case 8:   return 0x02U;
        case 16:  return 0x03U;
        case 32:  return 0x04U;
        case 64:  return 0x05U;
        case 128: return 0x06U;
        case 256: return 0x07U;
        case 512: return 0x08U;
        default:  return 0x09U;
    }
}

static uint8_t ram_size_code_from_bank_count(uint8_t ram_bank_count) {
    switch (ram_bank_count) {
        case 0:  return 0x00U;
        case 1:  return 0x02U;
        case 4:  return 0x03U;
        case 8:  return 0x05U;
        case 16: return 0x04U;
        default: return 0x06U;
    }
}

static void clear_fixed_directory_cache(void) {
    memset(GameROMBanks, 0, sizeof(GameROMBanks));
    memset(GameROMBanksNeeded, 0, sizeof(GameROMBanksNeeded));
    memset(GameMBCs, 0, sizeof(GameMBCs));
    memset(GameMbcOverrides, 0, sizeof(GameMbcOverrides));
    memset(GameHasBattery, 0, sizeof(GameHasBattery));
    memset(GameRAMBanks, 0, sizeof(GameRAMBanks));
    memset(GameRAMSizes, 0, sizeof(GameRAMSizes));
    memset(GameROMSizes, 0, sizeof(GameROMSizes));
    memset(GameRAMBankMaps, 0xFF, sizeof(GameRAMBankMaps));
    memset(GameEntryAddrs, 0, sizeof(GameEntryAddrs));
    memset(&gMenuLayer, 0, sizeof(gMenuLayer));
    GAME_COUNT = 0U;
    TOTAL_PAGES = 0U;
    current_page = 0U;
    game_index = 0U;
    arrow_index = 0;
    current_state = MAIN_MENU_STATE;
    gCurrentDynamicTileCount = 0U;
}

static uint8_t get_menu_games_per_page(void) {
    if (gMenuLayer.rows_per_page != 0U) {
        return gMenuLayer.rows_per_page;
    }
    return GAMES_PER_PAGE;
}

static uint8_t get_games_on_page(uint8_t page) {
    uint8_t start_index = (uint8_t)(page * get_menu_games_per_page());

    if (start_index >= GAME_COUNT) {
        return 0U;
    }

    if ((uint8_t)(start_index + get_menu_games_per_page()) > GAME_COUNT) {
        return (uint8_t)(GAME_COUNT - start_index);
    }

    return get_menu_games_per_page();
}

static uint8_t get_menu_arrow_y_px(uint8_t row) {
    return (uint8_t)(ARROW_BASE_Y + row * ARROW_Y_STEP);
}

static void sync_menu_selection_from_game_index(void) {
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t games_on_page;

    if (GAME_COUNT == 0U) {
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
    static uint8_t config_header[FIXED_MAP_HEADER_SIZE];
    static uint8_t dir_header[FIXED_MAP_HEADER_SIZE];
    static uint8_t entry_data[FIXED_MAP_ROM_ENTRY_SIZE];
    uint16_t entry_count;
    uint16_t entry_size;
    uint32_t entry_base;
    uint16_t i;

    clear_fixed_directory_cache();
    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET, config_header, FIXED_MAP_HEADER_SIZE);
    if (memcmp(config_header, "MBCXCFG", 7) != 0) {
        switch_rom_bank(MENU_RUNTIME_ROM_BANK);
        return 0U;
    }

    gMenuConfig.cart_save_mode = config_header[10];
    gMenuConfig.ui_language = MENU_BUILD_ZH ? 1U : 0U;
    gMenuConfig.rom_count = read_le16(config_header + 14);

    nor_read_linear_bytes(FIXED_MAP_ROM_DIR_OFFSET, dir_header, FIXED_MAP_HEADER_SIZE);
    if (memcmp(dir_header, "MBCXROM", 7) != 0) {
        switch_rom_bank(MENU_RUNTIME_ROM_BANK);
        return 0U;
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

    for (i = 0U; i < entry_count; i++) {
        uint32_t entry_addr = entry_base + (uint32_t)i * entry_size;
        uint32_t rom_offset_bank;
        uint16_t rom_bank_count;
        uint8_t ram_bank_count;
        uint8_t has_battery;
        uint8_t j;

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

        for (j = 0U; j < MAX_GAME_SRAM_BANKS; j++) {
            GameRAMBankMaps[GAME_COUNT][j] = 0xFFU;
        }

        if (game_uses_sram_map(GAME_COUNT)) {
            for (j = 0U; j < GameRAMBanks[GAME_COUNT]; j++) {
                uint8_t mapped_bank = entry_data[FIXED_MAP_ROM_SRAM_MAP_OFFSET + j];
                if (mapped_bank < MAX_SRAM_BANKS || mapped_bank == 0xFEU || mapped_bank == 0xFFU) {
                    GameRAMBankMaps[GAME_COUNT][j] = mapped_bank;
                }
            }
        }

        GAME_COUNT++;
    }

    gMenuConfig.rom_count = GAME_COUNT;
    if (GAME_COUNT > 0U) {
        TOTAL_PAGES = (uint8_t)((GAME_COUNT + get_menu_games_per_page() - 1U) / get_menu_games_per_page());
    }

    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return (GAME_COUNT > 0U);
}

static uint8_t load_menu_layer_header(void) {
    static uint8_t raw_header[MENU_LAYER_HEADER_SIZE];

    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET + MENU_LAYER_HEADER_OFFSET, raw_header, MENU_LAYER_HEADER_SIZE);
    if (memcmp(raw_header, MENU_LAYER_HEADER_MAGIC, 8) != 0) {
        switch_rom_bank(MENU_RUNTIME_ROM_BANK);
        return 0U;
    }

    gMenuLayer.version = raw_header[8];
    gMenuLayer.ui_tile_count = raw_header[9];
    gMenuLayer.dynamic_max_tile_count = raw_header[10];
    gMenuLayer.page_count = raw_header[11];
    gMenuLayer.rows_per_page = raw_header[12];
    gMenuLayer.reserved0[0] = raw_header[13];
    gMenuLayer.reserved0[1] = raw_header[14];
    gMenuLayer.reserved0[2] = raw_header[15];
    gCompactGlyphSizePx = 8U;
    gCompactGlyphBytesPerChar = 8U;
    gCompactGlyphRowBytes = 1U;
    if (gMenuLayer.version >= 4U) {
        if (raw_header[13] >= 8U) {
            gCompactGlyphSizePx = raw_header[13];
        }
        if (raw_header[14] >= gCompactGlyphSizePx) {
            gCompactGlyphBytesPerChar = raw_header[14];
        }
        if (raw_header[15] != 0U) {
            gCompactGlyphRowBytes = raw_header[15];
        } else {
            gCompactGlyphRowBytes = (uint8_t)((gCompactGlyphSizePx + 7U) >> 3);
        }
    }
    gMenuLayer.game_count = read_le16(raw_header + 16);
    gMenuLayer.list_entry_words = read_le16(raw_header + 18);
    gMenuLayer.list_strings_offset = read_le32(raw_header + 20);
    gMenuLayer.ui_tile_data_offset = read_le32(raw_header + 24);
    gMenuLayer.top_bar_map_offset = read_le32(raw_header + 28);
    gMenuLayer.bottom_bar_static_map_offset = read_le32(raw_header + 32);
    gMenuLayer.page_label_map_offset = read_le32(raw_header + 36);
    gMenuLayer.list_tile_counts_offset = read_le32(raw_header + 40);
    gMenuLayer.list_tile_data_offset = read_le32(raw_header + 44);
    gMenuLayer.list_tilemap_offset = read_le32(raw_header + 48);

    if (gMenuLayer.version >= 2U) {
        gMenuLayer.detail_tile_counts_offset = read_le32(raw_header + 52);
        gMenuLayer.detail_tile_data_offset = read_le32(raw_header + 56);
    gMenuLayer.detail_tilemap_offset = read_le32(raw_header + 60);
    } else {
        gMenuLayer.detail_tile_counts_offset = 0U;
        gMenuLayer.detail_tile_data_offset = 0U;
        gMenuLayer.detail_tilemap_offset = 0U;
    }

    if (gMenuLayer.version >= 3U) {
        gMenuLayer.detail_top_bar_map_offset = read_le32(raw_header + 64);
        gMenuLayer.detail_bottom_bar_static_map_offset = read_le32(raw_header + 68);
    } else {
        gMenuLayer.detail_top_bar_map_offset = 0U;
        gMenuLayer.detail_bottom_bar_static_map_offset = 0U;
    }

    if (gMenuLayer.version >= 5U) {
        gMenuLayer.title_scroll_table_offset = read_le32(raw_header + 72);
        gMenuLayer.title_scroll_data_offset = read_le32(raw_header + 76);
    } else {
        gMenuLayer.title_scroll_table_offset = 0U;
        gMenuLayer.title_scroll_data_offset = 0U;
    }

    if (gMenuLayer.version == 0U) {
        return 0U;
    }
    if (gMenuLayer.ui_tile_count == 0U || gMenuLayer.ui_tile_count > MENU_UI_MAX_TILES) {
        return 0U;
    }
    if (gMenuLayer.dynamic_max_tile_count == 0U || gMenuLayer.dynamic_max_tile_count > MENU_DYNAMIC_MAX_TILES) {
        return 0U;
    }
    if (gMenuLayer.page_count == 0U || gMenuLayer.rows_per_page == 0U) {
        return 0U;
    }
    if (gMenuLayer.game_count == 0U || gMenuLayer.game_count != GAME_COUNT) {
        return 0U;
    }
    gPageLabelTileBase = MENU_PAGE_LABEL_TILE_BASE;
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static void load_blank_tile_to_vram(void) {
    uint8_t blank_tile[MENU_TILE_BYTES];
    memset(blank_tile, 0, sizeof(blank_tile));
    set_bkg_data(MENU_UI_TILE_BASE, 1U, blank_tile);
}

static uint8_t load_ui_tiles_to_vram(void) {
    uint16_t tile_bytes = (uint16_t)gMenuLayer.ui_tile_count * MENU_TILE_BYTES;

    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET + gMenuLayer.ui_tile_data_offset, gPageTileData, tile_bytes);
    set_bkg_data(MENU_UI_TILE_BASE, gMenuLayer.ui_tile_count, gPageTileData);
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t load_compact_glyphs(void) {
    uint16_t glyph_bytes_to_read = (uint16_t)MENU_COMPACT_GLYPH_COUNT * gCompactGlyphBytesPerChar;

    memset(gCompactGlyphRows, 0, sizeof(gCompactGlyphRows));
    if (glyph_bytes_to_read > sizeof(gCompactGlyphRows)) {
        glyph_bytes_to_read = sizeof(gCompactGlyphRows);
    }
    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.page_label_map_offset,
        gCompactGlyphRows,
        glyph_bytes_to_read
    );
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t load_top_bar(void) {
    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.top_bar_map_offset,
        gTopBarTileMap,
        MENU_TOP_BAR_TILEMAP_SIZE
    );
    set_bkg_tiles(0U, MENU_TOP_BAR_Y, MENU_SCREEN_TILES_W, MENU_TOP_BAR_TILES_H, gTopBarTileMap);
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t load_bottom_bar_static(void) {
    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.bottom_bar_static_map_offset,
        gBottomBarTileMap,
        MENU_BOTTOM_BAR_TILEMAP_SIZE
    );
    set_bkg_tiles(0U, MENU_BOTTOM_BAR_Y, MENU_SCREEN_TILES_W, MENU_BOTTOM_BAR_TILES_H, gBottomBarTileMap);
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t compact_glyph_index_for_char(char ch) {
    if (ch >= '0' && ch <= '9') {
        return (uint8_t)(ch - '0');
    }
    if (ch == '/') {
        return 10U;
    }
    if (ch == '.') {
        return 11U;
    }
    if (ch == 'M') {
        return 12U;
    }
    if (ch == 'B') {
        return 13U;
    }
    if (ch == 'C') {
        return 14U;
    }
    return 0xFFU;
}

static uint16_t compact_glyph_row_bits(const uint8_t* glyph_rows, uint8_t row) {
    uint16_t row_bits = 0U;
    uint8_t byte_index;
    uint16_t row_offset = (uint16_t)row * gCompactGlyphRowBytes;
    uint8_t unused_bits;

    for (byte_index = 0U; byte_index < gCompactGlyphRowBytes; byte_index++) {
        row_bits = (uint16_t)((row_bits << 8) | glyph_rows[row_offset + byte_index]);
    }

    unused_bits = (uint8_t)(gCompactGlyphRowBytes * 8U);
    if (unused_bits > gCompactGlyphSizePx) {
        row_bits >>= (unused_bits - gCompactGlyphSizePx);
    }
    return row_bits;
}

static uint8_t compact_glyph_advance_px(const uint8_t* glyph_rows) {
    int8_t right = -1;
    uint8_t row;
    uint8_t col;

    for (row = 0U; row < gCompactGlyphSizePx; row++) {
        uint16_t row_bits = compact_glyph_row_bits(glyph_rows, row);
        for (col = 0U; col < gCompactGlyphSizePx; col++) {
            if ((row_bits & (1U << (gCompactGlyphSizePx - 1U - col))) != 0U && (int8_t)col > right) {
                right = (int8_t)col;
            }
        }
    }

    if (right < 0) {
        return 6U;
    }
    return (uint8_t)((((uint8_t)right + 1U) * 12U + gCompactGlyphSizePx - 1U) / gCompactGlyphSizePx);
}

static void clear_page_label_buffer(void) {
    uint8_t tile_index;

    memset(gPageLabelTileData, 0, (uint16_t)MENU_PAGE_LABEL_TILEMAP_SIZE * MENU_TILE_BYTES);
    for (tile_index = 0U; tile_index < MENU_PAGE_LABEL_TILEMAP_SIZE; tile_index++) {
        gPageLabelTileMap[tile_index] = (uint8_t)(gPageLabelTileBase + tile_index);
    }
}

static void plot_page_label_pixel(uint8_t x, uint8_t y) {
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t tile_index;
    uint8_t row_in_tile;
    uint8_t col_in_tile;
    uint16_t tile_offset;

    if (x >= (MENU_PAGE_LABEL_TILES_W * 8U) || y >= (MENU_PAGE_LABEL_TILES_H * 8U)) {
        return;
    }

    tile_x = (uint8_t)(x >> 3);
    tile_y = (uint8_t)(y >> 3);
    tile_index = (uint8_t)(tile_y * MENU_PAGE_LABEL_TILES_W + tile_x);
    row_in_tile = (uint8_t)(y & 0x07U);
    col_in_tile = (uint8_t)(x & 0x07U);
    tile_offset = (uint16_t)tile_index * MENU_TILE_BYTES + (uint16_t)row_in_tile * 2U;

    gPageLabelTileData[tile_offset] |= (uint8_t)(0x80U >> col_in_tile);
    gPageLabelTileData[(uint16_t)(tile_offset + 1U)] = gPageLabelTileData[tile_offset];
}

static void draw_compact_label_text(const char* text) {
    uint8_t advances[MENU_COMPACT_LABEL_CHAR_COUNT];
    uint8_t glyph_indices[MENU_COMPACT_LABEL_CHAR_COUNT];
    uint8_t length = 0U;
    uint8_t total_width = 0U;
    uint8_t cursor_x;
    uint8_t i;

    while (text[length] != '\0' && length < MENU_COMPACT_LABEL_CHAR_COUNT) {
        uint8_t glyph_index = compact_glyph_index_for_char(text[length]);
        const uint8_t* glyph_rows;
        uint8_t advance;

        if (glyph_index == 0xFFU) {
            return;
        }

        glyph_indices[length] = glyph_index;
        glyph_rows = &gCompactGlyphRows[(uint16_t)glyph_index * gCompactGlyphBytesPerChar];
        advance = compact_glyph_advance_px(glyph_rows);
        advances[length] = advance;
        total_width = (uint8_t)(total_width + advance);
        if (text[length + 1U] != '\0') {
            total_width = (uint8_t)(total_width + 2U);
        }
        length++;
    }

    if (length == 0U) {
        return;
    }

    if (total_width >= ((MENU_PAGE_LABEL_TILES_W * 8U) - MENU_PAGE_LABEL_RIGHT_MARGIN_PX)) {
        cursor_x = 0U;
    } else {
        cursor_x = (uint8_t)(((MENU_PAGE_LABEL_TILES_W * 8U) - MENU_PAGE_LABEL_RIGHT_MARGIN_PX) - total_width);
    }

    for (i = 0U; i < length; i++) {
        const uint8_t* glyph_rows = &gCompactGlyphRows[(uint16_t)glyph_indices[i] * gCompactGlyphBytesPerChar];
        uint8_t sy;
        uint8_t sx;

        for (sy = 0U; sy < gCompactGlyphSizePx; sy++) {
            uint16_t row_bits = compact_glyph_row_bits(glyph_rows, sy);
            uint8_t y0 = (uint8_t)((sy * 12U) / gCompactGlyphSizePx);
            uint8_t y1 = (uint8_t)((((uint16_t)(sy + 1U) * 12U) + gCompactGlyphSizePx - 1U) / gCompactGlyphSizePx);

            if (y1 <= y0) {
                y1 = (uint8_t)(y0 + 1U);
            }

            for (sx = 0U; sx < gCompactGlyphSizePx; sx++) {
                uint8_t x0;
                uint8_t x1;
                uint8_t py;
                uint8_t px;

                if ((row_bits & (1U << (gCompactGlyphSizePx - 1U - sx))) == 0U) {
                    continue;
                }

                x0 = (uint8_t)((sx * 12U) / gCompactGlyphSizePx);
                x1 = (uint8_t)((((uint16_t)(sx + 1U) * 12U) + gCompactGlyphSizePx - 1U) / gCompactGlyphSizePx);
                if (x1 <= x0) {
                    x1 = (uint8_t)(x0 + 1U);
                }

                for (py = y0; py < y1; py++) {
                    for (px = x0; px < x1; px++) {
                        plot_page_label_pixel(
                            (uint8_t)(cursor_x + px),
                            (uint8_t)(MENU_PAGE_LABEL_BASELINE_Y + py)
                        );
                    }
                }
            }
        }

        cursor_x = (uint8_t)(cursor_x + advances[i]);
        if ((uint8_t)(i + 1U) < length) {
            cursor_x = (uint8_t)(cursor_x + 2U);
        }
    }
}

static void format_two_digit_value(uint8_t value, char* out) {
    if (value > 99U) {
        value = 99U;
    }
    out[0] = (char)('0' + (value / 10U));
    out[1] = (char)('0' + (value % 10U));
}

static uint8_t get_active_game_mbc(uint8_t index) {
    if (index >= GAME_COUNT) {
        return MBC5_TYPE;
    }
    return GameMbcOverrides[index] != 0U ? GameMbcOverrides[index] : GameMBCs[index];
}

static uint8_t get_mbc_dialog_option_for_type(uint8_t mbc_type) {
    switch (mbc_type) {
        case 0x01U:
        case 0x02U:
        case 0x03U:
            return 0U;
        default:
            return 1U;
    }
}

static uint8_t get_mbc_type_for_dialog_option(uint8_t option) {
    return option == 0U ? MBC1_TYPE : MBC5_TYPE;
}

static uint8_t load_page_label(uint8_t current_value, uint8_t total_value) {
    char label_text[6];

    if (current_value == 0U || total_value == 0U) {
        return 0U;
    }

    format_two_digit_value(current_value, &label_text[0]);
    label_text[2] = '/';
    format_two_digit_value(total_value, &label_text[3]);
    label_text[5] = '\0';

    clear_page_label_buffer();
    draw_compact_label_text(label_text);

    set_bkg_data(gPageLabelTileBase, MENU_PAGE_LABEL_TILE_CAP, gPageLabelTileData);
    set_bkg_tiles(MENU_PAGE_LABEL_X, MENU_PAGE_LABEL_Y, MENU_PAGE_LABEL_TILES_W, MENU_PAGE_LABEL_TILES_H, gPageLabelTileMap);
    return 1U;
}

static void clear_dialog_buffer(uint8_t tile_base, uint8_t* tile_data, uint8_t* tile_map) {
    uint8_t tile_index;

    memset(tile_data, 0, (uint16_t)MBC_DIALOG_TILEMAP_SIZE * MENU_TILE_BYTES);
    for (tile_index = 0U; tile_index < MBC_DIALOG_TILEMAP_SIZE; tile_index++) {
        tile_map[tile_index] = (uint8_t)(tile_base + tile_index);
    }
}

static void plot_dialog_pixel(uint8_t x, uint8_t y, uint8_t* tile_data) {
    uint8_t tile_x;
    uint8_t tile_y;
    uint8_t tile_index;
    uint8_t row_in_tile;
    uint8_t col_in_tile;
    uint16_t tile_offset;

    if (x >= MBC_DIALOG_PIXEL_W || y >= MBC_DIALOG_PIXEL_H) {
        return;
    }

    tile_x = (uint8_t)(x >> 3);
    tile_y = (uint8_t)(y >> 3);
    tile_index = (uint8_t)(tile_y * MBC_DIALOG_TILES_W + tile_x);
    row_in_tile = (uint8_t)(y & 0x07U);
    col_in_tile = (uint8_t)(x & 0x07U);
    tile_offset = (uint16_t)tile_index * MENU_TILE_BYTES + (uint16_t)row_in_tile * 2U;

    tile_data[tile_offset] |= (uint8_t)(0x80U >> col_in_tile);
    tile_data[(uint16_t)(tile_offset + 1U)] = tile_data[tile_offset];
}

static void draw_dialog_border_pixels(uint8_t* tile_data) {
    uint8_t x;
    uint8_t y;

    for (x = 0U; x < MBC_DIALOG_PIXEL_W; x++) {
        plot_dialog_pixel(x, 0U, tile_data);
        plot_dialog_pixel(x, (uint8_t)(MBC_DIALOG_PIXEL_H - 1U), tile_data);
    }

    for (y = 0U; y < MBC_DIALOG_PIXEL_H; y++) {
        plot_dialog_pixel(0U, y, tile_data);
        plot_dialog_pixel((uint8_t)(MBC_DIALOG_PIXEL_W - 1U), y, tile_data);
    }
}

static void draw_dialog_compact_text(const char* text, uint8_t x, uint8_t y, uint8_t* tile_data) {
    uint8_t cursor_x = x;
    uint8_t i = 0U;

    while (text[i] != '\0') {
        uint8_t glyph_index = compact_glyph_index_for_char(text[i]);
        const uint8_t* glyph_rows;
        uint8_t advance;
        uint8_t sy;
        uint8_t sx;

        if (glyph_index == 0xFFU) {
            i++;
            continue;
        }

        glyph_rows = &gCompactGlyphRows[(uint16_t)glyph_index * gCompactGlyphBytesPerChar];
        advance = compact_glyph_advance_px(glyph_rows);

        for (sy = 0U; sy < gCompactGlyphSizePx; sy++) {
            uint16_t row_bits = compact_glyph_row_bits(glyph_rows, sy);
            uint8_t y0 = (uint8_t)((sy * 12U) / gCompactGlyphSizePx);
            uint8_t y1 = (uint8_t)((((uint16_t)(sy + 1U) * 12U) + gCompactGlyphSizePx - 1U) / gCompactGlyphSizePx);

            if (y1 <= y0) {
                y1 = (uint8_t)(y0 + 1U);
            }

            for (sx = 0U; sx < gCompactGlyphSizePx; sx++) {
                uint8_t x0;
                uint8_t x1;
                uint8_t py;
                uint8_t px;

                if ((row_bits & (1U << (gCompactGlyphSizePx - 1U - sx))) == 0U) {
                    continue;
                }

                x0 = (uint8_t)((sx * 12U) / gCompactGlyphSizePx);
                x1 = (uint8_t)((((uint16_t)(sx + 1U) * 12U) + gCompactGlyphSizePx - 1U) / gCompactGlyphSizePx);
                if (x1 <= x0) {
                    x1 = (uint8_t)(x0 + 1U);
                }

                for (py = y0; py < y1; py++) {
                    for (px = x0; px < x1; px++) {
                        plot_dialog_pixel((uint8_t)(cursor_x + px), (uint8_t)(y + py), tile_data);
                    }
                }
            }
        }

        cursor_x = (uint8_t)(cursor_x + advance);
        if (text[i + 1U] != '\0') {
            cursor_x = (uint8_t)(cursor_x + 2U);
        }
        i++;
    }
}

static uint8_t render_mbc_dialog(void) {
    uint8_t tile_base;
    uint8_t tile_count;
    uint8_t* tile_data;
    uint8_t tile_map[MBC_DIALOG_TILEMAP_SIZE];

    if (gCurrentDynamicTileCount >= MENU_DYNAMIC_MAX_TILES) {
        return 0U;
    }

    tile_count = MBC_DIALOG_TILEMAP_SIZE;
    if ((uint8_t)(MENU_DYNAMIC_MAX_TILES - gCurrentDynamicTileCount) < tile_count) {
        return 0U;
    }

    tile_base = (uint8_t)(MENU_DYNAMIC_TILE_BASE + gCurrentDynamicTileCount);
    tile_data = &gPageTileData[(uint16_t)gCurrentDynamicTileCount * MENU_TILE_BYTES];
    clear_dialog_buffer(tile_base, tile_data, tile_map);
    draw_dialog_border_pixels(tile_data);
    draw_dialog_compact_text("MBC1", 18U, 8U, tile_data);
    draw_dialog_compact_text("MBC5", 18U, 22U, tile_data);

    set_bkg_data(tile_base, tile_count, tile_data);
    set_bkg_tiles(MBC_DIALOG_X, MBC_DIALOG_Y, MBC_DIALOG_TILES_W, MBC_DIALOG_TILES_H, tile_map);
    return 1U;
}

static uint8_t load_list_page(uint8_t page) {
    uint32_t tile_addr;
    uint32_t map_addr;
    uint8_t page_tile_count;

    if (page >= gMenuLayer.page_count) {
        return 0U;
    }

    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.list_tile_counts_offset + page,
        &page_tile_count,
        1U
    );
    if (page_tile_count == 0U || page_tile_count > gMenuLayer.dynamic_max_tile_count) {
        return 0U;
    }

    tile_addr = FIXED_MAP_CONFIG_OFFSET + gMenuLayer.list_tile_data_offset +
        (uint32_t)page * (uint32_t)gMenuLayer.dynamic_max_tile_count * MENU_TILE_BYTES;
    map_addr = FIXED_MAP_CONFIG_OFFSET + gMenuLayer.list_tilemap_offset +
        (uint32_t)page * MENU_LIST_TILEMAP_SIZE;

    nor_read_linear_bytes(tile_addr, gPageTileData, (uint16_t)page_tile_count * MENU_TILE_BYTES);
    nor_read_linear_bytes(map_addr, gListTileMap, MENU_LIST_TILEMAP_SIZE);
    gCurrentDynamicTileCount = page_tile_count;

    set_bkg_data(MENU_DYNAMIC_TILE_BASE, page_tile_count, gPageTileData);
    set_bkg_based_tiles(
        0U,
        0U,
        MENU_SCREEN_TILES_W,
        MENU_SCREEN_TILES_H,
        gListTileMap,
        MENU_DYNAMIC_TILE_BASE
    );
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t load_detail_top_bar(void) {
    if (gMenuLayer.detail_top_bar_map_offset == 0U) {
        return 0U;
    }

    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.detail_top_bar_map_offset,
        gDetailTopBarTileMap,
        MENU_TOP_BAR_TILEMAP_SIZE
    );
    set_bkg_tiles(0U, MENU_TOP_BAR_Y, MENU_SCREEN_TILES_W, MENU_TOP_BAR_TILES_H, gDetailTopBarTileMap);
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t load_detail_bottom_bar_static(void) {
    if (gMenuLayer.detail_bottom_bar_static_map_offset == 0U) {
        return 0U;
    }

    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.detail_bottom_bar_static_map_offset,
        gDetailBottomBarTileMap,
        MENU_BOTTOM_BAR_TILEMAP_SIZE
    );
    set_bkg_tiles(0U, MENU_BOTTOM_BAR_Y, MENU_SCREEN_TILES_W, MENU_BOTTOM_BAR_TILES_H, gDetailBottomBarTileMap);
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static uint8_t load_detail_page(uint8_t index) {
    uint32_t tile_addr;
    uint32_t map_addr;
    uint8_t page_tile_count;

    if (index >= GAME_COUNT ||
        gMenuLayer.detail_tile_counts_offset == 0U ||
        gMenuLayer.detail_tile_data_offset == 0U ||
        gMenuLayer.detail_tilemap_offset == 0U) {
        return 0U;
    }

    if ((LCDC_REG & LCDCF_ON) != 0U) {
        wait_vbl_done();
    }

    nor_read_linear_bytes(
        FIXED_MAP_CONFIG_OFFSET + gMenuLayer.detail_tile_counts_offset + index,
        &page_tile_count,
        1U
    );
    if (page_tile_count == 0U || page_tile_count > gMenuLayer.dynamic_max_tile_count) {
        return 0U;
    }

    tile_addr = FIXED_MAP_CONFIG_OFFSET + gMenuLayer.detail_tile_data_offset +
        (uint32_t)index * (uint32_t)gMenuLayer.dynamic_max_tile_count * MENU_TILE_BYTES;
    map_addr = FIXED_MAP_CONFIG_OFFSET + gMenuLayer.detail_tilemap_offset +
        (uint32_t)index * MENU_DETAIL_TILEMAP_SIZE;

    nor_read_linear_bytes(tile_addr, gPageTileData, (uint16_t)page_tile_count * MENU_TILE_BYTES);
    nor_read_linear_bytes(map_addr, gDetailTileMap, MENU_DETAIL_TILEMAP_SIZE);
    gCurrentDynamicTileCount = page_tile_count;

    set_bkg_data(MENU_DYNAMIC_TILE_BASE, page_tile_count, gPageTileData);
    set_bkg_based_tiles(
        0U,
        0U,
        MENU_SCREEN_TILES_W,
        MENU_SCREEN_TILES_H,
        gDetailTileMap,
        MENU_DYNAMIC_TILE_BASE
    );
    switch_rom_bank(MENU_RUNTIME_ROM_BANK);
    return 1U;
}

static void clear_list_region(void) {
    fill_bkg_rect(
        MENU_LIST_REGION_X,
        MENU_LIST_REGION_Y,
        MENU_LIST_REGION_TILES_W,
        MENU_LIST_REGION_TILES_H,
        MENU_UI_TILE_BASE
    );
}

static void clear_detail_content_region(void) {
    fill_bkg_rect(0U, MENU_LIST_REGION_Y, MENU_SCREEN_TILES_W, MENU_LIST_REGION_TILES_H, MENU_UI_TILE_BASE);
}

static uint8_t load_page_content(uint8_t page) {
    if ((LCDC_REG & LCDCF_ON) != 0U) {
        wait_vbl_done();
    }

    if (!load_list_page(page)) {
        show_no_menu_layers_screen();
        return 0U;
    }

    return 1U;
}

static void show_no_config_screen(void) {
    font_init();
    font_set(font_load(font_ibm));
    cls();
    gotoxy(3U, 7U);
    printf("NO CONFIG");
    gotoxy(1U, 9U);
    printf("FLASH MENU DATA");
    SHOW_BKG;
}

static void show_no_menu_layers_screen(void) {
    HIDE_SPRITES;
    font_init();
    font_set(font_load(font_ibm));
    cls();
    gotoxy(2U, 7U);
    printf("NO MENU LAYERS");
    gotoxy(2U, 9U);
    printf("CHECK BUILDER");
    SHOW_BKG;
}

static void move_arrow_wrapped(int direction) {
    int new_index = arrow_index + direction;
    uint8_t games_per_page = get_menu_games_per_page();
    uint8_t games_on_current_page = get_games_on_page(current_page);

    if (GAME_COUNT == 0U || games_on_current_page == 0U) {
        return;
    }

    if (new_index >= 0 && new_index < games_on_current_page) {
        play_move_sound();
        arrow_index = new_index;
        game_index = (uint8_t)(current_page * games_per_page + (uint8_t)arrow_index);
    } else if (direction > 0 && (uint8_t)(current_page + 1U) < TOTAL_PAGES) {
        play_move_sound();
        current_page++;
        arrow_index = 0;
        game_index = (uint8_t)(current_page * games_per_page);
        clear_list_region();
        if (!load_page_content(current_page)) {
            return;
        }
    } else if (direction < 0 && current_page > 0U) {
        play_move_sound();
        current_page--;
        games_on_current_page = get_games_on_page(current_page);
        if (games_on_current_page == 0U) {
            arrow_index = 0;
            game_index = (uint8_t)(current_page * games_per_page);
        } else {
            arrow_index = (int)(games_on_current_page - 1U);
            game_index = (uint8_t)(current_page * games_per_page + games_on_current_page - 1U);
        }
        clear_list_region();
        if (!load_page_content(current_page)) {
            return;
        }
    } else {
        return;
    }

    move_sprite(0U, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
}

static void change_page_safe(int direction) {
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

    play_page_sound();
    current_page = (uint8_t)new_page;
    if (saved_arrow_idx >= games_on_this_page) {
        arrow_index = (int)(games_on_this_page - 1U);
    } else {
        arrow_index = (int)saved_arrow_idx;
    }

    game_index = (uint8_t)(current_page * get_menu_games_per_page() + (uint8_t)arrow_index);
    clear_list_region();
    if (!load_page_content(current_page)) {
        return;
    }

    move_sprite(0U, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
}

static void change_detail_selection(int direction) {
    int new_index;

    if (GAME_COUNT == 0U) {
        return;
    }

    new_index = (int)game_index + direction;
    if (new_index < 0 || new_index >= GAME_COUNT) {
        return;
    }

    game_index = (uint8_t)new_index;
    sync_menu_selection_from_game_index();
    clear_detail_content_region();
    if (!load_detail_page(game_index)) {
        show_no_menu_layers_screen();
        return;
    }

    play_move_sound();
}

static void show_detail_page(void) {
    if (GAME_COUNT == 0U || game_index >= GAME_COUNT) {
        return;
    }

    HIDE_SPRITES;
    current_state = DETAIL_PAGE_STATE;
    fill_bkg_rect(0U, 0U, MENU_SCREEN_TILES_W, MENU_SCREEN_TILES_H, MENU_UI_TILE_BASE);
    if (!load_detail_page(game_index)) {
        show_no_menu_layers_screen();
    }
}

static void show_mbc_selection_dialog(void) {
    uint8_t option;
    uint8_t dialog_cooldown = 0U;

    if (GAME_COUNT == 0U || game_index >= GAME_COUNT) {
        return;
    }

    option = get_mbc_dialog_option_for_type(get_active_game_mbc(game_index));
    if (!render_mbc_dialog()) {
        return;
    }

    move_sprite(0U, MBC_DIALOG_ARROW_X, option == 0U ? MBC_DIALOG_ARROW_Y0 : MBC_DIALOG_ARROW_Y1);
    SHOW_SPRITES;
    play_confirm_sound();

    while (1) {
        uint8_t keys = joypad();

        if (dialog_cooldown > 0U) {
            dialog_cooldown--;
            keys = 0U;
        }

        if (keys & (J_UP | J_LEFT)) {
            play_move_sound();
            option = option == 0U ? (MBC_DIALOG_OPTION_COUNT - 1U) : (uint8_t)(option - 1U);
            //render_mbc_dialog();
            move_sprite(0U, MBC_DIALOG_ARROW_X, option == 0U ? MBC_DIALOG_ARROW_Y0 : MBC_DIALOG_ARROW_Y1);
            dialog_cooldown = MENU_INPUT_COOLDOWN_FRAMES;
        } else if (keys & (J_DOWN | J_RIGHT)) {
            play_move_sound();
            option = (uint8_t)((option + 1U) % MBC_DIALOG_OPTION_COUNT);
            //render_mbc_dialog();
            move_sprite(0U, MBC_DIALOG_ARROW_X, option == 0U ? MBC_DIALOG_ARROW_Y0 : MBC_DIALOG_ARROW_Y1);
            dialog_cooldown = MENU_INPUT_COOLDOWN_FRAMES;
        } else if (keys & J_A) {
            play_confirm_sound();
            delay(MENU_CONFIRM_SOUND_WAIT_MS);
            HIDE_SPRITES;
            GameMbcOverrides[game_index] = get_mbc_type_for_dialog_option(option);
            into_game();
            return;
    } else if (keys & (J_B | J_START)) {
        play_cancel_sound();
        HIDE_SPRITES;
        clear_detail_content_region();
        if (!load_detail_page(game_index)) {
            show_no_menu_layers_screen();
            return;
        }
        return;
    }

        wait_vbl_done();
    }
}

static void handle_main_menu_input(uint8_t keys) {
    if (keys & J_DOWN) {
        move_arrow_wrapped(1);
    } else if (keys & J_UP) {
        move_arrow_wrapped(-1);
    } else if (keys & J_RIGHT) {
        change_page_safe(1);
    } else if (keys & J_LEFT) {
        change_page_safe(-1);
    } else if (keys & J_A) {
        play_detail_sound();
        show_detail_page();
    }
}

static void handle_detail_page_input(uint8_t keys) {
    if (keys & J_B) {
        play_cancel_sound();
        return_to_main_menu();
    } else if (keys & J_A) {
        play_confirm_sound();
        delay(MENU_CONFIRM_SOUND_WAIT_MS);
        into_game();
    } else if (keys & J_RIGHT) {
        change_detail_selection(1);
    } else if (keys & J_LEFT) {
        change_detail_selection(-1);
    } else if (keys & J_START) {
        show_mbc_selection_dialog();
    } else if (keys & J_DOWN) {
        change_detail_selection((int)DETAIL_PAGE_VERTICAL_STEP);
    } else if (keys & J_UP) {
        change_detail_selection(-(int)DETAIL_PAGE_VERTICAL_STEP);
    }
}

static void return_to_main_menu(void) {
    current_state = MAIN_MENU_STATE;
    fill_bkg_rect(0U, 0U, MENU_SCREEN_TILES_W, MENU_SCREEN_TILES_H, MENU_UI_TILE_BASE);
    if (!load_page_content(current_page)) {
        return;
    }

    if (GAME_COUNT > 0U) {
        move_sprite(0U, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    }
}

static void run_main_menu(void) {
    uint8_t config_test[7];
    uint8_t derived_pages;

    DISPLAY_OFF;
    HIDE_WIN;
    HIDE_SPRITES;
    SPRITES_8x8;
    BGP_REG = DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK);
    OBP0_REG = DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK);

    font_init();
    font_set(font_load(font_ibm));
    cls();
    gotoxy(5U, 8U);
    printf("LOADING...");
    SHOW_BKG;
    DISPLAY_ON;

    nor_read_linear_bytes(FIXED_MAP_CONFIG_OFFSET, config_test, 7U);
    if (memcmp(config_test, "MBCXCFG", 7) != 0) {
        show_no_config_screen();
        while (1) {
            wait_vbl_done();
        }
    }

    if (!load_fixed_directory() || !load_menu_layer_header()) {
        show_no_menu_layers_screen();
        while (1) {
            wait_vbl_done();
        }
    }

    derived_pages = (uint8_t)((GAME_COUNT + get_menu_games_per_page() - 1U) / get_menu_games_per_page());
    if (derived_pages == 0U || gMenuLayer.page_count < derived_pages) {
        show_no_menu_layers_screen();
        while (1) {
            wait_vbl_done();
        }
    }
    TOTAL_PAGES = derived_pages;

    if (GAME_COUNT == 0U) {
        current_page = 0U;
        game_index = 0U;
        arrow_index = 0;
    } else {
        sync_menu_selection_from_game_index();
    }

    DISPLAY_OFF;

    load_blank_tile_to_vram();
    if (!load_compact_glyphs()) {
        DISPLAY_ON;
        show_no_menu_layers_screen();
        while (1) {
            wait_vbl_done();
        }
    }

    fill_bkg_rect(0U, 0U, MENU_SCREEN_TILES_W, MENU_SCREEN_TILES_H, MENU_UI_TILE_BASE);
    if (!load_page_content(current_page)) {
        DISPLAY_ON;
        show_no_menu_layers_screen();
        while (1) {
            wait_vbl_done();
        }
    }

    set_sprite_data(MENU_ARROW_TILE_INDEX, 1U, arrow);
    set_sprite_tile(0U, MENU_ARROW_TILE_INDEX);

    if (GAME_COUNT > 0U) {
        move_sprite(0U, ARROW_START_X, get_menu_arrow_y_px((uint8_t)arrow_index));
        SHOW_SPRITES;
    }

    SHOW_BKG;
    DISPLAY_ON;

    while (1) {
        uint8_t keys = joypad();

        if (gInputCooldown > 0U) {
            gInputCooldown--;
            keys = 0U;
        } else if (keys != 0U) {
            gInputCooldown = MENU_INPUT_COOLDOWN_FRAMES;
        }

        if (current_state == MAIN_MENU_STATE) {
            handle_main_menu_input(keys);
        } else if (current_state == DETAIL_PAGE_STATE) {
            handle_detail_page_input(keys);
        }

        wait_vbl_done();
    }
}

static void into_game(void) {
    uint8_t hram_code[] = {
        0x3E, 0x01,
        0xEA, 0x00, 0xA0,
        0x00, 0x00,
        0x00, 0x00,
        0x3E, 0x00,
        0xEA, 0x00, 0x40,
        0xE1,
        0x31, 0xFE, 0xFF,
        0xC3, 0x00, 0x01
    };
    uint8_t rom_bank_high;
    uint8_t rom_bank_low;
    uint8_t ram_map_send_idx;
    uint8_t i;

    disable_interrupts();
    DISPLAY_OFF;
    *RAM_BANK_ADDR = 0x40U;

    rom_bank_high = (uint8_t)((GameROMBanks[game_index] >> 8) & 0xFFU);
    rom_bank_low = (uint8_t)(GameROMBanks[game_index] & 0xFFU);
    *GAME_SEL_ADDR = rom_bank_low;
    *GAME_SEL_ADDR = rom_bank_high;
    *GAME_SEL_ADDR = get_active_game_mbc(game_index);

    if (GameRAMBankMaps[game_index][0] != 0xFFU && GameRAMBankMaps[game_index][0] != 0xFEU) {
        *GAME_SEL_ADDR = 0xFEU;
        for (ram_map_send_idx = 0U; ram_map_send_idx < MAX_GAME_SRAM_BANKS; ram_map_send_idx++) {
            if (GameRAMBankMaps[game_index][ram_map_send_idx] == 0xFFU ||
                GameRAMBankMaps[game_index][ram_map_send_idx] == 0xFEU) {
                break;
            }
            *GAME_SEL_ADDR = GameRAMBankMaps[game_index][ram_map_send_idx];
        }
    }

    *GAME_SEL_ADDR = 0xFFU;

    for (i = 0U; i < sizeof(hram_code); i++) {
        GB_HRAM_ADDR[i] = hram_code[i];
    }

    LCDC_REG = 0x91U;
    ((void (*)(void))0xFF80)();
}

void main(void) {
    init_sound();
    run_main_menu();
}
