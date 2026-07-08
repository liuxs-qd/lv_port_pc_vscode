#include <stdio.h>
#include <math.h>
#include <stdint.h>

#define PI          3.14159265358979323846
#define SAMPLE_RATE 100000.0      // 100 KHz
#define CARRIER_FREQ 10000.0      // 10 KHz
#define TOTAL_POINTS 8192         // 8K 点
#define BASE_NOISE  30            // 基底噪声幅值

// 四个缺陷回波参数：{中心位置, 峰值幅度, 半宽（包络宽度系数）}
typedef struct {
    float center;      // 中心采样点位置
    float amplitude;   // 峰值幅度
    float width;       // 包络半宽度（控制回波宽度，越大越宽）
} EchoParams;

int main() {
    // 1. 定义缺陷回波参数
    EchoParams echos[] = {
        {100.0f, 1500.0f, 10.0f},
        {200.0f, 1300.0f, 10.0f},
        {300.0f, 1000.0f, 10.0f},
        {400.0f,  600.0f, 10.0f}
    };
    int echo_count = sizeof(echos) / sizeof(EchoParams);

    // 2. 开辟内存存放信号
    float signal_float[TOTAL_POINTS];
    uint16_t signal_uint16[TOTAL_POINTS];

    // 3. 生成信号
    for (int i = 0; i < TOTAL_POINTS; i++) {
        float t = (float)i / SAMPLE_RATE;   // 当前时间（秒）

        // 3.1 基底噪声（用小幅正弦波模拟随机噪声）
        float noise = BASE_NOISE * (0.5f + 0.5f * sinf(2.0f * PI * 1234.5f * t)); // 简单模拟，实际可用随机数

        // 3.2 正常载波信号（基底振荡）
        float carrier = BASE_NOISE * sinf(2.0f * PI * CARRIER_FREQ * t);

        float signal = carrier + noise;

        // 3.3 叠加缺陷回波（高斯包络调制）
        for (int e = 0; e < echo_count; e++) {
            float x = (float)i - echos[e].center;
            // 高斯包络：exp(-x^2 / width^2)
            float envelope = expf(-(x * x) / (echos[e].width * echos[e].width));
            // 载波：cos(2πf t + φ)，与中心频率同频
            float echo_signal = echos[e].amplitude * envelope * sinf(2.0f * PI * CARRIER_FREQ * t);
            signal += echo_signal;
        }

        // 3.4 加入少量随机噪声（模拟真实环境）
        // 此处用固定种子伪随机，实际可用rand()
        float random_noise = (float)(rand() % 100) / 100.0f * 20.0f - 10.0f;
        signal += random_noise;

        // 3.5 存储浮点值
        signal_float[i] = signal;

        // 3.6 转换为无符号整型（限幅到0~4095，模拟12位ADC）
        int int_val = (int)(signal + 2048.0f);  // 假设偏置2048
        if (int_val < 0) int_val = 0;
        if (int_val > 4095) int_val = 4095;
        signal_uint16[i] = (uint16_t)int_val;
    }

    // 4. 保存为CSV文件（便于Excel/MATLAB/Python绘图）
    FILE *fp = fopen("ultrasonic_signal.csv", "w");
    if (fp == NULL) {
        printf("文件打开失败！\n");
        return 1;
    }
    fprintf(fp, "Index,FloatValue,ADCValue\n");
    for (int i = 0; i < TOTAL_POINTS; i++) {
        fprintf(fp, "%d,%.4f,%d\n", i, signal_float[i], signal_uint16[i]);
    }
    fclose(fp);

    // 5. 也输出一组简洁的C数组（便于直接嵌入代码）
    FILE *fp_arr = fopen("signal_array.h", "w");
    if (fp_arr) {
        fprintf(fp_arr, "#ifndef SIGNAL_ARRAY_H\n#define SIGNAL_ARRAY_H\n\n");
        fprintf(fp_arr, "#define TOTAL_POINTS %d\n\n", TOTAL_POINTS);
        fprintf(fp_arr, "uint16_t test_signal[TOTAL_POINTS] = {\n");
        for (int i = 0; i < TOTAL_POINTS; i++) {
            fprintf(fp_arr, "    %d", signal_uint16[i]);
            if (i < TOTAL_POINTS - 1) fprintf(fp_arr, ",\n");
            else fprintf(fp_arr, "\n");
        }
        fprintf(fp_arr, "};\n\n#endif\n");
        fclose(fp_arr);
    }

    printf("信号生成完成！\n");
    printf("总点数: %d\n", TOTAL_POINTS);
    printf("缺陷回波中心位置: ");
    for (int e = 0; e < echo_count; e++) {
        printf("%.0f ", echos[e].center);
    }
    printf("\n缺陷回波幅度: ");
    for (int e = 0; e < echo_count; e++) {
        printf("%.0f ", echos[e].amplitude);
    }
    printf("\n\n数据已保存至: ultrasonic_signal.csv 和 signal_array.h\n");

    return 0;
}
