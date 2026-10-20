# 菜单构建 / Menu Build

中文、英文菜单源码位于 `source/menu-zh/` 和 `source/menu-en/`，使用 **GBDK 4.4.0**。

Chinese and English menu sources are in `source/menu-zh/` and `source/menu-en/`. Use **GBDK 4.4.0**.

在包根目录运行，输出目录放在包外。Run from the package root with an external output directory:

```powershell
.\Build-Menus.ps1 -GbdkRoot <GBDK-directory> -OutputDirectory <menu-build-directory>
```

输出为 `menu_zh.gb` 和 `menu_en.gb`，每份 32,768 字节。

Outputs: `menu_zh.gb` and `menu_en.gb`, each 32,768 bytes.

固件 Verilog 源码见 [source/firmware/](source/firmware/)。Firmware Verilog sources: [source/firmware/](source/firmware/).
