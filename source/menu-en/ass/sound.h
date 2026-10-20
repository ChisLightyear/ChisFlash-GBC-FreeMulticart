#ifndef SOUND_H
#define SOUND_H

#include <gb/gb.h>

// 初始化声音系统
void init_sound(void);

// 播放移动音效
void play_move_sound(void);

// 播放选择音效
void play_select_sound(void);

// 播放翻页音效
void play_page_sound(void);

// 播放翻页音效
void play_detail_sound(void);

// 播放确认音效
void play_confirm_sound(void);

// 播放取消音效
void play_cancel_sound(void);

// 播放调整数字音效
void play_adjust_sound(void);

#endif