# 玩家实体模块

玩家实体模块负责封装玩家的移动、能力、背包、经验、脚步声和游泳声等核心状态，是客户端主循环和服务端玩家逻辑之间的共同抽象。

## 目录结构

```text
src/common/entity/entities/player/
├── Player.hpp         # 玩家实体声明，包含状态、移动和网络同步接口
├── Player.cpp         # 玩家实体实现，包含物理、脚步声、游泳声和序列化
├── GameModeUtils.hpp  # 游戏模式能力映射工具
├── GameModeUtils.cpp  # 游戏模式能力映射实现
└── README.md          # 本文档
```

## 文件介绍

### Player.hpp

声明 `Player` 类以及玩家专用的状态字段和访问接口，包括：

- 游戏模式和能力标志
- 生命值、饥饿、经验和吸收值
- 物理移动入口、跳跃、游泳和步距统计
- 脚步声和游泳声的触发标志
- 网络同步所需的玩家位置封装

### Player.cpp

实现玩家的核心行为：

- 缓存移动输入，固定在 20TPS 物理 tick 中消费，避免渲染帧率改变行走速度
- 执行原版风格地面移动公式：地面输入速度使用 `getGroundMoveFactor()`，移动后水平摩擦使用脚下方块 `slipperiness * 0.91`
- 执行物理更新、碰撞和跳跃
- 在蹲下、游泳和睡眠姿态切换时，先检查目标碰撞箱能否容纳当前空间
- 统计移动距离并生成步脚声/游泳声触发信号
- 统一处理受伤和死亡声音，并通过 `Entity::playSound()` 走世界级声音出口
- 序列化和反序列化玩家状态

### GameModeUtils.hpp / GameModeUtils.cpp

把游戏模式映射成玩家能力配置，避免把创造、旁观、生存等模式逻辑散落在各处。

## 模块关系

- `Player` 继承自 `Entity`，复用通用的位置、旋转、碰撞和数据管理能力。
- `Player` 在退出蹲伏、游泳和睡眠姿态时，会通过 `IWorld` 的碰撞查询判断当前空间是否允许切回站立。
- `ClientApplication` 使用 `Player` 的 `distanceWalkedModified` 等价累计值和 `cameraYaw/prevCameraYaw` 来驱动原版 `GameRenderer.applyBobbing()` 风格的视图矩阵变换，并读取步脚声/游泳声标志来播放本地音效。
- `NetworkClient` 和玩家序列化逻辑负责把服务器传来的传送、位置和状态同步到本地玩家。
- 服务端玩家管理由 `server/world/player/ServerPlayerEntityManager` 负责。
- 客户端本地玩家身份由 `client/world/player/LocalPlayerIdentity` 管理。
- `GameModeUtils` 负责把游戏模式转换为玩家能力，避免重复实现。

## 整体职责

这个模块的职责是把”玩家”从通用实体里单独抽出来，统一管理和玩家强相关的行为：

1. 处理输入到速度的映射
2. 执行玩家专有的移动与跳跃逻辑
3. 维护脚步声、游泳声和视野晃动所需的统计量
4. 提供背包、经验、游戏模式和能力状态
5. 支持网络同步和传送复位
6. **管理物品冷却时间**

## 物品冷却系统

玩家通过 `CooldownTracker` 管理物品冷却，用于紫颂果、末影珍珠、盾牌等物品的冷却追踪。

### 核心接口

```cpp
class Player : public LivingEntity {
public:
    // 冷却追踪器访问
    CooldownTracker& cooldownTracker();
    const CooldownTracker& cooldownTracker() const;
    
    // 便捷方法
    bool hasItemCooldown(const Item* item) const;
    void setItemCooldown(const Item* item, i32 ticks);
    
    // tick() 中自动调用 m_cooldownTracker.tick()
};
```

### 使用示例

```cpp
// 检查物品是否可用
if (!player.hasItemCooldown(chorusFruit)) {
    useItem();
    player.setItemCooldown(chorusFruit, 20);  // 20 ticks = 1秒
}

// 获取冷却进度（用于渲染）
float progress = player.cooldownTracker().getCooldownProgress(item, partialTicks);
```

### 典型冷却时间

