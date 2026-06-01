# 生命周期管理

管理脚本系统的整体生命周期，包括初始化、tick驱动、看门狗和日志。

- `ScriptManager` — 顶层脚本管理器，协调引擎、插件管理器、事件总线、看门狗等组件
- `ScriptTickListener` — 每tick驱动脚本系统（beginTick→tickPlugins→executePendingJobs→flushEvents→endTick）
- `ScriptWatchdog` — 脚本超时看门狗，检测执行时间超限和内存越限
- `ScriptLogger` — 脚本日志桥接，将脚本输出转发到spdlog
