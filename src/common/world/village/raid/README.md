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
```

### RaidManager - 袭击管理器

世界级别的袭击管理：

```cpp
// 创建管理器
RaidManager manager(world);

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

## 依赖关系

```
RaidManager
    ├── ServerWorld (世界引用)
    ├── Village (村庄关联)
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

## TODO

- [ ] 实现掠夺者实体生成
- [ ] 集成村庄管理器
- [ ] 集成效果系统（不祥之兆、英雄）
- [ ] 实现Boss栏显示
- [ ] 实现袭击音效
- [ ] 实现掠夺者庆祝/失败行为

## 参考

- MC 1.16.5 `net.minecraft.world.raid.Raid`
- MC 1.16.5 `net.minecraft.world.raid.RaidManager`
