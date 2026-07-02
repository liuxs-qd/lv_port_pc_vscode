#include "lvgl.h"

// 根据你的屏幕分辨率调整（320x240）
#define BAR_WIDTH  200
#define BAR_HEIGHT 30

static lv_obj_t *battery_bar = NULL;
static lv_obj_t *percent_label = NULL;
static lv_anim_t *breath_anim = NULL;   // 保存呼吸动画指针，用于停止
static int current_battery = 60;         // 假设当前电量为 60%，你可以从实际系统获取

/* 呼吸动画执行回调：控制剩余部分的淡入淡出 */
static void breath_anim_cb(void * obj, int32_t opa)
{
    // 进度条的指示器（绿色填充部分）背景为不透明，其余部分通过父容器背景的透明度制造呼吸感
    lv_obj_set_style_bg_opa(obj, opa, 0);
}

/* 电量更新回调：每次充电脉冲增加电量 */
static void charge_pulse_cb(lv_timer_t * timer)
{
    // 每次脉冲增加 1% ~ 3%（模拟充电）
    int increment = lv_rand(1, 3);
    current_battery += increment;
    if (current_battery > 100) current_battery = 100;

    // 更新进度条
    lv_bar_set_value(battery_bar, current_battery, LV_ANIM_OFF);

    // 更新百分比标签
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", current_battery);
    lv_label_set_text(percent_label, buf);

    // 当电量达到 100%，停止呼吸动画和充电脉冲
    if (current_battery >= 100) {
        lv_timer_del(timer);          // 停止脉冲定时器
        if (breath_anim) {
            lv_anim_delete(battery_bar, breath_anim_cb); // 停止呼吸动画
            breath_anim = NULL;
        }
        // 恢复进度条指示器为完全不透明（保证满电时全绿）
        lv_obj_set_style_bg_opa(battery_bar, LV_OPA_COVER, LV_PART_INDICATOR);
        LV_LOG_USER("充电完成！");
    }
}

static lv_anim_t anim1;
/* 启动充电动画（初始电量为 current_battery） */
void start_charging_animation(int initial_battery)
{
    if (initial_battery < 0) initial_battery = 0;
    if (initial_battery > 100) initial_battery = 100;
    current_battery = initial_battery;

    // 1. 创建进度条（电池电量条）
    battery_bar = lv_bar_create(lv_screen_active());
    lv_obj_set_size(battery_bar, BAR_WIDTH, BAR_HEIGHT);
    lv_obj_align(battery_bar, LV_ALIGN_CENTER, 0, -20);

    lv_bar_set_range(battery_bar, 0, 100);
    lv_bar_set_value(battery_bar, current_battery, LV_ANIM_OFF);

    // 进度条背景样式（灰色底）
    lv_obj_set_style_radius(battery_bar, 8, 0);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_pad_all(battery_bar, 4, 0);

    // 进度条指示器（绿色填充部分）
    lv_obj_set_style_radius(battery_bar, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(battery_bar, lv_color_hex(0x44CC44), LV_PART_INDICATOR);
    // ★ 关键：指示器的透明度将用于呼吸效果
    lv_obj_set_style_bg_opa(battery_bar, LV_OPA_COVER, LV_PART_INDICATOR);

    // 2. 创建百分比标签
    percent_label = lv_label_create(lv_screen_active());
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", current_battery);
    lv_label_set_text(percent_label, buf);
    lv_obj_set_style_text_font(percent_label, &lv_font_montserrat_18, 0);
    lv_obj_align(percent_label, LV_ALIGN_CENTER, 0, 40);

    // 3. 如果还没充满，启动呼吸动画
    if (current_battery < 100) {
        // 呼吸动画：控制指示器透明度在 0.3 ~ 1.0 之间循环（模拟呼吸）
        breath_anim = &anim1;
        lv_anim_init(breath_anim);
        lv_anim_set_var(breath_anim, battery_bar);
        lv_anim_set_exec_cb(breath_anim, breath_anim_cb);
        lv_anim_set_values(breath_anim, LV_OPA_30, LV_OPA_COVER);
        lv_anim_set_time(breath_anim, 1500);          // 一个完整呼吸周期
        lv_anim_set_path_cb(breath_anim, lv_anim_path_ease_in_out);
        lv_anim_set_repeat_count(breath_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(breath_anim);

        // 4. 启动充电脉冲定时器（每 300ms 增加一点电量）
        lv_timer_t * timer = lv_timer_create(charge_pulse_cb, 300, NULL);
        lv_timer_set_repeat_count(timer, -1); // 无限重复，直到达到100%
    } else {
        LV_LOG_USER("电池已满，不启动充电动画");
    }
}
