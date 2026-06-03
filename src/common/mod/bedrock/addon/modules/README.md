# @minecraft/server模块绑定

实现基岩版@minecraft/server API的C++→JS绑定。

## 目录结构

```
modules/
├── MinecraftModuleFactory.hpp/.cpp   # 模块工厂，注册所有@minecraft/server绑定
├── ScriptCustomComponentBinding.hpp/.cpp  # 自定义组件JS绑定
├── events/                           # 事件类型定义（待实现）
│   └── events/                       # 空目录，待填充
└── types/                            # 脚本类型包装类（待实现）
```

## 已实现的API

### 自定义组件（完整实现）
- `blockComponentRegistry.registerCustomComponent(typeId, component)` — 注册方块自定义组件
- `itemComponentRegistry.registerCustomComponent(typeId, component)` — 注册物品自定义组件
- 方块事件回调：onStepOn, onStepOff, onPlace, onBreak, onPlayerBreak, onPlayerInteract, beforeOnPlayerPlace, onEntityFallOn, onRandomTick, onTick, onEntity, onRedstoneUpdate, onBlockStateChange
- 物品事件回调：onUse, onUseOn, onHitEntity, onMineBlock, beforeDurabilityDamage, onCompleteUse, onConsume

### 类注册框架（结构已定义，实现为stub）

| 类 | 已注册方法/属性 | 实现状态 |
|---|---|---|
| System | run, runInterval, runTimeout, clearRun, currentTick | Stub — 返回0/undefined，需集成ScriptTickListener |
| World | getDimension, getAllPlayers, sendMessage | Stub — 返回undefined/空数组/无操作 |
| Dimension | id | Stub — 返回硬编码"minecraft:overworld" |
| Entity | id, typeId, getDimension, getLocation | Stub — 全部返回undefined |
| Player | name（继承Entity） | Stub — 返回undefined |
| Block | （空类注册） | 未实现 |
| ItemStack | typeId, amount | Stub — 返回undefined |

### 已导出的常量
- GameMode枚举：Survival=0, Creative=1, Adventure=2, Spectator=3
- MinecraftDimensionTypes：Overworld, Nether, TheEnd

## 待实现的API

### System调度器
- [ ] system.run(callback) — 延迟到下一tick执行
- [ ] system.runInterval(callback, tickInterval) — 周期性执行
- [ ] system.runTimeout(callback, tickDelay) — 延迟执行
- [ ] system.clearRun(runId) — 取消调度
- [ ] system.currentTick — 当前tick

### World API
- [ ] world.afterEvents / world.beforeEvents — 事件订阅
- [ ] world.getDimension(dimensionId)
- [ ] world.getAllPlayers()
- [ ] world.sendMessage(message)
- [ ] world.scoreboard
- [ ] world.gameRules
- [ ] world.getTimeOfDay() / world.setTimeOfDay()
- [ ] world.getDynamicProperty() / world.setDynamicProperty()
- [ ] world.structureManager

### Dimension API
- [ ] dimension.id
- [ ] dimension.getBlock(location)
- [ ] dimension.getEntities(options)
- [ ] dimension.getPlayers(options)
- [ ] dimension.spawnEntity(identifier, location)

### Entity/Player API
- [ ] Entity属性：id, typeId, location, velocity, rotation, dimension
- [ ] Entity方法：teleport(), kill(), remove(), runCommand(), getComponent()
- [ ] Player属性：name, gameMode, isSneaking, selectedSlot
- [ ] Player方法：sendMessage(), getInventory()

### Block API
- [ ] Block属性：location, permutation, type, isWaterlogged
- [ ] Block方法：getComponent(), setPermutation()

### ItemStack API
- [ ] ItemStack属性：typeId, amount, nameTag, lore, durability
- [ ] ItemStack方法：getComponent(), clone(), isStackable()

## 内部依赖

- `../binding/` — 模块绑定框架（NativeModuleBuilder, ClassRegistrar, TypeConverter）
- `../component/` — 自定义组件注册表
- `../event/` — 脚本事件总线
- `../engine/` — QuickJS上下文和值转换
- `../lifecycle/` — ScriptTickListener（调度器需要）

## 外部依赖

- `src/common/world/` — 游戏世界对象（Dimension, Block, Entity等）
- `src/server/` — 服务器对象（ServerWorld, ServerPlayer等）

## 容易踩的坑

1. **Stub陷阱**：几乎所有API都返回undefined/0/空数组，不是真实实现
2. **types/和events/目录为空**：ScriptBlock/ScriptEntity等包装类和事件类型尚未实现
3. **MinecraftModuleFactory.cpp中的TODO**：每个stub方法都有TODO注释标记需要集成的地方
4. **对象生命周期**：JS对象持有的C++游戏对象指针可能在tick间失效，需要谨慎处理
