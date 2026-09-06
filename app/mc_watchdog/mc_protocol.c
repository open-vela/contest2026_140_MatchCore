/****************************************************************************
 * MC Watchdog AI — Minecraft SLP / Query / RCON 协议实现
 ****************************************************************************/

#include "mc_watchdog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/****************************************************************************
 * SLP (Server List Ping) — Java Edition Status Protocol
 * 基于 wiki.vg 规范：
 *   Handshake (0x00) -> Status Request (0x00) <- Status Response (JSON)
 ****************************************************************************/

/* VarInt 编码 */

static int encode_varint(uint8_t *buf, int32_t value)
{
  int idx = 0;
  uint32_t uval = (uint32_t)value;
  do {
    uint8_t byte = uval & 0x7f;
    uval >>= 7;
    if (uval != 0)
      {
        byte |= 0x80;
      }
    buf[idx++] = byte;
  }
  while (uval != 0);
  return idx;
}

/* 简易 JSON 字段提取（避免引入完整 JSON 库） */

static const char *json_get_string(const char *json, const char *key,
                                   char *out, size_t out_size)
{
  char pattern[128];
  const char *start;
  const char *end;
  size_t len;

  snprintf(pattern, sizeof(pattern), "\"%s\":", key);
  start = strstr(json, pattern);
  if (!start)
    {
      return NULL;
    }

  start += strlen(pattern);
  while (*start == ' ')
    {
      start++;
    }

  if (*start != '"')
    {
      return NULL;
    }
  start++;

  end = strchr(start, '"');
  if (!end)
    {
      return NULL;
    }

  len = (size_t)(end - start);
  if (len >= out_size)
    {
      len = out_size - 1;
    }
  memcpy(out, start, len);
  out[len] = '\0';
  return out;
}

static int32_t json_get_int(const char *json, const char *key)
{
  char pattern[128];
  const char *start;

  snprintf(pattern, sizeof(pattern), "\"%s\":", key);
  start = strstr(json, pattern);
  if (!start)
    {
      return -1;
    }

  start += strlen(pattern);
  return (int32_t)strtol(start, NULL, 10);
}

/****************************************************************************
 * Public: SLP Query
 ****************************************************************************/

