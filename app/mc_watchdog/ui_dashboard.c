/****************************************************************************
 * MC Watchdog AI — LVGL 仪表盘 UI
 * 服务器卡片网格 + 玩家数趋势折线图
 ****************************************************************************/

#include "mc_watchdog.h"

#include <lvgl/lvgl.h>

/****************************************************************************
 * Private data
 ****************************************************************************/

static lv_obj_t *g_dashboard_cont;  /* 根容器 */
static lv_obj_t *g_server_cards[MC_MAX_SERVERS];
static lv_obj_t *g_card_status[MC_MAX_SERVERS];
static lv_obj_t *g_card_name[MC_MAX_SERVERS];
static lv_obj_t *g_card_info[MC_MAX_SERVERS];
static lv_obj_t *g_chart;
static lv_chart_series_t *g_player_series;
static lv_chart_series_t *g_latency_series;
static int g_card_count;
static int g_selected = -1;

/* 颜色常量 */

#define COLOR_BG       lv_color_hex(0x0f0f23)
#define COLOR_CARD     lv_color_hex(0x1a1a2e)
#define COLOR_ONLINE   lv_color_hex(0x00cc66)
#define COLOR_WARN     lv_color_hex(0xffaa00)
#define COLOR_OFFLINE  lv_color_hex(0xff3333)
#define COLOR_TEXT     lv_color_hex(0xe0e0e0)
#define COLOR_CHART_BG lv_color_hex(0x16213e)
#define COLOR_SERIES1  lv_color_hex(0x00cc66)
#define COLOR_SERIES2  lv_color_hex(0x4488ff)

/****************************************************************************
 * Private: 创建单个服务器卡片
 ****************************************************************************/

static lv_obj_t *create_server_card(lv_obj_t *parent, int index)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, LV_PCT(100), 72);
  lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_pad_all(card, 10, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, COLOR_CARD, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  /* 状态指示灯 */

  lv_obj_t *dot = lv_obj_create(card);
  lv_obj_set_size(dot, 12, 12);
  lv_obj_set_style_radius(dot, 6, 0);
  lv_obj_set_style_bg_color(dot, COLOR_OFFLINE, 0);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(dot, 0, 0);
  lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
  g_card_status[index] = dot;

  /* 服务器名称 */

  lv_obj_t *name_label = lv_label_create(card);
  lv_label_set_text(name_label, "Server");
  lv_obj_set_style_text_color(name_label, COLOR_TEXT, 0);
  lv_obj_set_style_text_font(name_label, &lv_font_montserrat_16, 0);
  lv_obj_align(name_label, LV_ALIGN_LEFT_MID, 20, -12);
  g_card_name[index] = name_label;

  /* 状态信息 */

  lv_obj_t *info_label = lv_label_create(card);
  lv_label_set_text(info_label, "Offline");
  lv_obj_set_style_text_color(info_label, lv_color_hex(0x888888), 0);
  lv_obj_set_style_text_font(info_label, &lv_font_montserrat_12, 0);
  lv_obj_align(info_label, LV_ALIGN_LEFT_MID, 20, 12);
  g_card_info[index] = info_label;

  /* 点击事件 */

  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_user_data(card, (void *)(intptr_t)index);

  return card;
}

/****************************************************************************
 * Public API
 ****************************************************************************/

