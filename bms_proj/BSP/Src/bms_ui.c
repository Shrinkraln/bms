/**
 * bms_ui.c — BMS 化成 UI 实现 (LVGL 9.x, 240×240, 3 标签页)
 */
#include "bms_ui.h"
#include "lvgl.h"
#include <stdio.h>

/* ===== 全局: fm 指针 (按钮回调要用) ===== */
static fm_ctx_t *s_fm = NULL;

/* ===== tabview ===== */
static lv_obj_t *s_tv;

/* ===== Dashboard 页控件 ===== */
static lv_obj_t *s_dash_state_dot;     /* 颜色圆点表示状态 */
static lv_obj_t *s_dash_state_lbl;
static lv_obj_t *s_dash_pack_lbl;
static lv_obj_t *s_dash_cur_lbl;
static lv_obj_t *s_dash_cell_bar[5];
static lv_obj_t *s_dash_cell_lbl[5];
static lv_obj_t *s_dash_bot_lbl;

/* ===== Chart 页控件 ===== */
static lv_obj_t *s_chart;
static lv_chart_series_t *s_chart_v;   /* 总压 mV (主 Y) */
static lv_chart_series_t *s_chart_i;   /* 电流 mA (次 Y) */
static lv_obj_t *s_chart_lbl;          /* 当前最新值文字 */

/* ===== Setup 页 ===== */
static lv_obj_t *s_setup_status_lbl;

/* ===== 状态映射 ===== */
static lv_color_t color_for_state(uint8_t s)
{
    switch (s) {
        case FM_CC_CHARGE:
        case FM_CV_CHARGE:           return lv_color_hex(0x1976D2);
        case FM_REST_AFTER_CHG:
        case FM_REST_AFTER_DSG:      return lv_color_hex(0x616161);
        case FM_CC_DISCHARGE:        return lv_color_hex(0xF57C00);
        case FM_COMPLETE:            return lv_color_hex(0x388E3C);
        case FM_ERROR:               return lv_color_hex(0xD32F2F);
        default:                     return lv_color_hex(0x424242);
    }
}
static const char *state_short(uint8_t s)
{
    static const char *n[] = {
        "IDLE", "CC CHG", "CV CHG", "REST CHG", "CC DSG", "REST DSG", "DONE", "ERROR"
    };
    return (s < 8) ? n[s] : "?";
}
static const char *err_short(uint8_t e)
{
    static const char *n[] = { "NONE", "OV", "UV", "TEMP", "SENSOR", "TIMEOUT" };
    return (e < 6) ? n[e] : "?";
}

/* ===== 按钮回调 ===== */
static void on_start_clicked(lv_event_t *e)
{
    (void)e;
    if (s_fm) fm_start(s_fm);
}
static void on_stop_clicked(lv_event_t *e)
{
    (void)e;
    if (s_fm) fm_stop(s_fm);
}

