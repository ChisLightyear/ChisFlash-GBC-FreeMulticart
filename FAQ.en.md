# GBC Free Multicart FAQ

**English** | [简体中文](FAQ.zh-CN.md) | [Operation Guide](TUTORIAL.en.md)

## Which Cards Does This Package Target?

**MBCX or MBC5 MAX 1.2 cartridges with 32 MiB NOR and 512 KiB SRAM**.

This POF is not for AG32, GBA cards or NES/MD MIX cards.

## How Are the Menu and Firmware Different?

The `.pof` programs the CPLD through JTAG and implements address mapping / MBC logic. `menu_zh.gb` and `menu_en.gb` are menu kernels stored in NOR.

A 32 KiB kernel does not contain the complete directory or games. Install the directory and add games with the Client, or generate a complete image with the Builder.

## Should I Use the Client or the Builder?

- **Beggar programmer:** use the Client to access the cartridge, add games, configure the menu and write directly.
- **Another compatible programmer, such as gbflash:** use the Builder to combine the menu, directory and games into one complete ROM, then write the entire file with your programmer. The Builder does not program the cartridge itself or produce just a menu kernel.

## Which Firmware Should I Use?

Use `firmware/ChisFlash-Free-Multicart-v1.01.pof`: Free Multicart v1.01 with Chinese menu support.

The included sources have not been verified to reproduce this v1.01 binary.

## Where Is the Firmware Source?

[`source/firmware/`](source/firmware/) contains `.v` files for the top level, MBC modules and oscillator. Start with [ChisFlash.v](source/firmware/rtl/ChisFlash.v).

## Which MBCs and Games Can Be Used?

Currently supports **common MBC1–MBC5 games**. ROM information is detected automatically, and the corresponding MBC type is selected at launch. You can also select an MBC type manually before launching.

This is the maintainer-provided compatibility description for the supplied firmware.

## How Many Games Fit? Can I Select 128 MiB or 256 MiB?

Up to 99 games. This firmware addresses 32 MiB; the first 1 MiB is reserved for the menu / directory, leaving about 31 MiB for games.

Other capacities may appear in the tools, but this package provides no verified firmware for 128 MiB / 256 MiB hardware. Do not use a larger setting to bypass the target's limits.

## What Is Dedicated Versus Shared Save Allocation?

Dedicated allocation gives a game its own SRAM banks for progress that must be retained. Shared space is for data that need not be retained across games.

There are 64 banks of 8 KiB: 60 for dedicated allocation and four shared banks. Each game can map up to eight banks (64 KiB); the shared area is 32 KiB. Reduce games or adjust allocation if space runs out; do not share important saves.

## Does Dedicated Allocation Mean Batteryless Saving?

No. Allocation prevents games overwriting each other; it does not make ordinary SRAM nonvolatile. Power-off retention still depends on the card's battery or actual storage hardware.

This package does not supply a GBA-style batteryless patch, and a dedicated-save checkbox cannot replace missing save hardware.

## Do GB/GBC Games Need SRAM, Batteryless or WAITCNT Patches?

Do not apply the GBA versions of these patches. Game headers, MBC logic and SRAM mappings determine this workflow.

An unsupported MBC needs suitable firmware or a game-specific conversion. Changing the displayed type does not automatically provide hardware compatibility.

## How Do I Switch Chinese and English?

Select `Menu Language` in the Client, then update or burn the menu. In the Builder, choose `menu_zh.gb` or `menu_en.gb` as `Base Menu`.

The desktop language controls the tool UI; it does not mean the card has been updated. English uses built-in ASCII glyphs. For Chinese, provide your own 12px font; the tool generates the glyphs and menu layers.

## How Do I Prepare a Chinese Font?

Provide a 12px pixel font covering the Chinese characters you need. TTF, OTF and BDF are supported. Copy it into the tool's `Assets/Fonts` directory, reopen the tool, then select it in the menu preview or the Builder's font list.

Chinese menus require a 12px font. 8px fonts are not listed and cannot be used for previews or menu generation. TTF/OTF filenames must include `12px` or `_12` (for example, `myfont_12.ttf`); fonts without a size marker are rejected. BDF files must declare a 12px glyph size in their contents. Follow the font's licence.

## Why Does It Say NO MENU LAYERS or Show No Games?

An empty installation, a standalone kernel or incomplete directory / layers can cause this.

On a confirmed blank card, add at least one game and rebuild / burn. If a used card unexpectedly loses its list, analyze and back it up before clearing configuration or erasing NOR.

## Does Updating the Menu Erase Games and Saves?

Choose No to retain the directory, and export saves beforehand. Clearing configuration erases the directory and rewrites the head 1 MiB. ROM bytes remaining afterwards do not guarantee that games remain listed.

For old formats, damaged directories or another target card, export ROMs / saves, rebuild, and restore each save to its matching game.

## What About Missing Chinese Glyphs or Long Titles?

Select a font covering the characters in the preview, or shorten display titles, then update / burn the menu. Display titles can change without renaming the original ROM files.

Prefer supported ASCII titles for the English menu. Screenshots and software previews check appearance; launching and save retention still require a real-card check.
