/**
 * bms_ui.c — BMS 化成 UI (LVGL 9.x, ST7796 320×480, 3 标签页)
 *
 * 布局充分利用 3.5" 屏:
 *   Dashboard: 状态行 / 大字 Pack(28) / 大字 Current(28) / 5 节竖条+电压文字 / 底部行
 *   Chart    : 60 秒 V/I 滚动曲线 (340 高)
 *   Setup    : 巨型 START / STOP 按钮 (300×100, 易触)
 */
#include "bms_ui.h"
#include "lvgl.h"
#include <stdio.h>

static fm_ctx_t *s_fm = NULL;
static lv_obj_t *s_tv;

/* Dashboard */
static lv_obj_t *s_dot, *s_state_lbl;
static lv_obj_t *s_pack_lbl, *s_cur_lbl;
static lv_obj_t *s_cell_bar[5], *s_cell_name[5], *s_cell_mv[5];
static lv_obj_t *s_bot_lbl;

/* Chart */
static lv_obj_t *s_chart;
static lv_chart_series_t *s_chart_v, *s_chart_i;
static lv_obj_t *s_chart_top_lbl;

/* Setup */
static lv_obj_t *s_setup_status_lbl;

/* ===== 颜色 / 名字 ===== */
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
static void on_start_clicked(lv_event_t *e) { (void)e; if (s_fm) fm_start(s_fm); }
static void on_stop_clicked (lv_event_t *e) { (void)e; if (s_fm) fm_stop (s_fm); }

