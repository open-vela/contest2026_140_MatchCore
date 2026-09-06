/****************************************************************************
 * MC Watchdog AI — Minecraft 智能运维终端
 * 共享类型与声明
 ****************************************************************************/

#ifndef MC_WATCHDOG_H
#define MC_WATCHDOG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 服务器状态结构体 */

#define MC_MAX_SERVERS 8
#define MC_MAX_PLAYERS 64
#define MC_ADDR_LEN 64
#define MC_MOTD_LEN 256
#define MC_PLAYER_NAME_LEN 32

typedef enum {
  MC_SERVER_OFFLINE = 0,
  MC_SERVER_ONLINE,
  MC_SERVER_WARNING, /* 延迟高或 TPS 低 */
  MC_SERVER_ERROR,   /* 连接异常 */
} mc_server_state_t;

typedef struct {
  char name[MC_ADDR_LEN];      /* 服务器显示名称 */
  char host[MC_ADDR_LEN];      /* 地址 */
  uint16_t port;                /* 默认 25565 */
  char rcon_host[MC_ADDR_LEN]; /* RCON 地址（可同 host） */
  uint16_t rcon_port;           /* RCON 端口，默认 25575 */
  char rcon_password[64];       /* RCON 密码 */

  /* 运行时状态（由轮询线程更新） */
  mc_server_state_t state;
  char version[32];
  char motd[MC_MOTD_LEN];
  int32_t online_players;
  int32_t max_players;
  int32_t latency_ms;          /* 往返延迟 */
  int64_t last_query_time;     /* 上次查询时间戳 */
  bool queried;                /* 是否已查询过 */

  /* 历史数据（环形缓冲） */
  int32_t player_history[60];  /* 最近 60 个采样点的在线人数 */
  int32_t latency_history[60]; /* 最近 60 个采样点的延迟 */
  int32_t history_idx;         /* 当前写入位置 */
} mc_server_info_t;

typedef struct {
  mc_server_info_t servers[MC_MAX_SERVERS];
  int server_count;
  int selected_server;         /* 当前选中的服务器索引，-1 表示无 */
  int poll_interval_s;         /* 轮询间隔（秒） */
  bool alert_enabled;          /* 告警开关 */
  int32_t latency_threshold;   /* 延迟告警阈值（ms） */
  int32_t tps_threshold;       /* TPS 告警阈值 */
} mc_watchdog_ctx_t;

/* ---- mc_protocol.c ---- */

int mc_slp_query(const char *host, uint16_t port, mc_server_info_t *info);
int mc_rcon_send(const char *host, uint16_t port,
                 const char *password, const char *command,
                 char *response, size_t response_size);

/* ---- ui_dashboard.c ---- */

void ui_dashboard_init(void *parent);
void ui_dashboard_update(const mc_watchdog_ctx_t *ctx);
void ui_dashboard_set_selected(int index);

/* ---- ui_alert.c ---- */

void ui_alert_init(void);
void ui_alert_show(const char *title, const char *message);
void ui_alert_dismiss(void);

/* ---- ai_integration.c ---- */

int ai_integration_init(mc_watchdog_ctx_t *ctx);
void ai_integration_deinit(void);
int ai_integration_query(const char *text);

#endif /* MC_WATCHDOG_H */
