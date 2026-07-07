#include "lvgl.h"
#include <src/misc/lv_timer.h>
#include <src/misc/lv_types.h>
#include <src/widgets/chart/lv_chart.h>
#include <stdlib.h> // 用于 rand()
#include <math.h>   // 用于 sinf()

#define SCREEN_WIDTH  428
#define SCREEN_HEIGHT 142

#define DATA_POINT_COUNT 1024 // 显示的点数

static lv_obj_t *wrapper;
static lv_chart_series_t *ser1;
static lv_timer_t *data_timer;
static int32_t vals[DATA_POINT_COUNT]; // 用于存储模拟ADC数据

void adc_raw(
    int32_t *raw_signal,  // 输出：原始ADC采样数据
    uint32_t len          // 长度
) {
    // 模拟生成一个带噪声的正弦波信号作为示例
    for (uint32_t i = 0; i < len; i++) {
        float t = (float)i / len * 2.0f * 3.14159f * 5.0f; // 5个周期
        float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 100; // ±50噪声
        raw_signal[i] = (int32_t)(1000.0f * sinf(t) + noise)/20 +50;
    }
}

/* 模拟数据生成，您可替换为自己的ADC采集函数 */
static void simulate_data_generation(lv_timer_t *timer) {
    lv_obj_t * chart = (lv_obj_t *)lv_timer_get_user_data(timer);
    // 假设ADC采样值范围是 0-4095，映射到Y轴范围 0-100
    // 生成一个随机波动，模拟真实信号
    adc_raw(vals, DATA_POINT_COUNT); 
    
    // 将新数据推入图表
    lv_chart_set_series_values(chart, ser1, vals, DATA_POINT_COUNT);
    lv_chart_refresh(chart);
}

/* 初始化波形显示界面 */
void create_waveform_ui(void) {
    /* 创建一个容器，用于容纳图表和刻度，使其可滚动 */
    wrapper = lv_obj_create(lv_screen_active());
    lv_obj_set_size(wrapper, SCREEN_WIDTH, SCREEN_HEIGHT);
    // lv_obj_set_size(wrapper, lv_pct(300), lv_pct(100));
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_align(wrapper, LV_ALIGN_CENTER, 0, 0);

    // 创建图表对象，占据整个屏幕
    lv_obj_t * chart = lv_chart_create(wrapper);
    lv_obj_set_size(chart, SCREEN_WIDTH-50, SCREEN_HEIGHT-50);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);

    // 配置图表基本属性
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);          // 线形图
    lv_chart_set_point_count(chart, DATA_POINT_COUNT);            // 屏幕同时显示100个点
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); // Y轴范围 0-100
    // lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, 100); // 如果需要双Y轴
    lv_chart_set_div_line_count(chart, 3, 11); // 设置网格线数量

    // 美化：移除背景填充、边框和点标记，仅保留干净的线
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);    // 隐藏数据点

    // 添加数据序列 (Series)
    ser1 = lv_chart_add_series(chart, lv_color_hex(0x00FF00), LV_CHART_AXIS_PRIMARY_Y);
    // 设置线条宽度和圆角，使波形更平滑
    lv_obj_set_style_line_width(chart, 2, LV_PART_INDICATOR);
    lv_obj_set_style_line_rounded(chart, 1, LV_PART_INDICATOR);

    // 5. 初始化数据，防止出现从0开始的“扫描线”
    for (int i = 0; i < DATA_POINT_COUNT; i++) {
        lv_chart_set_next_value(chart, ser1, 50); 
    }

    /* 创建刻度对象，放在图表下方 */
    lv_obj_t * scale_bottom = lv_scale_create(wrapper);
    lv_scale_set_mode(scale_bottom, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_obj_set_size(scale_bottom, SCREEN_WIDTH-50, 10);
    lv_scale_set_total_tick_count(scale_bottom, 11); // 0-100刻度，每10个单位一个刻度

    lv_scale_set_major_tick_every(scale_bottom, 1); // 每10个单位一个主刻度

    /* 3. 设置自定义刻度标签 */
    static const char * txt[] = {"0", "10", "20", "30", "40", "50", 
                        "60", "70", "80", "90", "100", NULL};
    lv_scale_set_text_src(scale_bottom, txt);

    // 6. 启动定时器，模拟以50ms间隔不断采集新数据
    data_timer = lv_timer_create(simulate_data_generation, 50, chart);
}