int mc_slp_query(const char *host, uint16_t port, mc_server_info_t *info)
{
  int sock;
  struct sockaddr_in addr;
  struct timeval tv;
  uint8_t buf[2048];
  uint8_t packet[256];
  int pkt_len;
  int n;
  char json_buf[2048];
  char tmp[256];
  struct timespec ts_start;
  struct timespec ts_end;

  if (!host || !info)
    {
      return -EINVAL;
    }

  info->state = MC_SERVER_OFFLINE;
  info->latency_ms = -1;

  /* 创建 TCP socket */

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      info->state = MC_SERVER_ERROR;
      return -errno;
    }

  /* 3 秒超时 */

  tv.tv_sec = 3;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  /* 解析地址 */

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
    {
      struct hostent *he = gethostbyname(host);
      if (!he || !he->h_addr_list[0])
        {
          close(sock);
          info->state = MC_SERVER_ERROR;
          return -ENOENT;
        }
      memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

  /* 连接并计时 */

  clock_gettime(CLOCK_MONOTONIC, &ts_start);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      close(sock);
      info->state = MC_SERVER_OFFLINE;
      return -errno;
    }

  /* 构建 Handshake 包：
   * Length + VarInt(0x00) + VarInt(protocol) + Host + UShort(port) + VarInt(1) */

  {
    int host_len = (int)strlen(host);
    int body_len;
    int idx = 0;

    /* packet body (不含长度前缀) */
    idx += encode_varint(packet + idx, 0x00);       /* Packet ID */
    idx += encode_varint(packet + idx, 764);        /* Protocol 764 (1.20.4) */
    idx += encode_varint(packet + idx, host_len);   /* Host length */
    memcpy(packet + idx, host, host_len);           /* Host */
    idx += host_len;
    packet[idx++] = (port >> 8) & 0xff;             /* Port high byte */
    packet[idx++] = port & 0xff;                    /* Port low byte */
    idx += encode_varint(packet + idx, 1);          /* Next state: Status */

    body_len = idx;
    pkt_len = encode_varint(buf, body_len);
    memcpy(buf + pkt_len, packet, body_len);
    pkt_len += body_len;

    if (send(sock, buf, pkt_len, 0) != pkt_len)
      {
        close(sock);
        info->state = MC_SERVER_ERROR;
        return -EIO;
      }
  }

  /* 发送 Status Request */

  {
    int body_len;
    int idx = 0;

    idx += encode_varint(packet, 0x00);  /* Packet ID: Status Request */
    body_len = idx;
    pkt_len = encode_varint(buf, body_len);
    memcpy(buf + pkt_len, packet, body_len);
    pkt_len += body_len;

    if (send(sock, buf, pkt_len, 0) != pkt_len)
      {
        close(sock);
        info->state = MC_SERVER_ERROR;
        return -EIO;
      }
  }

  /* 读取 Status Response */

  n = recv(sock, buf, sizeof(buf), 0);
  clock_gettime(CLOCK_MONOTONIC, &ts_end);

  close(sock);

  if (n <= 0)
    {
      info->state = MC_SERVER_OFFLINE;
      return (n == 0) ? -ECONNRESET : -errno;
    }

  /* 计算延迟 */

  info->latency_ms = (int32_t)((ts_end.tv_sec - ts_start.tv_sec) * 1000 +
                     (ts_end.tv_nsec - ts_start.tv_nsec) / 1000000);

  /* 解析响应：跳过长度 VarInt 和 Packet ID VarInt，读取 JSON 长度 + JSON */

  {
    int pos = 0;
    uint8_t json_len_buf;
    uint32_t json_length = 0;
    int shift = 0;

    /* 跳过 packet length VarInt */

    while (pos < n && (buf[pos] & 0x80))
      {
        pos++;
      }
    pos++; /* skip last byte of VarInt */

    /* 跳过 packet ID VarInt */

    while (pos < n && (buf[pos] & 0x80))
      {
        pos++;
      }
    pos++;

    /* 读取 JSON string length VarInt */

    do
      {
        if (pos >= n)
          {
            info->state = MC_SERVER_ERROR;
            return -EPROTO;
          }
        json_len_buf = buf[pos++];
        json_length |= (uint32_t)(json_len_buf & 0x7f) << shift;
        shift += 7;
      }
    while (json_len_buf & 0x80);

    /* 拷贝 JSON */

    if (pos + json_length > (uint32_t)n)
      {
        info->state = MC_SERVER_ERROR;
        return -EPROTO;
      }

    if (json_length >= sizeof(json_buf))
      {
        json_length = sizeof(json_buf) - 1;
      }
    memcpy(json_buf, buf + pos, json_length);
    json_buf[json_length] = '\0';
  }

  /* 解析 JSON 字段 */

  if (json_get_string(json_buf, "version", tmp, sizeof(tmp)))
    {
      json_get_string(tmp, "name", info->version, sizeof(info->version));
    }

  if (json_get_string(json_buf, "description", tmp, sizeof(tmp)))
    {
      json_get_string(tmp, "text", info->motd, sizeof(info->motd));
    }

  if (json_get_string(json_buf, "players", tmp, sizeof(tmp)))
    {
      info->online_players = json_get_int(tmp, "online");
      info->max_players = json_get_int(tmp, "max");
    }

  /* 记录历史 */

  info->player_history[info->history_idx] = info->online_players;
  info->latency_history[info->history_idx] = info->latency_ms;
  info->history_idx = (info->history_idx + 1) % 60;

  info->state = MC_SERVER_ONLINE;
  info->queried = true;
  info->last_query_time = time(NULL);

  return 0;
}

/****************************************************************************
 * Public: RCON (Source RCON Protocol)
 ****************************************************************************/

