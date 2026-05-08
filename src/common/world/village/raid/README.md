# 袭击系统 (Raid System)

本目录实现村庄袭击事件系统，包括掠夺者生成、波次管理和Boss栏显示。

## 目录结构

```
raid/
├── RaiderType.hpp/cpp    # 掠夺者类型枚举和工具
├── Raid.hpp/cpp          # 袭击事件
├── RaidManager.hpp/cpp   # 袭击管理器
└── README.md             # 本文档
```

## 核心类

### RaiderType - 掠夺者类型

```cpp
enum class RaiderType : u8 {
    Pillager,   // 掠夺者（持弩）
    Vindicator, // 灾厄村民（持铁斧）
    Evoker,     // 唤魔者（召唤恼鬼和尖牙）
    Ravager,    // 劫掠兽（巨型野兽）
    Witch       // 女巫（在袭击中会参与）
};
```

### Raid - 袭击事件

单个村庄的袭击事件管理：

```cpp
// 创建袭击
Raid raid(id, village);
raid.setBadOmenLevel(2); // 不祥之兆等级

// 更新袭击
raid.tick(world);

// 检查状态
if (raid.status() == RaidStatus::Victory) {
    // 玩家胜利
}

// 英雄追踪
raid.addHero(playerUuid, entityId);
bool isHero = raid.isHero(playerUuid);
```

### RaidManager - 袭击管理器

世界级别的袭击管理：

```cpp
// 创建管理器
RaidManager manager(world, villageManager);

// 设置回调
RaidCallbacks callbacks;
callbacks.onRaidStarted = [](const Raid& raid, BlockPos center) {
    // 播放号角声、发送消息
};
callbacks.onRaidVictory = [](const Raid& raid, const std::vector<Uuid>& heroes, i32 level) {
    // 给予英雄效果
};
callbacks.onRaidLoss = [](const Raid& raid) {
    // 处理村庄损失
};
manager.setCallbacks(std::move(callbacks));

// 触发袭击
Raid* raid = manager.tryStartRaid(pos, badOmenLevel);

// 查询袭击
Raid* raid = manager.getRaidAt(pos);
bool hasRaid = manager.hasRaidAt(pos);

// 每tick更新
manager.tick();
```

## 袭击流程

```
玩家携带不祥之兆进入村庄
        ↓
触发袭击（RaidManager.tryStartRaid）
        ↓
生成第1波掠夺者（Raid.startNextWave）
        ↓
玩家击败掠夺者
        ↓
等待60秒后生成下一波
        ↓
重复直到所有波次完成
        ↓
玩家胜利 → 获得英雄效果
或
掠夺者胜利 → 村庄被摧毁
```

## 波次配置

| 难度 | 波次数 |
|------|--------|
| 简单 | 3 |
| 普通 | 5 |
| 困难 | 7 |

不祥之兆等级每增加1级，额外增加1波。

## 掠夺者生成

每波生成的掠夺者类型取决于波次：

| 波次 | 可能出现的类型 |
|------|----------------|
| 1-2 | 掠夺者 |
| 3-4 | 掠夺者、灾厄村民、女巫 |
| 5-6 | + 唤魔者 |
| 7+ | + 劫掠兽 |

## 英雄系统

袭击期间击杀掠夺者的玩家会被记录为英雄：

```cpp
// 在 Raid 类中
void addHero(Uuid playerUuid, EntityId entityId);  // 添加英雄
bool isHero(Uuid playerUuid) const;                // 检查是否为英雄
void addContribution(Uuid playerUuid, i32 amount); // 增加贡献值
const std::unordered_set<Uuid, UuidHash>& heroes() const; // 获取所有英雄
```

袭击胜利时，所有英雄玩家将获得"村庄英雄"效果。

## 回调机制

`RaidManager` 提供回调机制来通知外部系统：

### RaidCallbacks 结构体

```cpp
struct RaidCallbacks {
    // 袭击开始时调用
    std::function<void(const Raid& raid, BlockPos center)> onRaidStarted;
    
    // 袭击胜利时调用
    std::function<void(const Raid& raid, const std::vector<Uuid>& heroes, i32 badOmenLevel)> onRaidVictory;
    
    // 袭击失败时调用
    std::function<void(const Raid& raid)> onRaidLoss;
    
    // 波次开始时调用
    std::function<void(const Raid& raid, i32 wave, BlockPos spawnPos)> onWaveStarted;
};
```

### 使用示例

```cpp
// 在 ServerWorld 初始化后设置回调
auto raidManager = world->raidManager();
RaidCallbacks callbacks;

callbacks.onRaidStarted = [this](const Raid& raid, BlockPos center) {
    // 播放号角声
    broadcastSound(SoundEvents::EVENT_RAID_HORN, SoundCategory::Neutral,
                   Vector3(center.x + 0.5f, center.y, center.z + 0.5f),
                   64.0f, 1.0f);
    // 发送聊天消息
    broadcastChatMessage("袭击开始了！");
};

callbacks.onRaidVictory = [this](const Raid& raid, const std::vector<Uuid>& heroes, i32 level) {
    // 给予英雄效果
    for (const auto& uuid : heroes) {
        ServerPlayer* player = getPlayerByUuid(uuid);
        if (player) {
            player->addEffect(EffectInstance::heroOfTheVillage(level));
        }
    }
};

raidManager->setCallbacks(std::move(callbacks));
```

## 依赖关系

```
RaidManager
    ├── IWorld (世界引用)
    ├── VillageManager (村庄关联)
    └── Raid (袭击事件)
         ├── RaiderType (掠夺者类型)
         └── EntityRegistry (实体生成)
```

## 与其他系统集成

### 村庄系统
- 袭击需要村庄作为目标
- 袭击失败可能导致村民死亡
- 袭击胜利增加村庄声誉

### 效果系统
- 不祥之兆效果触发袭击
- 英雄效果作为奖励

### 实体系统
- 掠夺者实体生成
- 劫掠兽骑乘系统

### 声音系统
- 号角声播放（`minecraft:event.raid_horn`）

## 已实现功能

- [x] 袭击创建和管理
- [x] 波次生成逻辑
- [x] 掠夺者类型选择
- [x] 英雄追踪系统
- [x] 回调机制（onRaidStarted, onRaidVictory, onRaidLoss）
- [x] 袭击开始时设置村庄状态

## 待实现功能

- [ ] Boss栏显示
- [ ] 号角声播放（需要集成到 ServerWorld）
- [ ] 英雄效果赋予（需要集成到 ServerWorld）
- [ ] 掠夺者庆祝/失败行为
- [ ] 袭击序列化/存档

## 参考

- MC 1.16.5 `net.minecraft.world.raid.Raid`
- MC 1.16.5 `net.minecraft.world.raid.RaidManager`
