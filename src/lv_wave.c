/**
 * @file lv_demo.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "gui.h"
#include <stdlib.h> // 用于 rand()
#include <math.h> // 用于 exp()

#if 1

/*********************
 *      DEFINES
 *********************/
#define SCREEN_WIDTH  428
#define SCREEN_HEIGHT 142

#define SAMPLE_LEN 1024

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
static lv_obj_t *chart;
static lv_chart_series_t *ser1;
static lv_timer_t *data_timer;

int32_t raw_data[SAMPLE_LEN] = {0};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void adc_raw_init()
{
    // 在位置 50, 200, 450, 800 处产生高斯型波峰
    int peak_pos[] = {50, 200, 450, 800};
    int peak_amp[] = {2000, 1500, 2300, 1200};

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
    lv_chart_set_series_values(chart, ser, raw_data, SAMPLE_LEN);
    lv_chart_refresh(chart);
}

/**
 * Circular line chart with gap
 */
void create_waveform_ui2(void)
{
    /*Create a stacked_area_chart.obj*/
    chart = lv_chart_create(lv_screen_active());
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_size(chart, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_center(chart);

    lv_chart_set_point_count(chart, SAMPLE_LEN);
    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    /*Prefill with data*/
    adc_raw_init();

    lv_chart_set_series_values(chart, ser1, raw_data, SAMPLE_LEN);

    data_timer = lv_timer_create(add_data, 1000, chart);

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
