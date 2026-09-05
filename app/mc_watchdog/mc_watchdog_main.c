/****************************************************************************
 * MC Watchdog AI — Minecraft 智能运维终端
 * 主入口：LVGL UI 初始化 + 网络轮询 + ai_agent 集成
 ****************************************************************************/

#include "mc_watchdog.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <lvgl/lvgl.h>

/****************************************************************************
 * Private data
 ****************************************************************************/

static mc_watchdog_ctx_t g_ctx;
static lv_timer_t *g_poll_timer;

/****************************************************************************
 * Private: 默认服务器配置（演示用）
 ****************************************************************************/

static void init_default_servers(mc_watchdog_ctx_t *ctx)
{
  ctx->server_count = 0;
  ctx->selected_server = -1;
  ctx->poll_interval_s = 30;
  ctx->alert_enabled = true;
  ctx->latency_threshold = 200;
  ctx->tps_threshold = 15;

  /* 示例服务器（可通过 RCON 或配置文件修改） */

  strncpy(ctx->servers[0].name, "Survival", sizeof(ctx->servers[0].name) - 1);
  strncpy(ctx->servers[0].host, "127.0.0.1", sizeof(ctx->servers[0].host) - 1);
  ctx->servers[0].port = 25565;
  strncpy(ctx->servers[0].rcon_host, "127.0.0.1",
          sizeof(ctx->servers[0].rcon_host) - 1);
  ctx->servers[0].rcon_port = 25575;
  strncpy(ctx->servers[0].rcon_password, "password",
          sizeof(ctx->servers[0].rcon_password) - 1);
  ctx->server_count = 1;
}

/****************************************************************************
 * Private: LVGL 定时轮询回调
 ****************************************************************************/

static void poll_timer_cb(lv_timer_t *timer)
{
  mc_watchdog_ctx_t *ctx = (mc_watchdog_ctx_t *)timer->user_data;
  int i;
  bool need_alert = false;
  char alert_msg[256];

  if (!ctx)
    {
      return;
    }

  /* 逐个查询服务器状态 */

  for (i = 0; i < ctx->server_count; i++)
    {
      mc_server_info_t *s = &ctx->servers[i];
      mc_server_state_t old_state = s->state;

      mc_slp_query(s->host, s->port, s);

      /* 检测状态变化 -> 触发告警 */

      if (ctx->alert_enabled && old_state != s->state)
        {
          if (s->state == MC_SERVER_OFFLINE ||
              s->state == MC_SERVER_ERROR)
            {
              snprintf(alert_msg, sizeof(alert_msg),
                       "%s is now OFFLINE!\n%s:%d",
                       s->name, s->host, s->port);
              ui_alert_show("Server Down", alert_msg);
              need_alert = true;
            }
          else if (s->state == MC_SERVER_WARNING)
            {
              snprintf(alert_msg, sizeof(alert_msg),
                       "%s high latency: %dms",
                       s->name, s->latency_ms);
              ui_alert_show("Warning", alert_msg);
            }
        }
    }

  /* 更新 UI */

  ui_dashboard_update(ctx);

  /* 通知 AI agent（如果需要告警） */

  if (need_alert)
    {
      char ai_msg[512];
      snprintf(ai_msg, sizeof(ai_msg),
               "MC服务器 %s 掉线了，请帮我检查一下并通知服主",
               ctx->servers[0].name);
      ai_integration_query(ai_msg);
    }
}

/****************************************************************************
 * Public: 应用入口
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  lv_obj_t *scr;

  (void)argc;
  (void)argv;

  printf("[MC Watchdog] Starting...\n");

  /* 初始化默认服务器配置 */

  init_default_servers(&g_ctx);

  /* 获取当前活跃屏幕（系统 LVGL 已由 launcher 初始化） */

  scr = lv_scr_act();
  if (!scr)
    {
      printf("[MC Watchdog] No active LVGL screen\n");
      return -1;
    }

  /* 初始化 UI 模块 */

  ui_alert_init();
  ui_dashboard_init(scr);

  /* 初始化 ai_agent 集成 */

  if (ai_integration_init(&g_ctx) < 0)
    {
      printf("[MC Watchdog] Warning: ai_agent not available\n");
    }

  /* 启动定时轮询（30 秒间隔） */

  g_poll_timer = lv_timer_create(poll_timer_cb,
                                  g_ctx.poll_interval_s * 1000,
                                  &g_ctx);
  if (!g_poll_timer)
    {
      printf("[MC Watchdog] Failed to create poll timer\n");
    }

  /* 首次立即查询 */

  poll_timer_cb(g_poll_timer);

  printf("[MC Watchdog] Initialized, polling every %ds\n",
         g_ctx.poll_interval_s);

  /* 主循环由系统 LVGL 驱动，此处无需额外循环 */

  return 0;
}
