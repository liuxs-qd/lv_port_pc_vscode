/* wave.h */
#ifndef WAVE_H
#define WAVE_H

#include <stdint.h>

// 波峰信息结构体
typedef struct {
    uint32_t index;             // 波峰在数组中的位置（采样序号）
    uint16_t amplitude;         // 波峰幅度值
} PeakInfo;

// 函数声明
int find_all_peaks(
    const uint16_t *signal,     // 输入信号数组
    uint32_t        len,        // 信号长度
    uint16_t        threshold,  // 幅度门限
    PeakInfo       *peaks,      // 输出波峰数组
    uint32_t        max_peaks   // 最大允许波峰数量
);

#endif // WAVE_H