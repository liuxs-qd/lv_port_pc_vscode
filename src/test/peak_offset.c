#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

// 波峰信息结构体
typedef struct {
    uint32_t index;          // 离散峰值点序号
    float    precise_index;  // 精确定位后的浮点位置
    uint16_t amplitude;      // 离散峰值幅度
    float    precise_amplitude; // 插值后的精确幅度
} PeakInfoPrecise;

// ============================================================
// 三点抛物线插值（核心算法）
// ============================================================
float parabola_peak_offset(float y0, float y1, float y2) {
    // y0: 左侧点幅度, y1: 峰值点幅度, y2: 右侧点幅度
    // 返回：真实峰值相对于 y1 位置的偏移量（-0.5 ~ +0.5）
    
    float denominator = y0 + y2 - 2.0f * y1;
    
    // 防止除零（如果信号完全平坦，则无偏移）
    if (fabs(denominator) < 1e-6) {
        return 0.0f;
    }
    
    float offset = -(y2 - y0) / (2.0f * denominator);
    
    // 限幅到合理范围（防止外推）
    if (offset < -0.5f) offset = -0.5f;
    if (offset > 0.5f)  offset = 0.5f;
    
    return offset;
}

// ============================================================
// 三点插值精确幅度计算
// ============================================================
float parabola_peak_amplitude(float y0, float y1, float y2, float offset) {
    // 根据抛物线方程 y = a*x² + b*x + c，计算顶点处的幅度
    float a = (y0 + y2 - 2.0f * y1) / 2.0f;
    float b = (y2 - y0) / 2.0f;
    float c = y1;
    
    // 顶点幅度 = c - b²/(4a)
    if (fabs(a) < 1e-6) {
        return y1;  // 近似平坦
    }
    return c - (b * b) / (4.0f * a);
}

// ============================================================
// 主波峰检测 + 精确插值函数
// ============================================================
int find_peaks_with_interpolation(
    const uint16_t *signal,
    uint32_t        len,
    uint16_t        threshold,
    PeakInfoPrecise *peaks,
    uint32_t        max_peaks
) {
    if (signal == NULL || peaks == NULL || len < 3) {
        return 0;
    }
    
    uint32_t count = 0;
    
    for (uint32_t i = 1; i < len - 1; i++) {
        // 1. 门限判断
        if (signal[i] <= threshold) continue;
        
        // 2. 离散峰值检测（左右比较）
        bool is_peak = (signal[i] > signal[i-1]) && (signal[i] >= signal[i+1]);
        if (!is_peak) continue;
        
        // 3. 检查边界（需要左右各至少1个点）
        if (i == 0 || i == len - 1) continue;
        
        // 4. 三点插值计算精确位置
        float y0 = (float)signal[i-1];
        float y1 = (float)signal[i];
        float y2 = (float)signal[i+1];
        
        float offset = parabola_peak_offset(y0, y1, y2);
        float precise_idx = (float)i + offset;
        float precise_amp = parabola_peak_amplitude(y0, y1, y2, offset);
        
        // 5. 与上一个波峰保持最小距离（防止噪声伪峰）
        if (count > 0 && (precise_idx - peaks[count-1].precise_index) < 3.0f) {
            // 保留幅度更大的波峰
            if (precise_amp > peaks[count-1].precise_amplitude) {
                peaks[count-1].index = i;
                peaks[count-1].precise_index = precise_idx;
                peaks[count-1].amplitude = signal[i];
                peaks[count-1].precise_amplitude = precise_amp;
            }
            continue;
        }
        
        // 6. 记录结果
        if (count < max_peaks) {
            peaks[count].index = i;
            peaks[count].precise_index = precise_idx;
            peaks[count].amplitude = signal[i];
            peaks[count].precise_amplitude = precise_amp;
            count++;
        } else {
            break;
        }
    }
    
    return count;
}

// ============================================================
// 测试主函数
// ============================================================
int main() {
    // 模拟采样数据：在位置 100.3 处产生一个正弦波峰
    #define LEN 256
    uint16_t signal[LEN];
    
    float true_peak_pos = 100.3f;
    float amplitude = 3000.0f;
    
    for (int i = 0; i < LEN; i++) {
        float x = (float)i - true_peak_pos;
        // 模拟高斯型波峰（接近实际超声回波包络）
        float val = amplitude * exp(-(x*x) / 8.0f);
        signal[i] = (uint16_t)(val + 100); // 加噪声基底
    }
    
    // 执行波峰检测
    PeakInfoPrecise peaks[10];
    int count = find_peaks_with_interpolation(signal, LEN, 500, peaks, 10);
    
    // 输出结果
    printf("检测到 %d 个波峰：\n", count);
    printf("离散峰值序号 | 精确位置 | 离散幅度 | 精确幅度 | 误差\n");
    printf("-------------|---------|---------|---------|------\n");
    for (int i = 0; i < count; i++) {
        float error = peaks[i].precise_index - true_peak_pos;
        printf("   %3d       | %7.3f  |  %4d   | %7.1f | %+.3f\n",
               peaks[i].index,
               peaks[i].precise_index,
               peaks[i].amplitude,
               peaks[i].precise_amplitude,
               error);
    }
    
    return 0;
}
