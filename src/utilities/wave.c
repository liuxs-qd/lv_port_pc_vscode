

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define GATE_LENGTH     128     // 闸门窗口采样点数
#define THRESHOLD       50      // 幅度阈值（固定值，实际可动态调整）

#define SOUND_SPEED     5900.0f // 钢材声速 mm/s（可根据材料调整）
#define SAMPLING_PERIOD 0.0001f // 采样周期 s（10 kHz采样率）

// 回波检测结果
typedef struct {
    uint32_t peak_index;        // 峰值在采样序列中的位置
    uint16_t peak_amplitude;    // 峰值幅度
    bool     valid;            // 是否有效回波
} EchoResult;

// 闸门状态（用于动态追踪）
typedef struct {
    uint32_t gate_start;        // 闸门起始位置
    uint32_t gate_width;        // 闸门宽度
    uint32_t last_peak_index;   // 上一帧峰值位置
} GateState;

// 函数：在指定闸门窗口内搜索回波峰值
EchoResult find_peak_in_gate(
    const uint16_t *signal,     // 输入信号数组（已包络检波）
    uint32_t        signal_len, // 信号总长度
    GateState      *gate,       // 闸门状态（含起始位置和宽度）
    uint16_t        threshold   // 幅度阈值
);


// 简化包络检波：取滑动窗口内的最大绝对值（相当于峰值保持）
void envelope_detection(
    const int16_t *raw_signal,   // 原始ADC采样数据（含符号）
    uint32_t       len,
    uint16_t      *envelope      // 输出包络（取绝对值）
) {
    for (uint32_t i = 0; i < len; i++) {
        // 取绝对值并限幅到0~65535
        int32_t val = raw_signal[i];
        envelope[i] = (val < 0) ? (uint16_t)(-val) : (uint16_t)val;
    }
}


EchoResult find_peak_in_gate(
    const uint16_t *signal,
    uint32_t        signal_len,
    GateState      *gate,
    uint16_t        threshold
) {
    EchoResult result = {0, 0, false};
    
    // 1. 计算闸门有效范围（边界保护）
    uint32_t start = gate->gate_start;
    uint32_t end = start + gate->gate_width;
    if (end >= signal_len) {
        end = signal_len - 1;
    }
    if (start >= signal_len) {
        start = signal_len - 1;
    }
    
    // 2. 在窗口内寻找峰值
    uint16_t max_amp = 0;
    uint32_t max_idx = start;
    
    for (uint32_t i = start; i <= end; i++) {
        if (signal[i] > max_amp) {
            max_amp = signal[i];
            max_idx = i;
        }
    }
    
    // 3. 有效性判断：幅度必须超过阈值
    if (max_amp > threshold) {
        result.peak_index = max_idx;
        result.peak_amplitude = max_amp;
        result.valid = true;
        
        // 4. 动态闸门追踪：将闸门移动到新峰值附近（下一帧使用）
        gate->gate_start = max_idx - gate->gate_width / 2;
        // 防止溢出
        if (gate->gate_start < 0) gate->gate_start = 0;
        if (gate->gate_start + gate->gate_width >= signal_len) {
            gate->gate_start = signal_len - gate->gate_width - 1;
        }
        gate->last_peak_index = max_idx;
    } else {
        // 未找到有效回波：闸门保持原位，等待信号恢复
        result.valid = false;
    }
    
    return result;
}

// 在包络峰值附近，寻找原始信号的过零点
uint32_t find_zero_crossing(
    const int16_t *raw_signal,  // 原始ADC数据（带符号）
    uint32_t       peak_pos,    // 包络峰值位置
    uint32_t       search_range // 搜索范围（例如±10点）
) {
    uint32_t start = (peak_pos > search_range) ? peak_pos - search_range : 0;
    uint32_t end = peak_pos + search_range;
    
    for (uint32_t i = start; i < end; i++) {
        // 检测符号变化（正变负或负变正）
        if ((raw_signal[i] >= 0 && raw_signal[i+1] < 0) ||
            (raw_signal[i] <= 0 && raw_signal[i+1] > 0)) {
            // 线性插值得到更精确的过零点位置（亚采样精度）
            float slope = (float)(raw_signal[i+1] - raw_signal[i]);
            if (slope != 0) {
                float offset = (float)(-raw_signal[i]) / slope;
                return i + (uint32_t)(offset * 1000); // 返回千分位精度
            }
            return i;
        }
    }
    return peak_pos; // 未找到过零点，返回峰值位置
}

void adc_raw(
    int16_t *raw_signal,  // 输出：原始ADC采样数据
    uint32_t len          // 长度
) {
    // 模拟生成一个带噪声的正弦波信号作为示例
    for (uint32_t i = 0; i < len; i++) {
        float t = (float)i / len * 2.0f * 3.14159f * 5.0f; // 5个周期
        float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 100; // ±50噪声
        raw_signal[i] = (int16_t)(1000.0f * sinf(t) + noise);
    }
}

// 主循环示例
int main() {
    // 1. 初始化
    GateState gate = {100, 128, 0}; // 闸门起始100，宽度128
    uint16_t threshold = 50;
    
    // 2. 假设已获取原始ADC数据 raw[1024]
    int16_t raw[1024];
    uint16_t envelope[1024];

    adc_raw(raw, 1024); // 模拟获取原始ADC数据
    
    // 3. 包络检波（FPGA已完成，此处为说明）
    envelope_detection(raw, 1024, envelope);
    
    // 4. 峰值搜索
    EchoResult result = find_peak_in_gate(envelope, 1024, &gate, threshold);
    
    if (result.valid) {
        // 5. 计算厚度（声速已在MCU中校准）
        float thickness = SOUND_SPEED * (result.peak_index * SAMPLING_PERIOD) / 2.0;
        printf("厚度: %.3f mm, 峰值位置: %d, 幅度: %d\n",
               thickness, result.peak_index, result.peak_amplitude);
    } else {
        printf("未找到有效回波，请检查探头耦合！\n");
    }
    
    return 0;
}