int mc_rcon_send(const char *host, uint16_t port,
                 const char *password, const char *command,
                 char *response, size_t response_size)
{
  int sock;
  struct sockaddr_in addr;
  struct timeval tv;
  uint8_t buf[4096];
  int id;
  int body_len;
  int idx;
  int n;

  if (!host || !password || !command || !response)
    {
      return -EINVAL;
    }

  response[0] = '\0';

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      return -errno;
    }

  tv.tv_sec = 5;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host, &addr.sin_addr) != 1)
    {
      struct hostent *he = gethostbyname(host);
      if (!he || !he->h_addr_list[0])
        {
          close(sock);
          return -ENOENT;
        }
      memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      close(sock);
      return -errno;
    }

  /* --- Auth packet --- */

  id = 1;
  body_len = 4 + (int)strlen(password) + 1 + 0 + 1; /* id + pass + \0 + cmd + \0 */

  idx = 0;
  /* Length (LE int32) */
  buf[idx++] = (uint8_t)(body_len & 0xff);
  buf[idx++] = (uint8_t)((body_len >> 8) & 0xff);
  buf[idx++] = (uint8_t)((body_len >> 16) & 0xff);
  buf[idx++] = (uint8_t)((body_len >> 24) & 0xff);
  /* Request ID (LE int32) */
  buf[idx++] = (uint8_t)(id & 0xff);
  buf[idx++] = (uint8_t)((id >> 8) & 0xff);
  buf[idx++] = (uint8_t)((id >> 16) & 0xff);
  buf[idx++] = (uint8_t)((id >> 24) & 0xff);
  /* Type: Auth (3) */
  buf[idx++] = 3;
  buf[idx++] = 0;
  /* Password */
  memcpy(buf + idx, password, strlen(password));
  idx += (int)strlen(password);
  buf[idx++] = '\0';
  /* Empty payload */
  buf[idx++] = '\0';

  if (send(sock, buf, idx, 0) != idx)
    {
      close(sock);
      return -EIO;
    }

  /* 读取 auth response */

  n = recv(sock, buf, sizeof(buf), 0);
  if (n < 4)
    {
      close(sock);
      return -EPROTO;
    }

  {
    int resp_id = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
    if (resp_id == -1)
      {
        close(sock);
        return -EACCES; /* 认证失败 */
      }
  }

  /* --- Command packet --- */

  id = 2;
  body_len = 4 + (int)strlen(command) + 1 + 0 + 1;

  idx = 0;
  buf[idx++] = (uint8_t)(body_len & 0xff);
  buf[idx++] = (uint8_t)((body_len >> 8) & 0xff);
  buf[idx++] = (uint8_t)((body_len >> 16) & 0xff);
  buf[idx++] = (uint8_t)((body_len >> 24) & 0xff);
  buf[idx++] = (uint8_t)(id & 0xff);
  buf[idx++] = (uint8_t)((id >> 8) & 0xff);
  buf[idx++] = (uint8_t)((id >> 16) & 0xff);
  buf[idx++] = (uint8_t)((id >> 24) & 0xff);
  buf[idx++] = 2; /* Type: Command */
  buf[idx++] = 0;
  memcpy(buf + idx, command, strlen(command));
  idx += (int)strlen(command);
  buf[idx++] = '\0';
  buf[idx++] = '\0';

  if (send(sock, buf, idx, 0) != idx)
    {
      close(sock);
      return -EIO;
    }

  /* 读取命令响应 */

  n = recv(sock, buf, sizeof(buf), 0);
  close(sock);

  if (n < 4 + 4 + 1 + 1)
    {
      return -EPROTO;
    }

  /* 跳过 length(4) + id(4) + type(2)，取 payload */

  {
    size_t payload_len = n - 4 - 4 - 1 - 1;
    if (payload_len >= response_size)
      {
        payload_len = response_size - 1;
      }
    memcpy(response, buf + 4 + 4 + 1, payload_len);
    response[payload_len] = '\0';
  }

  return 0;
}
