# 服务端脚本管理器

服务端侧的脚本系统入口，继承通用ScriptManager并集成MinecraftServer。

- `ServerScriptManager` — 包装ScriptManager，在MinecraftServer中初始化和驱动

## 集成点

- `MinecraftServer::initializeCoreManagers()` — 创建ServerScriptManager实例
- `MinecraftServer::initializeWorld()` — 初始化脚本引擎和加载行为包
- `MinecraftServer::tick()` — 驱动脚本系统tick
- `MinecraftServer::shutdownManagers()` — 关闭脚本系统