/* ===== Dashboard ===== */
static void build_dashboard(lv_obj_t *p)
{
    lv_obj_set_style_pad_all(p, 8, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x101218), 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部: 状态指示 */
    s_dot = lv_obj_create(p);
    lv_obj_set_size(s_dot, 24, 24);
    lv_obj_set_pos(s_dot, 0, 4);
    lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_dot, 0, 0);
    lv_obj_set_style_bg_color(s_dot, color_for_state(FM_IDLE), 0);
    lv_obj_remove_flag(s_dot, LV_OBJ_FLAG_SCROLLABLE);

    s_state_lbl = lv_label_create(p);
    lv_obj_set_style_text_color(s_state_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_state_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(s_state_lbl, 36, 4);
    lv_label_set_text(s_state_lbl, "IDLE");

    /* 大字 Pack */
    s_pack_lbl = lv_label_create(p);
    lv_obj_set_style_text_color(s_pack_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_pack_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(s_pack_lbl, 0, 44);
    lv_label_set_text(s_pack_lbl, "-- V");

    /* 大字 Current */
    s_cur_lbl = lv_label_create(p);
    lv_obj_set_style_text_color(s_cur_lbl, lv_color_hex(0xB3D4FC), 0);
    lv_obj_set_style_text_font(s_cur_lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_pos(s_cur_lbl, 0, 88);
    lv_label_set_text(s_cur_lbl, "-- mA");

    /* 5 节竖条 + 名字 + 电压数字 (每格 60 宽, 居中) */
    for (int i = 0; i < 5; ++i) {
        int x = i * 60;

        s_cell_bar[i] = lv_bar_create(p);
        lv_obj_set_size(s_cell_bar[i], 50, 120);
        lv_obj_set_pos(s_cell_bar[i], x, 140);
        lv_bar_set_range(s_cell_bar[i], 2500, 4250);
        lv_bar_set_value(s_cell_bar[i], 3700, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(s_cell_bar[i], lv_color_hex(0x263238), LV_PART_MAIN);
        lv_obj_set_style_bg_color(s_cell_bar[i], lv_color_hex(0x4CAF50), LV_PART_INDICATOR);

        s_cell_name[i] = lv_label_create(p);
        lv_obj_set_style_text_color(s_cell_name[i], lv_color_hex(0xAAAAAA), 0);
        lv_obj_set_style_text_font(s_cell_name[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(s_cell_name[i], x + 12, 264);
        lv_label_set_text_fmt(s_cell_name[i], "C%d", i + 1);

        s_cell_mv[i] = lv_label_create(p);
        lv_obj_set_style_text_color(s_cell_mv[i], lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_text_font(s_cell_mv[i], &lv_font_montserrat_12, 0);
        lv_obj_set_pos(s_cell_mv[i], x + 2, 282);
        lv_label_set_text(s_cell_mv[i], "----");
    }

    /* 底部行 */
    s_bot_lbl = lv_label_create(p);
    lv_obj_set_style_text_color(s_bot_lbl, lv_color_hex(0x8E9AAF), 0);
    lv_obj_set_style_text_font(s_bot_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_bot_lbl, 0, 320);
    lv_label_set_text(s_bot_lbl, "Chg --  Dsg --  T --  ALM NONE");
}

/* ===== Chart ===== */
static void build_chart(lv_obj_t *p)
{
    lv_obj_set_style_pad_all(p, 8, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x101218), 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    s_chart_top_lbl = lv_label_create(p);
    lv_obj_set_style_text_color(s_chart_top_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_chart_top_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(s_chart_top_lbl, 0, 0);
    lv_label_set_text(s_chart_top_lbl, "Pack -- V    I -- mA");

    s_chart = lv_chart_create(p);
    lv_obj_set_size(s_chart, 304, 340);
    lv_obj_set_pos(s_chart, 0, 28);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_chart_set_update_mode(s_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y,    0,  22000);   /* mV */
    lv_chart_set_range(s_chart, LV_CHART_AXIS_SECONDARY_Y, -3000, 3000);  /* mA */
    lv_obj_set_style_bg_color(s_chart, lv_color_hex(0x1A1D26), 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_obj_set_style_border_color(s_chart, lv_color_hex(0x37474F), 0);
    lv_obj_set_style_size(s_chart, 0, 0, LV_PART_INDICATOR);

    s_chart_v = lv_chart_add_series(s_chart, lv_color_hex(0x90CAF9), LV_CHART_AXIS_PRIMARY_Y);
    s_chart_i = lv_chart_add_series(s_chart, lv_color_hex(0xFFAB91), LV_CHART_AXIS_SECONDARY_Y);
}

/* ===== Setup ===== */
static void build_setup(lv_obj_t *p)
{
    lv_obj_set_style_pad_all(p, 12, 0);
    lv_obj_set_style_bg_color(p, lv_color_hex(0x101218), 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_start = lv_button_create(p);
    lv_obj_set_size(btn_start, 296, 100);
    lv_obj_set_pos(btn_start, 0, 20);
    lv_obj_set_style_bg_color(btn_start, lv_color_hex(0x388E3C), 0);
    lv_obj_set_style_radius(btn_start, 12, 0);
    lv_obj_t *l1 = lv_label_create(btn_start);
    lv_label_set_text(l1, "START");
    lv_obj_set_style_text_color(l1, lv_color_white(), 0);
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_28, 0);
    lv_obj_center(l1);
    lv_obj_add_event_cb(btn_start, on_start_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_stop = lv_button_create(p);
    lv_obj_set_size(btn_stop, 296, 100);
    lv_obj_set_pos(btn_stop, 0, 140);
    lv_obj_set_style_bg_color(btn_stop, lv_color_hex(0xD32F2F), 0);
    lv_obj_set_style_radius(btn_stop, 12, 0);
    lv_obj_t *l2 = lv_label_create(btn_stop);
    lv_label_set_text(l2, "STOP");
    lv_obj_set_style_text_color(l2, lv_color_white(), 0);
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_28, 0);
    lv_obj_center(l2);
    lv_obj_add_event_cb(btn_stop, on_stop_clicked, LV_EVENT_CLICKED, NULL);

    s_setup_status_lbl = lv_label_create(p);
    lv_obj_set_style_text_color(s_setup_status_lbl, lv_color_hex(0x9CA3AF), 0);
    lv_obj_set_style_text_font(s_setup_status_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(s_setup_status_lbl, 0, 280);
    lv_label_set_text(s_setup_status_lbl, "State: IDLE");
}

/* ===== Public ===== */
void bms_ui_init(fm_ctx_t *fm)
{
    s_fm = fm;

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0C12), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    s_tv = lv_tabview_create(scr);
    lv_tabview_set_tab_bar_position(s_tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(s_tv, 40);
    lv_obj_set_size(s_tv, 320, 480);

    build_dashboard(lv_tabview_add_tab(s_tv, "Dash"));
    build_chart    (lv_tabview_add_tab(s_tv, "Chart"));
    build_setup    (lv_tabview_add_tab(s_tv, "Setup"));
}

void bms_ui_update(const fm_ctx_t *ctx)
{
    char buf[40];

    /* Dashboard */
    lv_obj_set_style_bg_color(s_dot, color_for_state(ctx->state), 0);
    lv_label_set_text(s_state_lbl, state_short(ctx->state));

    snprintf(buf, sizeof(buf), "%lu.%03lu V",
             (unsigned long)(ctx->pack_mV / 1000U),
             (unsigned long)(ctx->pack_mV % 1000U));
    lv_label_set_text(s_pack_lbl, buf);

    int32_t i_mA = ctx->current_mA;
    snprintf(buf, sizeof(buf), "%s%ld mA", i_mA >= 0 ? "+" : "", (long)i_mA);
    lv_label_set_text(s_cur_lbl, buf);

    for (int i = 0; i < 5; ++i) {
        int32_t v = (int32_t)ctx->cell_mV[i];
        int32_t vc = v;
        if (vc < 2500) vc = 2500;
        if (vc > 4250) vc = 4250;
        lv_bar_set_value(s_cell_bar[i], vc, LV_ANIM_OFF);

        snprintf(buf, sizeof(buf), "%u.%02u",
                 (unsigned)(ctx->cell_mV[i] / 1000U),
                 (unsigned)((ctx->cell_mV[i] % 1000U) / 10U));
        lv_label_set_text(s_cell_mv[i], buf);
    }

    snprintf(buf, sizeof(buf), "Chg %d  Dsg %d  T %dC  ALM %s",
             (int)ctx->charged_mAh, (int)ctx->discharged_mAh,
             ctx->temp_C, err_short(ctx->error));
    lv_label_set_text(s_bot_lbl, buf);

    /* Chart */
    lv_chart_set_next_value(s_chart, s_chart_v, (int32_t)ctx->pack_mV);
    lv_chart_set_next_value(s_chart, s_chart_i, (int32_t)ctx->current_mA);
    snprintf(buf, sizeof(buf), "Pack %lu.%03lu V    I %s%ld mA",
             (unsigned long)(ctx->pack_mV / 1000U),
             (unsigned long)(ctx->pack_mV % 1000U),
             i_mA >= 0 ? "+" : "", (long)i_mA);
    lv_label_set_text(s_chart_top_lbl, buf);

    /* Setup */
    snprintf(buf, sizeof(buf), "State: %s    ALM: %s",
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