| 物品 | 冷却时间 (ticks) | 冷却时间 (秒) |
|------|------------------|---------------|
| 紫颂果 | 20 | 1.0 |
| 末影珍珠 | 20 | 1.0 |
| 盾牌（被斧击中） | 100 | 5.0 |

冷却系统参考 MC 1.16.5 `net.minecraft.util.CooldownTracker`。

## 输入 / 输出

### 输入

- 键盘和鼠标输入，驱动 `handleMovementInput()` 缓存当前 tick 输入
- 物理引擎的碰撞和重力结果，驱动 `updatePhysics()` 在固定 20TPS 中消费输入
- 服务器同步的传送、位置和旋转数据
- 游戏模式切换和能力更新
- 背包、经验和状态变化

### 输出

- 玩家位置、速度、旋转和姿态变化
- 步脚声和游泳声触发标志
- 视野晃动累计值
- `network::PlayerPosition` 同步数据
- 序列化后的玩家状态数据

## 依赖项

### 内部依赖

- `entity/core/Entity.hpp`
- `entity/player/CooldownTracker.hpp` - 物品冷却追踪
- `physics/PhysicsEngine.hpp`
- `physics/PhysicsConstants.hpp`
- `world/IWorld.hpp`
- `inventory/PlayerInventory.hpp`
- `experience/ExperienceManager.hpp`
- `movement/AutoJump.hpp`
- `world/block/BlockPos.hpp`
- `network/packet/ProtocolPackets.hpp`

### 外部依赖

- `spdlog`，用于少量日志输出
- 标准库的 `memory`、`array`、`vector`、`cmath`

## 使用方法

```cpp
using namespace mc;

auto player = std::make_unique<Player>(static_cast<EntityId>(1), "Steve");
player->setGameMode(GameMode::Survival);
player->setOnGround(true);

player->handleMovementInput(1.0f, 0.0f, false, false);
player->updatePhysics();

if (player->shouldPlayStepSound()) {
    // 播放脚步声
}
```

外部改坐标时要通过 `Player::setPosition()`，它会同步重置步距采样、脚步声状态和渲染插值历史，避免把传送或出生位置当成走路距离，也避免相机在旧位置与新位置之间拖影。

`Player::updatePhysics()` 会在每个固定物理 tick 开始冻结 `prevPosition()`，tick 内的碰撞移动不能再把 `prevPosition()` 当成步距统计来源。

`handleMovementInput()` 只缓存当前输入，不再直接修改速度；客户端必须由 `ClientApplication` 按 20TPS 调用 `updatePhysics()` 消费输入。直接在测试或逻辑里调用 `handleMovementInput()` 后，需要执行一次 `updatePhysics()` 才会看到速度和位置变化。

能力同步以 `Player::abilities()` 为运行时事实来源；`PlayerAbilitiesPacket::fromPlayer()` 不会再根据 GameMode 重新推导，避免覆盖飞行状态或自定义 walk/fly speed。

## 容易踩的坑

- 不要把 `Entity::prevPosition()` 当成脚步声采样位置，它是插值/帧历史状态，不是步距累计基准。
- 不要在外部直接修改玩家位置后继续沿用旧的步距计数，传送和出生都应该重置采样。
- `updateMoveDistance()` 可以在同一帧里被多次调用，但每次都必须只统计“上次采样之后”的增量。
- 视野晃动和脚步声共用同一套移动距离统计，统计语义错了会同时污染音效和镜头。
- 从蹲下、游泳、睡眠切回站立时，不要直接强行改成 `Standing`；应保留 `Player::setSneaking()` / `Player::setSwimming()` / `Player::setSleeping()` 的碰撞检查结果，否则会在低顶方块下错误穿模。
- 玩家受伤和死亡声音已经接入通用实体声音链路，不要再在服务器侧手写单独广播分支。
- 视野晃动的行走相位使用原版 `distanceWalkedModified = 水平实际位移 * 0.6`，不要再把未缩放的行走距离直接传给渲染层。
- `cameraYaw/prevCameraYaw` 是原版平滑晃动强度，只有站在地面、未死亡、未游泳时根据水平速度趋近，骑乘时应清零。

## 测试用例

