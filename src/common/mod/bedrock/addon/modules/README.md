# @minecraft/server模块绑定

实现基岩版@minecraft/server API的C++→JS绑定。

- `MinecraftModuleFactory` — @minecraft/server模块工厂，注册所有游戏API绑定

## 已绑定的API

- `world` — 世界对象（getDimension, getAllPlayers, sendMessage等）
- `system` — 系统对象（run, runInterval, runTimeout, clearRun）
- `Dimension` — 维度操作
- `Entity` / `Player` — 实体和玩家操作
- `Block` / `BlockPermutation` / `BlockType` — 方块操作
- `ItemStack` / `ItemType` — 物品操作
- `Container` — 容器/背包
- `Scoreboard` — 记分板
- `Commands` — 自定义命令注册
- `DynamicProperties` — 动态属性
- 事件绑定（BeforeEvents/AfterEvents）
