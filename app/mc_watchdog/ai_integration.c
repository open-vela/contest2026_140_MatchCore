/****************************************************************************
 * MC Watchdog AI — ai_agent (VelaClaw) 集成
 * 通过 velaclaw_client API 实现语音/文字运维指令
 ****************************************************************************/

#include "mc_watchdog.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <velaclaw/client.h>

/****************************************************************************
 * Private data
 ****************************************************************************/

static velaclaw_client_t *g_client;
static mc_watchdog_ctx_t *g_ctx;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/****************************************************************************
 * Private: AI 回调处理
 ****************************************************************************/

static void ask_callback(int status, const char *reply, void *cookie)
{
  (void)cookie;

  if (status != 0 || !reply)
    {
      printf("[MC Watchdog] AI query failed: status=%d\n", status);
      return;
    }

  printf("[MC Watchdog] AI reply: %s\n", reply);

  /* TODO: 解析 AI 回复，如果是运维指令则执行 RCON */

  /* 示例：如果回复包含 "RCON:" 前缀，则提取命令并执行 */

  if (strncmp(reply, "RCON:", 5) == 0)
    {
      const char *cmd = reply + 5;
      char rcon_resp[1024];

      printf("[MC Watchdog] Executing RCON: %s\n", cmd);

      pthread_mutex_lock(&g_mutex);
      if (g_ctx && g_ctx->server_count > 0)
        {
          mc_server_info_t *s = &g_ctx->servers[0];
          if (s->rcon_host[0] && s->rcon_password[0])
            {
              mc_rcon_send(s->rcon_host, s->rcon_port,
                           s->rcon_password, cmd,
                           rcon_resp, sizeof(rcon_resp));
              printf("[MC Watchdog] RCON response: %s\n", rcon_resp);
            }
        }
      pthread_mutex_unlock(&g_mutex);
    }
}

/****************************************************************************
 * Public API
 ****************************************************************************/

int ai_integration_init(mc_watchdog_ctx_t *ctx)
{
  if (!ctx)
    {
      return -EINVAL;
    }

  g_ctx = ctx;

  /* 打开 velaclaw 客户端 */

  g_client = velaclaw_client_open("mc_watchdog");
  if (!g_client)
    {
      printf("[MC Watchdog] Failed to open velaclaw client\n");
      return -1;
    }

  printf("[MC Watchdog] VelaClaw client opened\n");
  return 0;
}

void ai_integration_deinit(void)
{
  if (g_client)
    {
      velaclaw_client_close(g_client);
      g_client = NULL;
    }
  g_ctx = NULL;
}

int ai_integration_query(const char *text)
{
  velaclaw_ask_req_t req;

  if (!g_client || !text)
    {
      return -EINVAL;
    }

  req.text = text;
  req.timeout_ms = 30000; /* 30 秒超时 */

  return velaclaw_ask(g_client, &req, ask_callback, NULL);
}