- [tests/common/entity/PlayerMovementTest.cpp](../../../../../tests/common/entity/PlayerMovementTest.cpp)
- `UpdateMoveDistance_ResamplesCurrentPosition` 覆盖重复采样和坐标重置的回归场景
- `DamagePlaysHurtSound` / `LethalDamagePlaysDeathSound` 覆盖玩家声音事件回调
- [tests/entity/PlayerPoseCollisionTest.cpp](../../../../../tests/entity/PlayerPoseCollisionTest.cpp)
- `SetSneakingFalseKeepsCrouchWhenCeilingBlocksStanding` 覆盖低顶空间下的姿态回退
- [tests/common/entity/PlayerSwimTest.cpp](../../../../../tests/common/entity/PlayerSwimTest.cpp)
- 空气供应管理、溺水伤害、水下呼吸效果、游泳姿态尺寸、物理常量验证
- [tests/common/entity/player/PlayerSleepTest.cpp](../../../../../tests/common/entity/player/PlayerSleepTest.cpp)
- 睡眠功能测试：tryStartSleeping、startSleeping、stopSleeping、睡眠计时器、姿态切换、多态性验证
- [tests/entity/PlayerAttackTest.cpp](../../../../../tests/entity/PlayerAttackTest.cpp)
- 横扫攻击过滤测试：盔甲架标记模式、队友排除
- 荆棘附魔测试：触发概率、伤害计算
- 横扫之刃测试：伤害比例公式
- 武器耐久测试：耐久度消耗、物品损坏
- [tests/entity/EntityTeamTest.cpp](../../../../../tests/entity/EntityTeamTest.cpp)
- 队伍关系测试：isOnSameTeam、isOnScoreboardTeam、队友判断逻辑

## Mermaid 图表

```mermaid
flowchart TD
    Input[移动输入] --> Physics[Player::updatePhysics()]
    Physics --> Sample[Player::updateMoveDistance()]
    Sample --> StepFlag[脚步声/游泳声标志]
    Sample --> Bob[视野晃动累计]

    Teleport[Player::setPosition()] --> Reset[重置步距采样]
    Reset --> Sample

    StepFlag --> App[ClientApplication 播放音效]
    Bob --> Camera[Camera 视角偏移]

    style Input fill:#ffd166,stroke:#b7791f,color:#111
    style Physics fill:#8ecae6,stroke:#1d4ed8,color:#111
    style Sample fill:#90be6d,stroke:#2f6f3e,color:#111
    style StepFlag fill:#f4a261,stroke:#b45309,color:#111
    style Bob fill:#cdb4db,stroke:#6d28d9,color:#111
    style Teleport fill:#f28482,stroke:#b91c1c,color:#111
    style Reset fill:#bde0fe,stroke:#2563eb,color:#111
    style App fill:#e9c46a,stroke:#a16207,color:#111
    style Camera fill:#a8dadc,stroke:#0f766e,color:#111
```

## 近期补全

- 已新增 `ChatVisibility.hpp`，对齐 MC 1.16.5 `ChatVisibility` 的三档聊天可见性以及 ID 归一化规则。
- 已新增 `PlayerModelPart.hpp`，对齐披风、夹克、袖子、裤腿、帽子七个皮肤部件及其位掩码。
- `Player` 现已持有聊天可见性和皮肤部件位集，并提供 `isWearing()`、`setModelPartEnabled()` 等基础接口，供后续客户端设置同步、服务端设置包和渲染层复用。
- **已新增消息发送功能**（2026-05-12）：
  - `Player::sendStatusMessage(message, actionBar)` - 发送状态消息给玩家（虚方法，基类默认空操作）
  - `Player::canReceiveMessages()` - 检查玩家是否能接收消息（虚方法，基类默认返回 false）
  - `ServerPlayer::sendStatusMessage()` - 重写为通过网络发送消息到客户端
  - `ServerPlayer::canReceiveMessages()` - 重写为检查网络连接状态
  - 参考 MC 1.16.5 `PlayerEntity.sendStatusMessage(ITextComponent, boolean)`
