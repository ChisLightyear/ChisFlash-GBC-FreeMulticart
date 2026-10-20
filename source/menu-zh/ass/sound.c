#include "sound.h"

// 菜单移动音效备份
const UBYTE move2_sound[] = {
    0x25,         // 扫频: 时间2, 下扫, 步长5 (00100101)
    0xFF,         // 75%占空比
    0xA4,         // 音量10, 下降, 步速4 (10100100)
    0x78,         // 850Hz 低8位 (计算值:2048-(131072/850)=2048-154≈1894→0x768)
    0x87          // 触发+频率高3位
};

// 菜单移动音效
const UBYTE move_sound[] = {
    0x27,         // NR10: 轻微下扫(00010110)
    0xFF,         // NR11: 75%占空比(11111111)
    0x93,         // NR12: 音量9, 下降, 步速3(10010011)
    0x8C,         // NR13: 800Hz 低8位
    0x87          // NR14: 触发+频率高3位
};

// 菜单翻页音效
const UBYTE page_sound[] = {
    0x27,         // NR10: 轻微下扫(00010110)
    0xFF,         // NR11: 75%占空比(11111111)
    0x93,         // NR12: 音量9, 下降, 步速3(10010011)
    0x8C,         // NR13: 800Hz 低8位
    0x87          // NR14: 触发+频率高3位
};

// 取消音效 (经典FC GB风格)
const UBYTE cancel_sound[] = {
    0x96,         // NR10: 扫频关闭
    0x47,         // NR11: 占空比50%，长度计数器设置为最大值
    0x93,         // NR12: 初始音量15，下降包络，步速4
    0x50,         // NR13: 频率低8位 (1932)
    0x86          // NR14: 频率高3位 + 触发位
};

// 详细页进入音效
const UBYTE detail_sound[] = {
    0x17,         // NR10: 扫频设置 - 时间1(快)，下扫，步长7 (00010111)
    0x80,         // NR11: 50%占空比 (10000000)
    0x93,         // NR12: 初始音量9，下降包络，步速3 (11110011)
    0x00,         // NR13: 频率低8位 (从低频开始)
    0x87          // NR14: 触发+频率高3位 (10000111) - 约131Hz
};

// FC菜单移动音效
const UBYTE moveFC_sound[] = {
    0x37,         // 扫频: 时间3, 下扫, 步长7 (00110111)
    0xFF,         // 75%占空比
    0xC6,         // 音量12, 下降, 步速6 (11000110)
    0x40,         // 400Hz 低8位
    0x87          // 触发+频率高3位
};

// 选择音效 - 中音调 (1500Hz)
const UBYTE select_sound[] = {
    0xFF,         // NR10: 扫频关闭
    0xBF,         // NR11: 占空比50%，长度计数器设置为最大值
    0xF3,         // NR12: 初始音量15，下降包络，步速3
    0x89,         // NR13: 频率低8位 (1929)
    0x86          // NR14: 频率高3位 + 触发位
};

// 确认音效 - 低音调 (1000Hz)
const UBYTE confirm_sound[] = {
    0x37,         // 扫频: 时间3, 下扫, 步长7 (00110111)
    0xFF,         // 75%占空比
    0xC6,         // 音量12, 下降, 步速6 (11000110)
    0x40,         // 400Hz 低8位
    0x87          // 触发+频率高3位
};

// 飞机普通子弹音效,尾音拖长
const UBYTE yinxiao1_sound[] = {
    0x9F,         // NR10: 扫频关闭
    0xFF,         // NR11: 占空比50%，长度计数器设置为最大值
    0xF4,         // NR12: 初始音量15，下降包络，步速4
    0x8C,         // NR13: 频率低8位 (1932)
    0x87          // NR14: 频率高3位 + 触发位
};

// 吃金币音效,音调尖，尾音拖长
const UBYTE adjust_sound[] = {
    0x57,         // NR10: 扫频关闭
    0xFF,         // NR11: 占空比50%，长度计数器设置为最大值
    0x93,         // NR12: 初始音量15，下降包络，步速4
    0x8C,         // NR13: 频率低8位 (1932)
    0x87          // NR14: 频率高3位 + 触发位
};

// 嘟↑的声音
const UBYTE yinxiao3_sound[] = {
    0x96,         // NR10: 扫频关闭
    0x47,         // NR11: 占空比50%，长度计数器设置为最大值
    0xE5,         // NR12: 初始音量15，下降包络，步速4
    0x50,         // NR13: 频率低8位 (1932)
    0x86          // NR14: 频率高3位 + 触发位
};

// 赌博机调数字音效2,声音闷，尾音拖长
const UBYTE yinxiao4_sound[] = {
    0x86,         // NR10: 扫频关闭
    0x47,         // NR11: 占空比50%，长度计数器设置为最大值
    0xE5,         // NR12: 初始音量15，下降包络，步速4
    0x50,         // NR13: 频率低8位 (1932)
    0x86          // NR14: 频率高3位 + 触发位
};
// 初始化声音系统
void init_sound(void) {
    NR52_REG = 0x80; // 打开声音系统主控 (bit7=1)
    NR50_REG = 0x77; // 左右声道最大音量 (01110111)
    NR51_REG = 0x11; // 只使用通道1 (00010001)
}

// 通用音效播放函数
void play_sound(const UBYTE* sound_data) {
    // 确保声音系统已开启
    NR52_REG = 0x80;
    
    // 按正确顺序写入寄存器
    NR10_REG = sound_data[0]; // 扫频设置
    NR11_REG = sound_data[1]; // 波形/长度
    NR12_REG = sound_data[2]; // 音量包络
    NR13_REG = sound_data[3]; // 频率低8位
    NR14_REG = sound_data[4]; // 频率高3位 + 触发
}

// 播放移动音效
void play_move_sound(void) {
    play_sound(move_sound);
    delay(70); // 确保音效有时间播放
}

// 播放翻页音效
void play_page_sound(void) {
    play_sound(page_sound);
    delay(10); // 确保音效有时间播放
}

// 播放翻页音效
void play_detail_sound(void) {
    play_sound(detail_sound);
    delay(10); // 确保音效有时间播放
}

// 播放取消音效
void play_cancel_sound(void) {
    // 播放两次短促音效
    play_sound(cancel_sound);
    delay(50);
}

// 播放选择音效
void play_select_sound(void) {
    play_sound(select_sound);
    delay(10);
}

// 播放调整数字音效
void play_adjust_sound(void) {
    play_sound(adjust_sound);
    delay(10);
}

// 播放确认音效
void play_confirm_sound(void) {
    play_sound(confirm_sound);
    delay(30); // 确认音效稍长一点
}