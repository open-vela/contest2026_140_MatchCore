/****************************************************************************
 * MC Watchdog AI — 告警弹窗 UI
 ****************************************************************************/

#include "mc_watchdog.h"

#include <lvgl/lvgl.h>
#include <stdio.h>
#include <string.h>

/****************************************************************************
 * Private data
 ****************************************************************************/

static lv_obj_t *g_alert_box;
static lv_obj_t *g_alert_title;
static lv_obj_t *g_alert_msg;
static lv_obj_t *g_alert_btn;
static lv_obj_t *g_alert_mask;
static lv_timer_t *g_auto_dismiss_timer;

#define COLOR_ALERT_BG   lv_color_hex(0x2d1b1b)
#define COLOR_ALERT_BORDER lv_color_hex(0xff3333)
#define COLOR_ALERT_TEXT  lv_color_hex(0xffcccc)
#define COLOR_ALERT_BTN   lv_color_hex(0xff3333)

/****************************************************************************
 * Private: 自动关闭定时器回调
 ****************************************************************************/

static void auto_dismiss_cb(lv_timer_t *timer)
{
  (void)timer;
  ui_alert_dismiss();
}

/****************************************************************************
 * Private: 按钮点击回调
 ****************************************************************************/

static void btn_click_cb(lv_event_t *e)
{
  (void)e;
  ui_alert_dismiss();
}

/****************************************************************************
 * Public API
 ****************************************************************************/

void ui_alert_init(void)
{
  /* 半透明遮罩 */

  g_alert_mask = lv_obj_create(lv_layer_top());
  lv_obj_set_size(g_alert_mask, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(g_alert_mask, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(g_alert_mask, LV_OPA_50, 0);
  lv_obj_set_style_border_width(g_alert_mask, 0, 0);
  lv_obj_set_style_radius(g_alert_mask, 0, 0);
  lv_obj_set_style_pad_all(g_alert_mask, 0, 0);
  lv_obj_add_flag(g_alert_mask, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(g_alert_mask, LV_OBJ_FLAG_CLICKABLE);

  /* 告警框 */

  g_alert_box = lv_obj_create(g_alert_mask);
  lv_obj_set_size(g_alert_box, LV_PCT(85), LV_SIZE_CONTENT);
  lv_obj_center(g_alert_box);
  lv_obj_set_style_bg_color(g_alert_box, COLOR_ALERT_BG, 0);
  lv_obj_set_style_bg_opa(g_alert_box, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(g_alert_box, COLOR_ALERT_BORDER, 0);
  lv_obj_set_style_border_width(g_alert_box, 2, 0);
  lv_obj_set_style_radius(g_alert_box, 12, 0);
  lv_obj_set_style_pad_all(g_alert_box, 16, 0);
  lv_obj_set_flex_flow(g_alert_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(g_alert_box, 8, 0);
  lv_obj_clear_flag(g_alert_box, LV_OBJ_FLAG_SCROLLABLE);

  /* 标题 */

  g_alert_title = lv_label_create(g_alert_box);
  lv_label_set_text(g_alert_title, "Alert");
  lv_obj_set_style_text_color(g_alert_title, COLOR_ALERT_BORDER, 0);
  lv_obj_set_style_text_font(g_alert_title, &lv_font_montserrat_16, 0);
  lv_obj_set_width(g_alert_title, LV_PCT(100));
  lv_obj_set_style_text_align(g_alert_title, LV_TEXT_ALIGN_CENTER, 0);

  /* 消息 */

  g_alert_msg = lv_label_create(g_alert_box);
  lv_label_set_text(g_alert_msg, "");
  lv_obj_set_style_text_color(g_alert_msg, COLOR_ALERT_TEXT, 0);
  lv_obj_set_style_text_font(g_alert_msg, &lv_font_montserrat_12, 0);
  lv_obj_set_width(g_alert_msg, LV_PCT(100));
  lv_label_set_long_mode(g_alert_msg, LV_LABEL_LONG_WRAP);

  /* 关闭按钮 */

  g_alert_btn = lv_btn_create(g_alert_box);
  lv_obj_set_size(g_alert_btn, LV_PCT(50), 32);
  lv_obj_set_style_bg_color(g_alert_btn, COLOR_ALERT_BTN, 0);
  lv_obj_set_style_radius(g_alert_btn, 6, 0);
  lv_obj_align(g_alert_btn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(g_alert_btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btn_label = lv_label_create(g_alert_btn);
  lv_label_set_text(btn_label, "Dismiss");
  lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
  lv_obj_center(btn_label);
}

void ui_alert_show(const char *title, const char *message)
{
  if (!g_alert_mask || !title || !message)
    {
      return;
    }

  lv_label_set_text(g_alert_title, title);
  lv_label_set_text(g_alert_msg, message);
  lv_obj_clear_flag(g_alert_mask, LV_OBJ_FLAG_HIDDEN);

  /* 10 秒后自动关闭 */

  if (g_auto_dismiss_timer)
    {
      lv_timer_delete(g_auto_dismiss_timer);
    }
  g_auto_dismiss_timer = lv_timer_create(auto_dismiss_cb, 10000, NULL);
  lv_timer_set_repeat_count(g_auto_dismiss_timer, 1);
}

void ui_alert_dismiss(void)
{
  if (g_alert_mask)
    {
      lv_obj_add_flag(g_alert_mask, LV_OBJ_FLAG_HIDDEN);
    }
  if (g_auto_dismiss_timer)
    {
      lv_timer_delete(g_auto_dismiss_timer);
      g_auto_dismiss_timer = NULL;
    }
}