- **已新增睡眠系统功能**（2026-05-13）：
  - `Player::tryStartSleeping(bedPos)` - 尝试开始睡眠（虚方法，基类直接调用 startSleeping）
  - `Player::startSleeping(bedPos)` - 开始睡眠，通知世界睡眠状态变化
  - `Player::stopSleeping()` - 停止睡眠，通知世界睡眠状态变化
  - `IWorld::onPlayerSleepingChanged()` - 世界睡眠状态变化通知（虚方法，默认空实现）
  - `ServerPlayer::tryStartSleeping()` - 重写为调用完整验证逻辑 `trySleep()`
  - `ServerWorld::onPlayerSleepingChanged()` - 重写为调用 `updateAllPlayersSleepingFlag()`
  - 参考 MC 1.16.5 `PlayerEntity.trySleep(BlockPos)` 和 `ServerWorld.updateAllPlayersSleepingFlag()`
- **已完善横扫攻击条件**（2026-05-13）：
  - 实现完整的横扫攻击静止检测条件
  - MC 1.16.5 条件：`distanceWalkedModified - prevDistanceWalkedModified < aiMoveSpeed()`
  - 使用 `m_moveDistanceWalked` 和 `m_prevMoveDistanceWalked` 检测玩家是否静止
  - 横扫攻击完整条件：冷却>90%、非暴击、非疾跑击退、在地面、且几乎静止
  - 参考 MC 1.16.5 `PlayerEntity.attackTargetEntityWithCurrentItem()` 行 1147-1148
- **已新增注视检测功能**（2026-05-16）：
  - `Player::getLookVector()` - 根据yaw/pitch计算视线方向向量
  - `Player::getEyePosition()` - 获取玩家眼睛位置
  - `Player::isWearingPumpkin()` - 检查玩家是否戴着南瓜头
  - `Player::isLookingAt(target)` - 检查玩家是否正在注视目标实体
  - 参考 MC 1.16.5 `Entity.getLook()` 和 `EndermanEntity.shouldAttackPlayer()`
- **已完善攻击系统功能**（2026-05-16）：
  - **横扫攻击队友过滤**：使用 `Entity::isOnSameTeam()` 排除队友
  - **横扫攻击盔甲架过滤**：使用 `ArmorStandEntity::isMarker()` 排除标记模式盔甲架
  - **荆棘附魔反伤**：使用 `EnchantmentHelper::applyThornsEnchantments()` 应用荆棘伤害
  - **武器耐久消耗**：使用 `Item::hitEntity()` 消耗武器耐久
  - 参考 MC 1.16.5 `PlayerEntity.attackTargetEntityWithCurrentItem()`
- **已新增幸运属性注册**（2026-05-17）：
  - `Player::registerAttributes()` 中注册 LUCK 属性
  - 参考 MC 1.16.5 `PlayerEntity.registerAttributes()`
  - 幸运属性影响钓鱼掉落、战利品表生成等随机事件
  - 默认值 0.0，范围 -1024.0 ~ 1024.0
  - 受幸运/霉运药水效果影响

## 挖掘系统

玩家通过 `getDigSpeed()` 和 `canHarvestBlock()` 方法参与方块挖掘计算。

### 核心接口

```cpp
class Player : public LivingEntity {
public:
    /**
     * @brief 获取玩家挖掘速度
     *
     * MC 1.16.5: PlayerEntity.getDigSpeed(BlockState, BlockPos)
     * 计算玩家对指定方块的挖掘速度，考虑以下因素：
     * 1. 工具基础挖掘速度
     * 2. 效率附魔加成（仅当工具有效时）
     * 3. 急迫效果和潮涌能量加成
     * 4. 挖掘疲劳惩罚
     * 5. 水下挖掘惩罚（无水下速掘附魔时）
     * 6. 空中挖掘惩罚（不在地面时）
     *
     * @param state 目标方块状态
     * @param pos 方块位置（用于流体检测，可选）
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDigSpeed(const BlockState& state, const BlockPos& pos = BlockPos(0, 0, 0)) const;

    /**
     * @brief 检查玩家是否能采集方块
     *
     * MC 1.16.5: PlayerEntity.canHarvestBlock(BlockState)
     * 判断玩家使用当前手持工具是否能采集指定方块。
     *
     * 采集条件：
     * 1. 方块不需要工具（requiresTool() == false）-> 可采集
     * 2. 手持物品的工具类型匹配且等级足够 -> 可采集
     * 3. 其他情况 -> 不可采集
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const;
};
```