/* ===== 构建 Dashboard 页 ===== */
static void build_dashboard(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 4, 0);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x101218), 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部: 状态圆点 + 文字 */
    s_dash_state_dot = lv_obj_create(parent);
    lv_obj_set_size(s_dash_state_dot, 18, 18);
    lv_obj_set_pos(s_dash_state_dot, 0, 0);
    lv_obj_set_style_radius(s_dash_state_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_dash_state_dot, 0, 0);
    lv_obj_set_style_bg_color(s_dash_state_dot, color_for_state(FM_IDLE), 0);
    lv_obj_remove_flag(s_dash_state_dot, LV_OBJ_FLAG_SCROLLABLE);

    s_dash_state_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(s_dash_state_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_dash_state_lbl, &lv_font_montserrat_20, 0);
    lv_label_set_text(s_dash_state_lbl, "IDLE");
    lv_obj_set_pos(s_dash_state_lbl, 26, -2);

    /* 大字: Pack + Current */
    s_dash_pack_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(s_dash_pack_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_dash_pack_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(s_dash_pack_lbl, 0, 28);
    lv_label_set_text(s_dash_pack_lbl, "-- V");

    s_dash_cur_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(s_dash_cur_lbl, lv_color_hex(0xB3D4FC), 0);
    lv_obj_set_style_text_font(s_dash_cur_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(s_dash_cur_lbl, 0, 62);
    lv_label_set_text(s_dash_cur_lbl, "-- mA");

    /* 5 节单体竖条 */
    int x = 0;
    for (int i = 0; i < 5; ++i) {
        s_dash_cell_bar[i] = lv_bar_create(parent);
        lv_obj_set_size(s_dash_cell_bar[i], 36, 50);
        lv_obj_set_pos(s_dash_cell_bar[i], x, 100);
        lv_bar_set_range(s_dash_cell_bar[i], 2500, 4250);
        lv_bar_set_value(s_dash_cell_bar[i], 3700, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_dash_cell_bar[i], lv_color_hex(0x263238), LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_dash_cell_bar[i], lv_color_hex(0x4CAF50), LV_PART_INDICATOR);

        s_dash_cell_lbl[i] = lv_label_create(parent);
        lv_obj_set_style_text_color(s_dash_cell_lbl[i], lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(s_dash_cell_lbl[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(s_dash_cell_lbl[i], x + 10, 152);
        lv_label_set_text_fmt(s_dash_cell_lbl[i], "C%d", i + 1);
        x += 46;
    }

    /* 底部: 容量+温度+报警 */
    s_dash_bot_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(s_dash_bot_lbl, lv_color_hex(0x8E9AAF), 0);
    lv_obj_set_style_text_font(s_dash_bot_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(s_dash_bot_lbl, 0, 175);
    lv_label_set_text(s_dash_bot_lbl, "Chg --  Dsg --  T --  ALM NONE");
}

/* ===== 构建 Chart 页 ===== */
static void build_chart(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 4, 0);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x101218), 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* 当前值标签 (顶部一行) */
    s_chart_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(s_chart_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_chart_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_chart_lbl, 0, 0);
    lv_label_set_text(s_chart_lbl, "Pack -- V   I -- mA");

    /* 曲线 */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 224, 145);
    lv_obj_set_pos(s_chart, 0, 22);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);              /* 60 秒 */
    lv_chart_set_update_mode(s_chart, LV_CHART_UPDATE_MODE_SHIFT);

    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y,    0, 22000);   /* mV */
    lv_chart_set_range(s_chart, LV_CHART_AXIS_SECONDARY_Y, -3000, 3000); /* mA */

    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x1A1D26), 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_obj_set_style_border_color(s_chart, lv_color_hex(0x37474F), 0);
    lv_obj_set_style_size(s_chart, 0, 0, LV_PART_INDICATOR);   /* 关闭数据点圆点 */

    s_chart_v = lv_chart_add_series(s_chart, lv_color_hex(0x90CAF9),
                                    LV_CHART_AXIS_PRIMARY_Y);
    s_chart_i = lv_chart_add_series(s_chart, lv_color_hex(0xFFAB91),
                                    LV_CHART_AXIS_SECONDARY_Y);
}

/* ===== 构建 Setup 页 ===== */
static void build_setup(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x101218), 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* START 按钮 (大,绿) */
    lv_obj_t *btn_start = lv_button_create(parent);
    lv_obj_set_size(btn_start, 200, 60);
    lv_obj_set_pos(btn_start, 10, 10);
    lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x388E3C), 0);
    lv_obj_set_style_radius(btn_start, 8, 0);
    lv_obj_t *l1 = lv_label_create(btn_start);
    lv_label_set_text(l1, "START");
    lv_obj_set_style_text_color(l1, lv_color_white(), 0);
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_28, 0);
    lv_obj_center(l1);
    lv_obj_add_event_cb(btn_start, on_start_clicked, LV_EVENT_CLICKED, NULL);

    /* STOP 按钮 (大,红) */
    lv_obj_t *btn_stop = lv_button_create(parent);
    lv_obj_set_size(btn_stop, 200, 60);
    lv_obj_set_pos(btn_stop, 10, 80);
    lv_obj_set_style_bg_color(btn_stop, lv_color_hex(0xD32F2F), 0);
    lv_obj_set_style_radius(btn_stop, 8, 0);
    lv_obj_t *l2 = lv_label_create(btn_stop);
    lv_label_set_text(l2, "STOP");
    lv_obj_set_style_text_color(l2, lv_color_white(), 0);
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_28, 0);
    lv_obj_center(l2);
    lv_obj_add_event_cb(btn_stop, on_stop_clicked, LV_EVENT_CLICKED, NULL);

    /* 当前状态文字 */
    s_setup_status_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(s_setup_status_lbl, lv_color_hex(0x9CA3AF), 0);
    lv_obj_set_style_text_font(s_setup_status_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_setup_status_lbl, 10, 152);
    lv_label_set_text(s_setup_status_lbl, "State: IDLE");
}

