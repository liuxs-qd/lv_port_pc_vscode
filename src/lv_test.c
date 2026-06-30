/**
 * lv_pwr_on.c
 */

#include "lvgl.h"

lv_obj_t* lv_demo(void);

static void create_main_ui(void);

LV_IMG_DECLARE(img_benchmark_lvgl_logo_rgb);  // 你的Logo图片C数组

/* ========== 1. 图片缩放动画的执行回调 (修正点) ========== */
static void img_zoom_anim_cb(void * obj, int32_t zoom)
{
    /* ★ 关键：调用 lv_img_set_zoom() 后，需要触发重绘 */
    // LV_LOG_WARN("obj %p, zomm %d", obj, zoom);
    lv_img_set_zoom(obj, zoom);
    /* 注意：lv_img_set_zoom() 内部已包含 lv_obj_invalidate()，但为了保险可以手动加 */
    lv_obj_invalidate(obj);  // 强制标记该区域需要重绘
}

/* ========== 2. 透明度动画的执行回调 ========== */
static void img_opa_anim_cb(void * obj, int32_t opa)
{
    lv_obj_set_style_opa(obj, opa, 0);
    lv_obj_invalidate(obj);  // 同样强制重绘
}

static void text_opa_anim_cb(void * obj, int32_t opa)
{
    lv_obj_set_style_opa(obj, opa, 0);
    lv_obj_invalidate(obj);  // 同样强制重绘
}


/* ========== 3. 动画结束回调 ========== */
static void boot_anim_end_cb(lv_event_t * e)
{
    lv_obj_t * logo = lv_event_get_user_data(e);
    lv_demo();
    lv_obj_del(lv_obj_get_parent(logo));  // 删除承载Logo的容器
    // create_main_ui();  // 跳转到主界面
}

/* ========== 4. 进度条动画执行回调 ========== */
static void progress_cb(void * bar, int32_t v)
{
    lv_bar_set_value(bar, v, LV_ANIM_OFF);
}

void boot_animation_start(int off)
{
    /* 5.1 创建容器 */
    lv_obj_t * boot_screen = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(boot_screen);
    lv_obj_set_style_bg_color(boot_screen, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_size(boot_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(boot_screen);

    /* 5.2 创建Logo图像 */
    lv_obj_t * logo = lv_img_create(boot_screen);
    lv_img_set_src(logo, &img_benchmark_lvgl_logo_rgb);

    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_size(logo, 100, 45);

    /* ★ 确保初始状态可见（透明度为完全不透明，缩放为1倍） */
    // lv_img_set_zoom(logo, 1);
    lv_obj_set_style_opa(logo, LV_OPA_COVER, 0);
    lv_obj_invalidate(logo);  // 立即绘制一次

    /* 5.3 Logo缩放动画（使用修正后的执行回调） */
    lv_anim_t a_zoom;
    lv_anim_init(&a_zoom);
    lv_anim_set_var(&a_zoom, logo);
    lv_anim_set_exec_cb(&a_zoom, img_zoom_anim_cb);  // ★ 使用修正后的回调
    if(off)
    {
        lv_anim_set_values(&a_zoom, 256, 0);  // 1倍 → 2倍
    }
    else
    {
        lv_anim_set_values(&a_zoom, 0, 256);  // 1倍 → 2倍
    }
    lv_anim_set_time(&a_zoom, 1600);
    lv_anim_set_delay(&a_zoom, 100);
    lv_anim_set_path_cb(&a_zoom, lv_anim_path_overshoot);
    lv_anim_start(&a_zoom);
    // LV_LOG_WARN("obj %p, logo %p", &a_zoom, logo);

    /* 5.4 Logo透明度动画（使用修正后的回调） */
    lv_anim_t a_fade;
    lv_anim_init(&a_fade);
    lv_anim_set_var(&a_fade, logo);
    lv_anim_set_exec_cb(&a_fade, img_opa_anim_cb);  // ★ 使用修正后的回调
    if(off)
    {
        lv_anim_set_values(&a_fade, LV_OPA_COVER, 0);
    }
    else
    {
        lv_anim_set_values(&a_fade, 0, LV_OPA_COVER);
    }
    lv_anim_set_time(&a_fade, 500);
    lv_anim_set_delay(&a_fade, 50);
    lv_anim_start(&a_fade);

    /* 5.5 加载文字 */
    lv_obj_t * label = lv_label_create(boot_screen);
    if(off)
    {
        lv_label_set_text(label, "Exiting...");
    }
    else
    {
        lv_label_set_text(label, "Loading...");
    }
    lv_obj_set_style_text_color(label, lv_color_hex(0xe000e0), 0);
    // lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_opa(label, 0, 0);

    /* 文字渐入动画 */
    lv_anim_t a_label_fade;
    lv_anim_init(&a_label_fade);
    lv_anim_set_var(&a_label_fade, label);
    lv_anim_set_exec_cb(&a_label_fade, (lv_anim_exec_xcb_t)text_opa_anim_cb);
    if(off)
    {
        lv_anim_set_values(&a_label_fade, LV_OPA_COVER, 0);
    }
    else
    {
        lv_anim_set_values(&a_label_fade, 0, LV_OPA_COVER);
    }
    lv_anim_set_time(&a_label_fade, 400);
    lv_anim_set_delay(&a_label_fade, 1400);
    lv_anim_start(&a_label_fade);

    /* 5.6 进度条 */
    lv_obj_t * bar = lv_bar_create(boot_screen);
    lv_obj_set_size(bar, 200, 10);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333355), 0);
    lv_obj_set_style_radius(bar, 5, 0);
    lv_obj_set_style_anim_time(bar, 0, 0);

    lv_anim_t a_bar;
    lv_anim_init(&a_bar);
    lv_anim_set_var(&a_bar, bar);
    lv_anim_set_exec_cb(&a_bar, progress_cb);
    if(off)
    {
        lv_anim_set_values(&a_bar, 100, 0);
    }
    else
    {
        lv_anim_set_values(&a_bar, 0, 100);
    }
    lv_anim_set_time(&a_bar, 1500);
    lv_anim_set_delay(&a_bar, 300);
    lv_anim_start(&a_bar);

    /* 5.7 定时结束 */
    lv_timer_t * timer = lv_timer_create((lv_timer_cb_t)boot_anim_end_cb, 1500 + 300 + 200, logo);
    lv_timer_set_repeat_count(timer, 1);
}

/* ========== 主界面（占位） ========== */

static void create_main_ui(void)
{
    lv_obj_t * root;
    root = lv_demo();
}
