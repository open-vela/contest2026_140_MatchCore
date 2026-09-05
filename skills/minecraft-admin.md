# Minecraft Admin

MC 服务器运维助手，帮助服主通过自然语言管理 Minecraft 服务器。

## When to use

当用户提到 Minecraft、MC、服务器管理、玩家管理、服务器状态等话题时触发。
触发词：Minecraft、MC、服务器、玩家、白名单、封禁、踢人、OP、难度、天气、时间、RCON。

## How to use

1. 理解用户意图，判断是「查询状态」还是「执行指令」还是「告警处理」
2. 如果是查询状态：
   - 回复用户当前服务器的在线人数、延迟、版本等信息
   - 如果有历史数据，可以给出趋势分析
3. 如果是执行指令：
   - 将自然语言转换为对应的 RCON 命令
   - 通过回复中包含 "RCON:<command>" 前缀来触发端侧 RCON 执行
   - 常用指令映射：
     - "查看在线玩家" → "list"
     - "踢出 <玩家>" → "kick <player>"
     - "封禁 <玩家>" → "ban <player>"
     - "解封 <玩家>" → "pardon <player>"
     - "加白名单 <玩家>" → "whitelist add <player>"
     - "移除白名单 <玩家>" → "whitelist remove <player>"
     - "给OP <玩家>" → "op <player>"
     - "撤销OP <玩家>" → "deop <player>"
     - "设置时间 <值>" → "time set <value>"
     - "设置天气 <晴/雨/雷>" → "weather clear/rain/thunder"
     - "设置难度 <简单/普通/困难>" → "difficulty easy/normal/hard"
     - "广播消息 <内容>" → "say <message>"
     - "保存世界" → "save-all"
     - "停止服务器" → "stop"
4. 如果是告警处理：
   - 分析告警原因（掉线、高延迟、TPS 低等）
   - 给出排查建议
   - 必要时自动执行诊断命令（如 "tps"、"list"、"save-all"）

## Example

User: "生存服多少人在线？"
-> 分析意图：查询服务器状态
-> "当前生存服有 12/100 人在线，延迟 45ms，运行 Minecraft 1.20.4。"

User: "把 Steve 踢了"
-> 分析意图：执行 RCON 指令
-> "好的，正在执行踢人指令。"
-> RCON:kick Steve

User: "服务器好像卡了"
-> 分析意图：诊断问题
-> "让我帮你检查一下。请稍等..."
-> RCON:tps
-> RCON:list
-> 根据 TPS 和玩家数量给出诊断建议。

User: "每天早上8点提醒我检查服务器"
-> 分析意图：定时任务
-> 使用 cron_add 设置每日定时提醒
-> "已设置每日 08:00 自动检查服务器状态并播报。"