/* ===== 对外接口 ===== */
void bms_ui_init(fm_ctx_t *fm)
{
    s_fm = fm;

    /* 全屏深色 */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0C12), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* tabview: 顶部 30px tab bar */
    s_tv = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(s_tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(s_tv, 30);
    lv_obj_set_size(s_tv, 240, 240);

    build_dashboard(lv_tabview_add_tab(s_tv, "Dash"));
    build_chart    (lv_tabview_add_tab(s_tv, "Chart"));
    build_setup    (lv_tabview_add_tab(s_tv, "Setup"));
}

void bms_ui_update(const fm_ctx_t *ctx)
{
    char buf[40];

    /* ----- Dashboard ----- */
    lv_obj_set_style_bg_color(s_dash_state_dot, color_for_state(ctx->state), 0);
    lv_label_set_text(s_dash_state_lbl, state_short(ctx->state));

    snprintf(buf, sizeof(buf), "%lu.%03lu V",
             (unsigned long)(ctx->pack_mV / 1000U),
             (unsigned long)(ctx->pack_mV % 1000U));
    lv_label_set_text(s_dash_pack_lbl, buf);

    int32_t i_mA = ctx->current_mA;
    snprintf(buf, sizeof(buf), "%s%ld mA", i_mA >= 0 ? "+" : "", (long)i_mA);
    lv_label_set_text(s_dash_cur_lbl, buf);

    for (int i = 0; i < 5; ++i) {
        int32_t v = (int32_t)ctx->cell_mV[i];
        if (v < 2500) v = 2500;
        if (v > 4250) v = 4250;
        lv_bar_set_value(s_dash_cell_bar[i], v, LV_ANIM_OFF);
    }

    snprintf(buf, sizeof(buf), "Chg %d  Dsg %d  T %dC  ALM %s",
             (int)ctx->charged_mAh, (int)ctx->discharged_mAh,
             ctx->temp_C, err_short(ctx->error));
    lv_label_set_text(s_dash_bot_lbl, buf);

    /* ----- Chart ----- */
    lv_chart_set_next_value(s_chart, s_chart_v, (int32_t)ctx->pack_mV);
    lv_chart_set_next_value(s_chart, s_chart_i, (int32_t)ctx->current_mA);
    snprintf(buf, sizeof(buf), "Pack %lu.%03lu V   I %s%ld mA",
             (unsigned long)(ctx->pack_mV / 1000U),
             (unsigned long)(ctx->pack_mV % 1000U),
             i_mA >= 0 ? "+" : "", (long)i_mA);
    lv_label_set_text(s_chart_lbl, buf);

    /* ----- Setup ----- */
    snprintf(buf, sizeof(buf), "State: %s   ALM: %s",
             state_short(ctx->state), err_short(ctx->error));
    lv_label_set_text(s_setup_status_lbl, buf);
}

void bms_ui_next_tab(void)
{
    uint32_t cur = lv_tabview_get_tab_active(s_tv);
    uint32_t cnt = lv_tabview_get_tab_count(s_tv);
    uint32_t nxt = (cur + 1U) % cnt;
    lv_tabview_set_active(s_tv, nxt, LV_ANIM_ON);
}