### 挖掘速度计算公式

```
最终挖掘速度 = 基础速度 × 效率附魔加成 × 急迫效果乘数 × 挖掘疲劳乘数 × 水下惩罚 × 空中惩罚

详细公式:
digSpeed = baseSpeed;

// 效率附魔加成 (仅当基础速度 > 1.0 时)
if (baseSpeed > 1.0 && efficiencyLevel > 0) {
    digSpeed += (efficiencyLevel * efficiencyLevel + 1);
}

// 急迫/潮涌效果加成
if (hasHaste || hasConduitPower) {
    amplifier = max(hasteAmplifier, conduitAmplifier);
    digSpeed *= (1.0 + (amplifier + 1) * 0.2);
}

// 挖掘疲劳效果
if (hasMiningFatigue) {
    switch(fatigueAmplifier) {
        case 0: digSpeed *= 0.3;    break;  // 挖掘疲劳 I
        case 1: digSpeed *= 0.09;   break;  // 挖掘疲劳 II
        case 2: digSpeed *= 0.0027; break;  // 挖掘疲劳 III
        default: digSpeed *= 0.00081; break; // 挖掘疲劳 IV+
    }
}

// 水下惩罚 (没有水下亲和附魔时)
if (eyesInWater && !hasAquaAffinity) {
    digSpeed /= 5.0;
}

// 不在地面惩罚
if (!onGround) {
    digSpeed /= 5.0;
}
```

### 方块相对硬度计算

```cpp
// Block::getPlayerRelativeBlockHardness()
f32 hardness = state.hardness();
if (hardness <= 0.0f) {
    return 1.0f;  // 瞬间破坏（空气、水等）
}

if (player.isCreative()) {
    return 1.0f;  // 创造模式瞬间破坏
}

f32 digSpeed = player.getDigSpeed(state, pos);
bool canHarvest = player.canHarvestBlock(state);
f32 divisor = canHarvest ? 30.0f : 100.0f;

return digSpeed / hardness / divisor;
```

### 使用示例

```cpp
// 获取玩家对石头的挖掘速度
const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
f32 digSpeed = player.getDigSpeed(*stoneState);

// 检查玩家是否能采集钻石矿石
const BlockState* diamondOreState = &VanillaBlocks::DIAMOND_ORE->defaultState();
if (player.canHarvestBlock(*diamondOreState)) {
    // 可以采集，使用 30 作为除数
    f32 hardness = player.getDigSpeed(*diamondOreState) / hardness / 30.0f;
} else {
    // 不能采集，使用 100 作为除数，挖掘速度大幅降低
    f32 hardness = player.getDigSpeed(*diamondOreState) / hardness / 100.0f;
}
```

### 测试用例

- [tests/entity/PlayerDiggingTest.cpp](../../../../../tests/entity/PlayerDiggingTest.cpp)
- `EmptyHandHasBaseDigSpeed` - 空手基础挖掘速度
- `ToolHasCorrectDigSpeed` - 工具挖掘速度
- `WrongToolHasLowDigSpeed` - 错误工具挖掘速度
- `EfficiencyEnchantmentIncreasesDigSpeed` - 效率附魔加成
- `HasteEffectIncreasesDigSpeed` - 急迫效果加成
- `ConduitPowerIncreasesDigSpeed` - 潮涌能量加成
- `MiningFatigueReducesDigSpeed` - 挖掘疲劳惩罚
- `MiningFatigueLevels` - 挖掘疲劳各等级
- `CanHarvestBlockWithoutTool` - 不需要工具的方块采集
- `CannotHarvestStoneWithEmptyHand` - 空手不能采集石头
- `CanHarvestStoneWithPickaxe` - 镐可以采集石头
- `CannotHarvestDiamondOreWithWoodenPickaxe` - 木镐不能采集钻石矿石
- `CanHarvestDiamondOreWithIronPickaxe` - 铁镐可以采集钻石矿石
- `CombinedEffects` - 综合效果叠加测试