void ui_dashboard_init(void *parent)
{
  lv_obj_t *cont = (lv_obj_t *)parent;
  lv_obj_set_style_bg_color(cont, COLOR_BG, 0);
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(cont, 8, 0);
  lv_obj_set_style_pad_row(cont, 6, 0);

  g_dashboard_cont = cont;

  /* 标题 */

  lv_obj_t *title = lv_label_create(cont);
  lv_label_set_text(title, "MC Watchdog AI");
  lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_set_width(title, LV_PCT(100));
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

  /* 服务器卡片容器 */

  lv_obj_t *cards_cont = lv_obj_create(cont);
  lv_obj_set_size(cards_cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(cards_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(cards_cont, 0, 0);
  lv_obj_set_style_pad_row(cards_cont, 4, 0);
  lv_obj_set_style_bg_opa(cards_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cards_cont, 0, 0);
  lv_obj_clear_flag(cards_cont, LV_OBJ_FLAG_SCROLLABLE);

  /* 图表容器 */

  lv_obj_t *chart_cont = lv_obj_create(cont);
  lv_obj_set_size(chart_cont, LV_PCT(100), 140);
  lv_obj_set_style_bg_color(chart_cont, COLOR_CHART_BG, 0);
  lv_obj_set_style_bg_opa(chart_cont, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(chart_cont, 8, 0);
  lv_obj_set_style_pad_all(chart_cont, 4, 0);
  lv_obj_set_style_border_width(chart_cont, 0, 0);
  lv_obj_clear_flag(chart_cont, LV_OBJ_FLAG_SCROLLABLE);

  /* 创建图表 */

  g_chart = lv_chart_create(chart_cont);
  lv_obj_set_size(g_chart, LV_PCT(100), LV_PCT(100));
  lv_obj_align(g_chart, LV_ALIGN_CENTER, 0, 0);
  lv_chart_set_type(g_chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(g_chart, 60);
  lv_chart_set_range_min_value(g_chart, LV_CHART_AXIS_PRIMARY_Y, 0);
  lv_chart_set_range_max_value(g_chart, LV_CHART_AXIS_PRIMARY_Y, 100);
  lv_chart_set_range_min_value(g_chart, LV_CHART_AXIS_SECONDARY_Y, 0);
  lv_chart_set_range_max_value(g_chart, LV_CHART_AXIS_SECONDARY_Y, 500);

  /* 在线人数系列 (主 Y 轴) */

  g_player_series = lv_chart_add_series(g_chart, COLOR_SERIES1,
                                         LV_CHART_AXIS_PRIMARY_Y, NULL);
  lv_chart_set_ext_y_array(g_chart, g_player_series, NULL);

  /* 延迟系列 (次 Y 轴) */

  g_latency_series = lv_chart_add_series(g_chart, COLOR_SERIES2,
                                          LV_CHART_AXIS_SECONDARY_Y, NULL);
  lv_chart_set_ext_y_array(g_chart, g_latency_series, NULL);

  /* 图例 */

  lv_obj_t *legend = lv_label_create(chart_cont);
  lv_label_set_text(legend, "Players (g)  Latency (b)");
  lv_obj_set_style_text_color(legend, COLOR_TEXT, 0);
  lv_obj_set_style_text_font(legend, &lv_font_montserrat_10, 0);
  lv_obj_align(legend, LV_ALIGN_TOP_RIGHT, -4, 2);
}

void ui_dashboard_update(const mc_watchdog_ctx_t *ctx)
{
  int i;
  int32_t players[60];
  int32_t latencies[60];

  if (!ctx || !g_dashboard_cont)
    {
      return;
    }

  /* 更新每个服务器卡片 */

  for (i = 0; i < ctx->server_count && i < MC_MAX_SERVERS; i++)
    {
      const mc_server_info_t *s = &ctx->servers[i];

      if (!g_server_cards[i])
        {
          g_server_cards[i] = create_server_card(g_dashboard_cont, i);
        }

      /* 状态灯颜色 */

      lv_color_t dot_color;
      switch (s->state)
        {
          case MC_SERVER_ONLINE:
            dot_color = COLOR_ONLINE;
            break;
          case MC_SERVER_WARNING:
            dot_color = COLOR_WARN;
            break;
          case MC_SERVER_OFFLINE:
          case MC_SERVER_ERROR:
          default:
            dot_color = COLOR_OFFLINE;
            break;
        }
      lv_obj_set_style_bg_color(g_card_status[i], dot_color, 0);

      /* 服务器名称 */

      lv_label_set_text(g_card_name[i], s->name);

      /* 状态信息 */

      char info[128];
      if (s->state == MC_SERVER_ONLINE || s->state == MC_SERVER_WARNING)
        {
          snprintf(info, sizeof(info), "%d/%d  %dms  %s",
                   s->online_players, s->max_players,
                   s->latency_ms, s->version);
        }
      else
        {
          snprintf(info, sizeof(info), "Offline");
        }
      lv_label_set_text(g_card_info[i], info);

      /* 选中高亮 */

      if (i == ctx->selected_server)
        {
          lv_obj_set_style_border_color(g_server_cards[i],
                                         COLOR_SERIES1, 0);
        }
      else
        {
          lv_obj_set_style_border_color(g_server_cards[i],
                                         COLOR_CARD, 0);
        }
    }

  /* 更新图表（使用第一个服务器的历史数据） */

  if (ctx->server_count > 0 && g_chart)
    {
      const mc_server_info_t *s = &ctx->servers[0];
      int idx = s->history_idx;
      int j;

      for (j = 0; j < 60; j++)
        {
          int src = (idx + j) % 60;
          players[j] = s->player_history[src];
          latencies[j] = s->latency_history[src] > 0 ?
                          s->latency_history[src] : 0;
        }

      lv_chart_set_ext_y_array(g_chart, g_player_series, players);
      lv_chart_set_ext_y_array(g_chart, g_latency_series, latencies);
      lv_chart_refresh(g_chart);
    }

  g_card_count = ctx->server_count;
}

void ui_dashboard_set_selected(int index)
{
  g_selected = index;
}
