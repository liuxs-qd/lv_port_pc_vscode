

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "wave.h"

#define SAMPLE_LEN  1024        // 采样点数（可调整）
#define THRESHOLD   1000        // 闸门门限
#define ADC_MAX     4096        // ADC最大值（12位）


// 主函数示例
int _main() {
    // 模拟信号数据（实际应从ADC读取）
    uint16_t signal[SAMPLE_LEN] = {0};
    
    // 在位置 50, 200, 450, 800 处产生高斯型波峰
    int peak_pos[] = {50, 200, 450, 800};
    int peak_amp[] = {2000, 1500, 3000, 1200};
    
    for (int i = 0; i < 4; i++) {
        peak_pos[i] += rand() % 20 - 10; // 随机偏移 ±10
        peak_amp[i] += rand() % 200 - 100; // 随机偏移 ±100
    }

    // 生成模拟信号：包含多个波峰
    for (int i = 0; i < SAMPLE_LEN; i++) {
        signal[i] = 100; // 基底噪声
        for (int j = 0; j < 4; j++) {
            int dx = i - peak_pos[j];
            if (dx >= -20 && dx <= 20) {
                int val = peak_amp[j] * exp(-(dx*dx) / 100.0);
                if (val > signal[i]) signal[i] = val;
            }
        }
    }
    
    // 调用波峰检测函数
    PeakInfo peaks[100];
    int peak_count = find_all_peaks(signal, SAMPLE_LEN, THRESHOLD, peaks, 100);
    
    // 输出结果
    printf("检测到 %d 个波峰（门限：%d）：\n", peak_count, THRESHOLD);
    printf("序号 | 位置 | 幅度\n");
    printf("-----|------|------\n");
    for (int i = 0; i < peak_count; i++) {
        printf(" %3d | %5d | %4d\n", i+1, peaks[i].index, peaks[i].amplitude);
    }
    
    return 0;
}

// ============================================================
// 核心波峰检测函数
// ============================================================
int find_all_peaks(
    const uint16_t *signal,
    uint32_t        len,
    uint16_t        threshold,
    PeakInfo       *peaks,
    uint32_t        max_peaks
) {
    if (signal == NULL || peaks == NULL || len < 3) {
        return 0;
    }
    
    uint32_t count = 0;
    bool in_peak = false;   // 用于处理平顶波峰，避免重复计数
    
    for (uint32_t i = 1; i < len - 1; i++) {
        // 条件1：幅度超过门限
        if (signal[i] <= threshold) {
            in_peak = false;
            continue;
        }
        
        // 条件2：检测是否为峰值（左侧上升，右侧下降或相等）
        bool is_rising = (signal[i] > signal[i-1]);
        bool is_falling_or_flat = (signal[i] >= signal[i+1]);
        
        if (is_rising && is_falling_or_flat) {
            
            // 如果当前处于平顶区域，且已经是平顶区域的后半段，跳过（避免重复计数）
            // 但这里我们采取另一种策略：对于连续相等的情况，只取第一个下降沿之前的最后一个点
            // 更稳健：检查是否与上一个波峰位置太近（防止噪声导致的伪峰）
            if (count > 0 && (i - peaks[count-1].index) < 5) {
                // 如果新波峰与上一个波峰距离太近，保留幅度更大的那个
                if (signal[i] > peaks[count-1].amplitude) {
                    peaks[count-1].index = i;
                    peaks[count-1].amplitude = signal[i];
                }
                continue;
            }
            
            // 记录波峰
            peaks[count].index = i;
            peaks[count].amplitude = signal[i];
            count++;
            in_peak = true;
        }
        // 检查是否已有足够空间存储
        if (count >= max_peaks) {
            break;
        }
    }
    
    return count;
}


#define MIN_PEAK_DISTANCE  10   // 最小波峰间距（采样点数）
#define LOCAL_WINDOW       5    // 局部窗口半径

int find_all_peaks_robust(
    const uint16_t *signal,
    uint32_t        len,
    uint16_t        threshold,
    PeakInfo       *peaks,
    uint32_t        max_peaks
) {
    if (signal == NULL || peaks == NULL || len < 3) return 0;
    
    uint32_t count = 0;
    
    for (uint32_t i = 1; i < len - 1; i++) {
        // 跳过低于门限的点
        if (signal[i] <= threshold) continue;
        
        // 检查是否为局部最大值（左右各取 LOCAL_WINDOW 个点比较）
        bool is_local_max = true;
        uint32_t start = (i > LOCAL_WINDOW) ? i - LOCAL_WINDOW : 0;
        uint32_t end = (i + LOCAL_WINDOW < len) ? i + LOCAL_WINDOW : len - 1;
        
        for (uint32_t j = start; j <= end; j++) {
            if (j != i && signal[j] > signal[i]) {
                is_local_max = false;
                break;
            }
        }
        
        if (!is_local_max) continue;
        
        // 检查与上一个波峰的间距
        if (count > 0 && (i - peaks[count-1].index) < MIN_PEAK_DISTANCE) {
            // 保留幅度更大的波峰
            if (signal[i] > peaks[count-1].amplitude) {
                peaks[count-1].index = i;
                peaks[count-1].amplitude = signal[i];
            }
            continue;
        }
        
        // 记录波峰
        if (count < max_peaks) {
            peaks[count].index = i;
            peaks[count].amplitude = signal[i];
            count++;
        } else {
            break;
        }
    }
    
    return count;
}