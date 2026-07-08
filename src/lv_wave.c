/**
 * @file lv_wave.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "gui.h"
#include <src/core/lv_obj_pos.h>
#include <stdlib.h> // 用于 rand()
#include <math.h> // 用于 exp()

#include "utilities/wave.h"

#if 1

/*********************
 *      DEFINES
 *********************/
#define SCREEN_WIDTH  428
#define SCREEN_HEIGHT 142

#define SAMPLE_LEN 8096

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
void destroy_waveform_ui(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *root;
static lv_obj_t *chart;
static lv_chart_series_t *ser1;
static lv_timer_t *data_timer;

int16_t raw_data[SAMPLE_LEN] = {0};
#include "utilities/signal_array.h"

/**********************
 *      MACROS
 **********************/
#define CHART_WIDTH  400
#define CHART_HEIGHT 128

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#define PI          3.14159265358979323846
#define SAMPLE_RATE 200000.0      // 100 KHz
#define CARRIER_FREQ 10000.0      // 10 KHz
#define TOTAL_POINTS 8192         // 8K 点
#define BASE_NOISE  30            // 基底噪声幅值

// 四个缺陷回波参数：{中心位置, 峰值幅度, 半宽（包络宽度系数）}
typedef struct {
    float center;      // 中心采样点位置
    float amplitude;   // 峰值幅度
    float width;       // 包络半宽度（控制回波宽度，越大越宽）
} EchoParams;

float signal_float[TOTAL_POINTS];
uint16_t signal_uint16[TOTAL_POINTS];

PeakInfo peaks[100];


int create_signal() {
    // 定义缺陷回波参数
    EchoParams echos[] = {
        {200.0f, 1500.0f, 20.0f},
        {200.0f, 1300.0f, 10.0f},
        {300.0f, 1000.0f, 10.0f},
        {400.0f,  600.0f, 10.0f}
    };
    int echo_count = 10; // sizeof(echos) / sizeof(EchoParams);

    // 生成信号
    for (int i = 0; i < TOTAL_POINTS; i++) {
        float t = (float)i / SAMPLE_RATE;   // 当前时间（秒）

        // 基底噪声（用小幅正弦波模拟随机噪声）
        float noise = BASE_NOISE * (0.5f + 0.5f * sinf(2.0f * PI * 1234.5f * t)); // 简单模拟，实际可用随机数

        // 正常载波信号（基底振荡）
        float carrier = BASE_NOISE * sinf(2.0f * PI * CARRIER_FREQ * t);

        float signal = carrier + noise;

        // 叠加缺陷回波（高斯包络调制）
        for (int e = 0; e < echo_count; e++) {
            float x = (float)i - e * echos[0].center;
            // 高斯包络：exp(-x^2 / width^2)
            float envelope = expf(-(x * x) / (echos[0].width * echos[0].width));
            // 载波：cos(2πf t + φ)，与中心频率同频
            float echo_signal = (echos[0].amplitude-e*100.0f) * envelope * sinf(2.0f * PI * CARRIER_FREQ * t);
            signal += echo_signal;
        }

        // 加入少量随机噪声（模拟真实环境）
        // 此处用固定种子伪随机，实际可用rand()
        float random_noise = (float)(rand() % 100) / 100.0f * 20.0f - 10.0f;
        signal += random_noise;

        // 存储浮点值
        signal_float[i] = signal;

        // 转换为无符号整型（限幅到0~4095，模拟12位ADC）
        int int_val = (int)(signal + 2048.0f);  // 假设偏置2048
        if (int_val < 0) int_val = 0;
        if (int_val > 4095) int_val = 4095;
        signal_uint16[i] = (uint16_t)int_val;
    }

    return 0;
}

void _adc_raw_init()
{
    // 在位置 150, 600, 1050, 1800 处产生高斯型波峰
    int peak_pos[] = {150, 600, 1050, 1800};
    int peak_amp[] = {1000, 1500, 1300, 1200};

    for (int i = 0; i < 4; i++) {
        peak_pos[i] += rand() % 20 - 10; // 随机偏移 ±10
        peak_amp[i] += rand() % 200 - 100; // 随机偏移 ±100
    }
    
    // 生成模拟信号：包含多个波峰
    for (int i = 0; i < SAMPLE_LEN; i++) {
        raw_data[i] = 5; // 基底噪声
        for (int j = 0; j < 4; j++) {
            int dx = i - peak_pos[j];
            if (dx >= -20 && dx <= 20) {
                int val = peak_amp[j] * exp(-(dx*dx) / 100.0);
                if (val > raw_data[i]) raw_data[i] = val/20;
            }
        }
    }
}

void adc_raw_init()
{
    int32_t * ypoints = lv_chart_get_series_y_array(chart, ser1);
    create_signal();
    for(int i = 0; i < CHART_WIDTH; i++) {
        ypoints[i] = signal_uint16[i*2]/40;
        // ypoints[i] = test_signal[i*TOTAL_POINTS/CHART_WIDTH]/40;
    }

}

static void add_data(lv_timer_t * t)
{
    lv_obj_t * chart = (lv_obj_t *)lv_timer_get_user_data(t);
    lv_chart_series_t * ser = lv_chart_get_series_next(chart, NULL);

    // lv_chart_set_next_value(chart, ser, (int32_t)lv_rand(10, 90));

    // uint32_t p = lv_chart_get_point_count(chart);
    // uint32_t s = lv_chart_get_x_start_point(chart, ser);
    // int32_t * a = lv_chart_get_series_y_array(chart, ser);

    // a[(s + 1) % p] = LV_CHART_POINT_NONE;
    // a[(s + 2) % p] = LV_CHART_POINT_NONE;
    // a[(s + 2) % p] = LV_CHART_POINT_NONE;

    adc_raw_init();
    // lv_chart_set_series_values(chart, ser, raw_data, SAMPLE_LEN);
    lv_chart_refresh(chart);
}

/**
 * Circular line chart with gap
 */
void lv_wave(void)
{
    /*Create a stacked_area_chart.obj*/
    root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    chart = lv_chart_create(root);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_size(chart, CHART_WIDTH, CHART_HEIGHT);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t * label = lv_label_create(root);
    lv_label_set_text(label, "Waveform");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    lv_chart_set_point_count(chart, CHART_WIDTH);
    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    /*Prefill with data*/
    adc_raw_init();
    int peak_count = find_all_peaks(signal_uint16, TOTAL_POINTS, 3000, peaks, 100);
    char buffer[256];
    for(int i = 0; i < peak_count; i++) {
        snprintf(buffer, sizeof(buffer), "Peak %d: Pos=%d, Amp=%d", i+1, peaks[i].index, peaks[i].amplitude);
        printf("%s\n", buffer);
    }
    snprintf(buffer, sizeof(buffer), "Detected Peaks: %d", peak_count);
    lv_label_set_text(label, buffer);
    // lv_chart_set_series_values(chart, ser1, raw_data, SAMPLE_LEN);

    // data_timer = lv_timer_create(add_data, 1000, chart);

    lv_set_root(destroy_waveform_ui);
}

void destroy_waveform_ui(void)
{
    if(data_timer) {
        lv_timer_del(data_timer);
        data_timer = NULL;
    }
    if(chart) {
        lv_obj_del(chart);
        chart = NULL;
    }
}


#endif
