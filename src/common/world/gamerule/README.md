# GameRule 模块

游戏规则系统，用于控制 Minecraft 世界的行为参数。

## 目录结构

```
gamerule/
├── GameRule.hpp/cpp     # 游戏规则基础类型（RuleKey, RuleType, RuleValue）
├── GameRules.hpp/cpp    # 游戏规则容器类和所有规则定义
└── README.md            # 本文件
```

## 核心类型

### GameRuleKey\<T\>

游戏规则键，唯一标识一个规则。

```cpp
// 获取规则名称
const std::string& getName() const;

// 获取规则分类
GameRuleCategory getCategory() const;

// 获取本地化键
std::string getTranslationKey() const;  // 返回 "gamerule.mobGriefing" 等
```

### GameRuleType\<T\>

规则类型定义，包含默认值和变更监听器。

```cpp
// 创建规则类型
BooleanGameRuleType mobGriefingType(true);  // 默认值 true
IntegerGameRuleType tickSpeedType(3);       // 默认值 3

// 创建带监听器的规则类型
BooleanGameRuleType type(true, [](MinecraftServer* server, bool newValue) {
    // 规则变更时触发
});

// 创建规则值实例
GameRuleValue<bool> value = type.createValue();
```

### GameRuleValue\<T\>

规则的运行时值。

```cpp
// 获取/设置值
bool value = ruleValue.get();
ruleValue.set(false, server);  // server 可为 nullptr

// 重置为默认值
ruleValue.reset(server);

// 序列化
std::string str = ruleValue.toString();   // "true" / "3"
ruleValue.fromString("false");            // 解析字符串
```

### GameRules

游戏规则容器类，管理所有规则。

```cpp
// 创建实例（使用默认值）
GameRules rules;

// 获取规则值
bool mobGriefing = rules.getBoolean(GameRuleKeys::MOB_GRIEFING);
i32 tickSpeed = rules.getInt(GameRuleKeys::RANDOM_TICK_SPEED);

// 设置规则值
rules.setBoolean(GameRuleKeys::MOB_GRIEFING, false, server);
rules.setInt(GameRuleKeys::RANDOM_TICK_SPEED, 6, server);

// 从字符串设置（用于命令）
rules.setFromString("mobGriefing", "false", server);

// 序列化到 NBT
auto nbt = rules.write();

// 从 NBT 加载
rules.read(*nbt);

// 重置所有规则
rules.resetAll();

// 重置指定规则
rules.reset("mobGriefing", server);
```

## 预定义规则

### 玩家相关 (Player)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| keepInventory | Boolean | false | 玩家死亡后保留物品栏 |
| naturalRegeneration | Boolean | true | 玩家自然恢复生命值 |
| spawnRadius | Integer | 10 | 玩家重生半径 |
| spectatorsGenerateChunks | Boolean | true | 旁观者生成区块 |
| disableElytraMovementCheck | Boolean | false | 禁用鞘翅移动检查 |
| doImmediateRespawn | Boolean | false | 立即重生 |
| drowningDamage | Boolean | true | 溺水受伤 |
| fallDamage | Boolean | true | 摔落受伤 |
| fireDamage | Boolean | true | 火焰受伤 |
| doLimitedCrafting | Boolean | false | 限制合成 |

### 生物相关 (Mobs)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| mobGriefing | Boolean | true | 生物破坏方块 |
| maxEntityCramming | Integer | 24 | 实体挤压上限 |
| disableRaids | Boolean | false | 禁用袭击 |
| forgiveDeadPlayers | Boolean | true | 中立生物原谅死亡玩家 |
| universalAnger | Boolean | false | 通用愤怒机制 |

### 生成相关 (Spawning)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| doMobSpawning | Boolean | true | 生物生成 |
| doInsomnia | Boolean | true | 幻翼生成 |
| doPatrolSpawning | Boolean | true | 巡逻队生成 |
| doTraderSpawning | Boolean | true | 流浪商人生成 |

### 掉落相关 (Drops)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| doMobLoot | Boolean | true | 生物掉落物品 |
| doTileDrops | Boolean | true | 方块掉落物品 |
| doEntityDrops | Boolean | true | 实体掉落物品 |

### 更新相关 (Updates)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| doFireTick | Boolean | true | 火焰蔓延 |
| doDaylightCycle | Boolean | true | 日照循环 |
| randomTickSpeed | Integer | 3 | 随机刻速度 |
| doWeatherCycle | Boolean | true | 天气循环 |

### 聊天相关 (Chat)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| commandBlockOutput | Boolean | true | 命令方块输出 |
| logAdminCommands | Boolean | true | 记录管理员命令 |
| showDeathMessages | Boolean | true | 显示死亡消息 |
| sendCommandFeedback | Boolean | true | 发送命令反馈 |
| announceAdvancements | Boolean | true | 公布成就 |

### 杂项 (Misc)

| 规则名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| reducedDebugInfo | Boolean | false | 减少调试信息 |
| maxCommandChainLength | Integer | 65536 | 最大命令链长度 |

## 使用示例

### 在实体行为中检查规则

```cpp
// EatGrassGoal.cpp
void EatGrassGoal::eatGrass() {
    if (!m_world || !m_mob) {
        return;
    }

    // 检查 mobGriefing 游戏规则
    if (!m_world->getGameRules().getBoolean(GameRuleKeys::MOB_GRIEFING)) {
        // 只调用 eatGrassBonus，不破坏方块
        if (m_onEatGrass) {
            m_onEatGrass();
        }
        return;
    }

    // 破坏草方块...
}
```

### 在玩家 tick 中检查规则

```cpp
// Player.cpp
void Player::tick() {
    // ...
    if (m_gameMode == GameMode::Survival || m_gameMode == GameMode::Adventure) {
        bool naturalRegeneration = m_world->getGameRules().getBoolean(
            GameRuleKeys::NATURAL_REGENERATION
        );
        m_foodStats.tick(*this, difficulty(), naturalRegeneration);
    }
    // ...
}
```

### 在命令中设置规则

```cpp
// GameRuleCommand.cpp
i32 GameRuleCommand::execute(CommandContext<ServerCommandSource>& context) {
    std::string ruleName = context.getArgument<std::string>("rule");
    std::string value = context.getArgument<std::string>("value");

    auto& gameRules = context.getSource().getWorld().getGameRules();

    if (gameRules.setFromString(ruleName, value, context.getSource().getServer())) {
        context.getSource().sendFeedback("Game rule " + ruleName + " has been updated to " + value);
        return 1;
    } else {
        context.getSource().sendError("Unknown game rule: " + ruleName);
        return 0;
    }
}
```

## 与原版 MC 1.16.5 的对应关系

| 原版规则 | 本项目规则键 |
|---------|-------------|
| GameRules.MOB_GRIEFING | GameRuleKeys::MOB_GRIEFING |
| GameRules.NATURAL_REGENERATION | GameRuleKeys::NATURAL_REGENERATION |
| GameRules.DO_DAYLIGHT_CYCLE | GameRuleKeys::DO_DAYLIGHT_CYCLE |
| GameRules.RANDOM_TICK_SPEED | GameRuleKeys::RANDOM_TICK_SPEED |
| ... | ... |

## 参考

- MC 1.16.5: `net.minecraft.world.GameRules`
- MC 1.16.5: `net.minecraft.world.GameRules.BooleanValue`
- MC 1.16.5: `net.minecraft.world.GameRules.IntegerValue`
