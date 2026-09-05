# MC Watchdog AI — Minecraft 智能运维终端

## 一、作品简介

MC Watchdog AI 是一款运行在 openvela 上的 **Minecraft 服务器智能监控与运维终端**。它通过 SLP（Server List Ping）协议实时轮询多个 MC 服务器状态，在 LVGL 仪表盘上展示在线人数、延迟、版本等信息及历史趋势折线图；当检测到服务器掉线或高延迟时自动弹出告警，并通过 openvela **ai_agent（VelaClaw）** 将告警上下文发送给 AI，由 AI 理解后生成运维建议或自动执行 RCON 指令（踢人、封禁、广播、保存世界等）。

**一句话**：用 AI 管 MC 服——掉线自动通知、自然语言远程运维。

**亮点**：

- 嵌入式端侧 AI Agent 集成（VelaClaw），无需外挂电脑即可语音/文字运维
- 完整的 Minecraft 网络协议实现（SLP + RCON），纯 C、无第三方依赖
- LVGL 暗色主题仪表盘，支持多服务器卡片 + 实时折线图
- 可配置告警阈值、轮询间隔，支持最多 8 台服务器同时监控

## 二、选题方向

**AI 硬件产品创新** + 快应用方向融合。

理由：作品深度使用了 openvela 的 LVGL 图形能力、ai_agent 框架和网络栈，同时沉淀了 `minecraft-admin` Skill（自然语言 → RCON 命令映射），兼顾技术深度与产品创新。

## 三、目录结构

```text
contest2026_140_MatchCore/
├── app/
│   ├── mc_watchdog/              # 核心应用（本作品主体）
│   │   ├── mc_watchdog_main.c    # 主入口：LVGL 初始化 + 定时轮询 + AI 集成
│   │   ├── mc_watchdog.h         # 共享类型定义（服务器信息、上下文）
│   │   ├── mc_protocol.c         # Minecraft SLP 状态查询 + RCON 远程命令
│   │   ├── ui_dashboard.c        # LVGL 仪表盘（服务器卡片 + 趋势折线图）
│   │   ├── ui_alert.c            # 告警弹窗 UI
│   │   ├── ai_integration.c      # ai_agent（VelaClaw）集成：告警→AI→RCON
│   │   ├── CMakeLists.txt        # CMake 构建配置
│   │   ├── Makefile              # NuttX Make 构建配置
│   │   └── Kconfig               # menuconfig 选项
│   └── hello_app/                # 示例骨架（可删除）
├── quickapp/
│   └── hello_quickapp/           # 快应用示例骨架（可删除）
├── board/
│   └── contest_board/            # 板级适配骨架（可替换为真实板层）
├── logs/                         # AI Coding 日志（提交前导出真实日志）
├── skills/
│   └── minecraft-admin.md        # 比赛 Skill：自然语言 MC 服务器运维
├── contest2026_140_MatchCore.xml # repo manifest（linkfile 映射）
├── openvela.xml                  # openvela 基础 manifest
└── README.md                     # 本文件
```

## 四、运行方式

### 前置条件

```bash
# 在 openvela 工作区根目录（contest2026_140_MatchCore/ 的上一级）
source build/envsetup.sh
```

### 方式一：Goldfish 模拟器（推荐初赛使用）

```bash
cd /path/to/opvl  # openvela 工作区根目录

# 编译（Goldfish arm64 模拟器板层）
./build.sh vendor/openvela/boards/vela/configs/goldfish-arm64-v8a-ap

# 启动模拟器
./emulator.sh cmake_out/vela_goldfish-arm64-v8a-ap
```

在模拟器中通过 `nsh` 启动 `mc_watchdog`：

```bash
nsh> mc_watchdog
[MC Watchdog] Starting...
[MC Watchdog] Initialized, polling every 30s
```

### 方式二：真机（Gemini-S1 / BES2600WM 等）

1. 在 `menuconfig` 中启用 `LVX_USE_DEMO_MC_WATCHDOG`（位于 `Graphics Support → LVX Demos`）
2. 根据目标板层执行 `./build.sh <board-config-path>`
3. 烧录到开发板，通过 NSH shell 启动 `mc_watchdog`

### Kconfig 选项

| 选项 | 默认值 | 说明 |
|---|---|---|
| `LVX_USE_DEMO_MC_WATCHDOG` | n | 启用 MC Watchdog AI |
| `LVX_MC_WATCHDOG_POLL_INTERVAL` | 30 | 服务器轮询间隔（秒） |
| `LVX_MC_WATCHDOG_MAX_SERVERS` | 8 | 最大监控服务器数 |
| `LVX_MC_WATCHDOG_LATENCY_THRESHOLD` | 200 | 延迟告警阈值（ms） |

依赖项：`GRAPHICS_LVGL`、`NET_TCP`、`EXAMPLES_AI_AGENT_VELA`

### 配置服务器

默认配置连接 `127.0.0.1:25565`（RCON 端口 25575）。修改 [mc_watchdog_main.c](app/mc_watchdog/mc_watchdog_main.c) 中的 `init_default_servers()` 或通过 RCON/配置文件调整：

- `host` / `port`：MC 服务器地址
- `rcon_host` / `rcon_port` / `rcon_password`：RCON 远程管理凭据

## 五、AI Coding 使用说明

本作品全程使用 AI 辅助开发：

- **需求拆解与架构设计**：通过 AI 分析 Minecraft SLP/RCON 协议规范，设计了模块化的 C 代码架构（protocol / UI / AI integration 三层分离）
- **编码实现**：协议解析（VarInt 编码、JSON 字段提取）、LVGL UI 组件、VelaClaw 集成均由 AI 辅助生成初稿后人工审核
- **Skill 沉淀**：创建了 [minecraft-admin](skills/minecraft-admin.md) Skill，实现自然语言到 RCON 命令的映射，支持查询状态、踢人、封禁、白名单、天气/时间/难度设置等 15+ 种运维操作
- **调试与优化**：AI 协助排查 LVGL 内存布局、socket 超时配置等问题

完整对话日志见 [logs/](logs/) 目录。

### 核心技术点

| 模块 | 技术 | 说明 |
|---|---|---|
| `mc_protocol.c` | Minecraft SLP / RCON | 纯 C 实现 Java Edition Status Protocol（Protocol 764）和 Source RCON 协议 |
| `ui_dashboard.c` | LVGL 9.x | 暗色主题仪表盘，服务器卡片网格 + 双 Y 轴折线图（玩家数/延迟） |
| `ui_alert.c` | LVGL 弹窗 | 半透明遮罩 + 告警框，10 秒自动关闭 |
| `ai_integration.c` | VelaClaw API | 通过 `velaclaw_client` 接入 ai_agent，AI 回复含 `RCON:` 前缀时自动执行远程命令 |
| `minecraft-admin.md` | AI Skill | 自然语言 → RCON 命令映射，覆盖 15+ 种服务器运维操作 |
