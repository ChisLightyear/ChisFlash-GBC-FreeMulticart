# ChisFlash GBC Free Multicart — Prerelease

**Tutorial / 操作教程:** [English](TUTORIAL.en.md) | [简体中文](TUTORIAL.zh-CN.md)

**FAQ / 常见问题:** [English](FAQ.en.md) | [简体中文](FAQ.zh-CN.md)

**Download / 下载:** [GitHub Releases](https://github.com/ChisLightyear/ChisFlash-GBC-FreeMulticart/releases)

## English

Prerelease package for **MBCX or MBC5 MAX 1.2 GB/GBC cartridges with 32 MiB NOR and 512 KiB SRAM**. This is separate from the NES/MD MIX AG32 firmware.

| File | Purpose |
|---|---|
| `menus/menu_zh.gb` | Chinese menu kernel |
| `menus/menu_en.gb` | English menu kernel |
| `firmware/ChisFlash-Free-Multicart-v1.01.pof` | Free Multicart v1.01 firmware with Chinese menu support |
| `source/firmware/` | Verilog (`.v`) firmware sources |
| `source/menu-zh/`, `source/menu-en/` | Chinese and English menu C sources |

Firmware source: [ChisFlash.v](source/firmware/rtl/ChisFlash.v), with MBC modules and oscillator Verilog files in [source/firmware/](source/firmware/).

The supplied firmware is Free Multicart v1.01. The included Verilog sources have not been verified to reproduce this binary.

**Compatibility (maintainer-provided):** Currently supports common MBC1–MBC5 games. ROM information is detected automatically, and the corresponding MBC type is selected at launch. You can also select an MBC type manually before launching.

Menu kernels alone are not complete multicart images. Follow the tutorial to create the directory and add games. For Chinese, provide your own 12px font; no external font is bundled.

Screenshots use synthetic ROMs and simulated card data; menu images are software previews. No physical device was programmed.

Desktop tools: [ChisFlash-Tools-Exp](https://github.com/moribaka/ChisFlash-Tools-Exp), currently a private repository requiring access. Obtain the companion tools from the maintainer. The tutorial's GB full-erase action requires Client 2.034 or a later build that includes this feature; Client 2.031 does not include it. Offline checks and builds pass; physical-cartridge validation is pending. Tools and games are not bundled here.

Verify artifacts with [SHA256SUMS.txt](SHA256SUMS.txt). See [LICENSES](LICENSES/README.md) for third-party notices and the remaining project-licence decision.

## 简体中文

本包适用于配备 **32 MiB NOR、512 KiB SRAM 的 MBCX 或 MBC5 MAX 1.2 GB/GBC 卡带**，为自由合卡预发布版本。与 NES/MD MIX 的 AG32 固件分开使用。

| 文件 | 用途 |
|---|---|
| `menus/menu_zh.gb` | 中文菜单内核 |
| `menus/menu_en.gb` | 英文菜单内核 |
| `firmware/ChisFlash-Free-Multicart-v1.01.pof` | 支持中文菜单的自由合卡 v1.01 固件 |
| `source/firmware/` | 固件 Verilog（`.v`）源码 |
| `source/menu-zh/`、`source/menu-en/` | 中文、英文菜单 C 源码 |

固件源码入口：[ChisFlash.v](source/firmware/rtl/ChisFlash.v)，配套 MBC 模块和振荡器 Verilog 文件位于 [source/firmware/](source/firmware/)。

随附固件为自由合卡 v1.01；当前 `.v` 源码尚未验证能复现该成品。

**当前兼容性（维护者说明）：** 兼容 MBC1～MBC5 常见游戏，自动识别 ROM 文件信息，并在启动时切换到对应的 MBC 类型。也可以自行选择 MBC 类型后启动。

菜单内核不是完整合卡镜像。请按教程生成目录并添加游戏。中文菜单需自备 12px 字库，本包不附带外部字库文件。

截图使用假 ROM 和模拟卡带数据，菜单画面为软件预览，没有对真实设备烧录。

配套工具：[ChisFlash-Tools-Exp](https://github.com/moribaka/ChisFlash-Tools-Exp)，目前为私有仓库，需要访问权限；请向维护者获取配套工具。教程中的 GB 全片擦除入口需上位机 2.034 或后续包含此功能的版本，2.031 不具备；已通过离线回归与编译，尚待实卡验证。本包不附带工具或游戏。

校验见 [SHA256SUMS.txt](SHA256SUMS.txt)，第三方署名与待确认的项目许可见 [LICENSES](LICENSES/README.md)。
