# Spawn 模块

本模块定义生物群系的实体生成配置，参考 MC 1.16.5 的 `MobSpawnInfo` 系统。

## 目录结构

```
src/common/world/spawn/
├── MobSpawnInfo.hpp    # 生成信息类型定义
└── MobSpawnInfo.cpp    # 各生物群系的工厂方法实现
```

## 文件详解

### MobSpawnInfo.hpp

定义生物生成配置的核心类型，包含以下主要结构：

#### SpawnCosts 结构体

生成成本，用于限制高密度区域内的实体数量。

| 字段 | 类型 | 说明 |
|------|------|------|
| `energyBudget` | `f64` | 能量预算，区域内允许的最大生成成本总和 |
| `charge` | `f64` | 单个实体的充电成本 |

参考 MC 1.16.5 的 `SpawnCosts`，用于控制特定实体（如末影人）在特定区域的密度。

#### SpawnEntry 结构体

单个实体类型的生成配置条目。

| 字段 | 类型 | 说明 |
|------|------|------|
| `entityTypeId` | `std::string` | 实体类型ID（如 "minecraft:pig"） |
| `weight` | `i32` | 生成权重，越高越容易被选中 |
| `minCount` | `i32` | 最小生成数量（默认 1） |
| `maxCount` | `i32` | 最大生成数量（默认 4） |
| `costs` | `SpawnCosts` | 可选的生成成本 |

#### SpawnCategory 结构体

特定实体分类（怪物、动物等）的生成配置容器。

| 字段 | 类型 | 说明 |
|------|------|------|
| `entries` | `std::vector<SpawnEntry>` | 该分类的生成条目列表 |
| `maxInstances` | `i32` | 该分类的最大实例数 |
| `enabled` | `bool` | 是否启用 |

提供 `getTotalWeight()` 方法计算所有条目的总权重，用于加权随机选择。

#### MobSpawnInfo 类

完整的生物群系实体生成信息，包含所有分类的生成配置。

**成员分类：**

| 分类 | 说明 | 默认最大实例数 |
|------|------|---------------|
| Monster | 怪物（僵尸、骷髅等） | 70 |
| Creature | 动物（猪、牛、羊等） | 10 |
| Ambient | 环境生物（蝙蝠） | 15 |
| WaterCreature | 水生生物（鱿鱼、海豚） | 5 |
| WaterAmbient | 水生环境生物（鱼） | 20 |
| Misc | 其他 | 无限制 |

**核心方法：**

```cpp
// 添加生成条目
void addMonsterSpawn(const SpawnEntry& entry);
void addCreatureSpawn(const SpawnEntry& entry);
void addAmbientSpawn(const SpawnEntry& entry);
void addWaterCreatureSpawn(const SpawnEntry& entry);

// 根据分类获取生成列表
const std::vector<SpawnEntry>& getSpawns(EntityClassification classification) const;

// 获取生成成本
const SpawnCosts* getSpawnCost(const std::string& entityTypeId) const;

// 生物群系特性
f32 getCreatureSpawnProbability() const;  // 动物生成概率
bool isPlayerSpawnFriendly() const;        // 是否适合玩家生成
```

**Builder 模式：**

```cpp
MobSpawnInfo info = MobSpawnInfo::Builder()
    .addSpawn(EntityClassification::Creature, SpawnEntry("minecraft:pig", 10, 4, 4))
    .setCreatureSpawnProbability(0.1f)
    .setPlayerSpawnFriendly()
    .build();
```

### MobSpawnInfo.cpp

提供各生物群系的工厂方法，参考 MC 1.16.5 的生物群系生成配置。

**支持的生物群系工厂方法：**

| 方法 | 生物群系 | 特色生物 |
|------|----------|----------|
| `createPlains()` | 平原 | 马、驴 |
| `createForest()` | 森林 | 狼 |
| `createDesert()` | 沙漠 | 尸壳、兔子 |
| `createOcean()` | 海洋 | 溺尸、鳕鱼、鲑鱼、海豚 |
| `createTaiga()` | 针叶林 | 狼、狐狸 |
| `createJungle()` | 丛林 | 鹦鹉、熊猫、豹猫 |
| `createSavanna()` | 热带草原 | 马、驴、羊驼 |
| `createSwamp()` | 沼泽 | 女巫（高概率）、史莱姆 |
| `createMountains()` | 山地 | 羊驼 |
| `createSnowy()` | 雪地 | 流浪者、北极熊、兔子 |
| `createEmpty()` | 虚空 | 无生物 |

## 文件关系

```
┌─────────────────────────────────────────────────────────────────┐
│                    上游依赖                                      │
├─────────────────────────────────────────────────────────────────┤
│  EntityClassification.hpp  ←─ 实体分类枚举                       │
│  Types.hpp                  ←─ 基础类型定义                      │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    spawn 模块                                    │
├─────────────────────────────────────────────────────────────────┤
│  MobSpawnInfo.hpp                                               │
│  ├── SpawnCosts        生成成本                                  │
│  ├── SpawnEntry        生成条目                                  │
│  ├── SpawnCategory     生成分类                                  │
│  └── MobSpawnInfo      完整生成信息                              │
│                                                                 │
│  MobSpawnInfo.cpp                                               │
│  └── 工厂方法：createPlains(), createForest(), ...              │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    下游使用                                      │
├─────────────────────────────────────────────────────────────────┤
│  Biome.hpp                     生物群系定义                      │
│  ├── Biome::spawnInfo()        获取生成信息                      │
│  └── BiomeRegistry.cpp         注册时设置生成信息                │
│                                                                 │
│  WorldGenSpawner.hpp           区块生成时放置动物                │
│  ├── 使用 MobSpawnInfo 获取动物生成列表                         │
│  └── 仅使用 Creature 分类                                       │
│                                                                 │
│  NaturalSpawner.hpp (server)   运行时自然生成                    │
│  ├── 使用 MobSpawnInfo 获取所有分类                             │
│  └── 处理怪物、动物、环境生物等                                  │
└─────────────────────────────────────────────────────────────────┘
```

## 模块职责

### 整体职责

1. **定义生成配置数据结构**：提供 `SpawnEntry`、`SpawnCategory`、`MobSpawnInfo` 等类型来描述生物生成规则。

2. **提供生物群系工厂方法**：为每种生物群系预定义实体生成配置，遵循 MC 1.16.5 的生成规则。

3. **支持加权随机选择**：通过权重系统实现实体类型的概率选择。

### 输入和输出

**输入：**
- 实体分类（`EntityClassification`）
- 实体类型ID（字符串标识符）
- 生成权重、数量范围
- 生成成本配置

**输出：**
- 特定分类的生成条目列表
- 加权随机选择结果
- 生成成本查询

### 依赖项

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（i32, f32, f64, std::string 等） |
| `common/entity/EntityClassification.hpp` | 实体分类枚举 |

### 使用方法

**1. 获取预定义的生物群系生成信息：**

```cpp
#include "common/world/spawn/MobSpawnInfo.hpp"

// 获取平原生物群系的生成配置
world::spawn::MobSpawnInfo plainsInfo = world::spawn::MobSpawnInfo::createPlains();

// 获取动物生成列表
const auto& creatures = plainsInfo.getCreatureSpawns();
for (const auto& entry : creatures) {
    // entry.entityTypeId = "minecraft:pig", etc.
    // entry.weight = 10
    // entry.minCount = 4, entry.maxCount = 4
}

// 获取怪物生成列表
const auto& monsters = plainsInfo.getMonsterSpawns();
```

**2. 使用 Builder 模式自定义生成信息：**

```cpp
using namespace mc::world::spawn;

MobSpawnInfo customInfo = MobSpawnInfo::Builder()
    .addSpawn(EntityClassification::Monster, SpawnEntry("minecraft:zombie", 100, 4, 4))
    .addSpawn(EntityClassification::Monster, SpawnEntry("minecraft:skeleton", 100, 4, 4))
    .addSpawn(EntityClassification::Creature, SpawnEntry("minecraft:pig", 12, 4, 4))
    .setCreatureSpawnProbability(0.15f)
    .setPlayerSpawnFriendly()
    .setSpawnCost("minecraft:enderman", SpawnCosts(1.0, 0.5))
    .build();
```

**3. 与生物群系配合使用：**

```cpp
#include "common/world/biome/Biome.hpp"

// 从生物群系获取生成信息
const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
const MobSpawnInfo& spawnInfo = biome.spawnInfo();

// 获取特定分类的生成列表
const auto& entries = spawnInfo.getSpawns(EntityClassification::Creature);
```

**4. 加权随机选择：**

```cpp
#include "common/util/math/random/Random.hpp"

const auto& entries = spawnInfo.getMonsterSpawns();
i32 totalWeight = 0;
for (const auto& entry : entries) {
    totalWeight += entry.weight;
}

math::Random rng(seed);
i32 roll = rng.nextInt(totalWeight);
i32 cumulative = 0;
const SpawnEntry* selected = nullptr;

for (const auto& entry : entries) {
    cumulative += entry.weight;
    if (roll < cumulative) {
        selected = &entry;
        break;
    }
}
// selected 为选中的生成条目
```

## 容易踩的坑

### 1. 权重理解错误

**问题**：将 `weight` 误解为概率百分比。

**正确理解**：权重是相对值，实际概率 = `weight / totalWeight`。例如平原僵尸权重 95，骷髅权重 100，则僵尸概率 = 95/195 ≈ 48.7%。

```cpp
// 错误理解
if (entry.weight > 50) { /* 50% 概率 */ }  // 错误！

// 正确理解
i32 totalWeight = category.getTotalWeight();
f32 probability = static_cast<f32>(entry.weight) / totalWeight;
```

### 2. 实例数量限制混淆

**问题**：混淆 `SpawnEntry::minCount/maxCount` 与 `SpawnCategory::maxInstances`。

**区别**：
- `SpawnEntry::minCount/maxCount`：单次生成时的数量范围
- `SpawnCategory::maxInstances`：该分类在区域内的总实体数量上限

```cpp
// 例如：僵尸单次生成 4 个，但区域内最多 70 个怪物
SpawnEntry zombie("minecraft:zombie", 95, 4, 4);  // 单次 4 个
info.setMaxMonsterInstances(70);                   // 总共最多 70 个怪物
```

### 3. 工厂方法返回临时对象

**问题**：工厂方法返回值，不是引用。

```cpp
// 错误：返回临时对象的引用
const MobSpawnInfo& info = MobSpawnInfo::createPlains();  // 危险！

// 正确：保存值
MobSpawnInfo info = MobSpawnInfo::createPlains();  // OK
```

### 4. 空指针检查

**问题**：`getSpawnCost()` 可能返回 `nullptr`。

```cpp
// 错误：未检查空指针
SpawnCosts costs = *spawnInfo.getSpawnCost(entityId);  // 可能崩溃

// 正确：检查返回值
const SpawnCosts* costs = spawnInfo.getSpawnCost(entityId);
if (costs && costs->isValid()) {
    // 使用成本限制
}
```

### 5. 实体类型ID格式

**问题**：实体类型ID必须使用完整的命名空间格式。

```cpp
// 错误：缺少命名空间
SpawnEntry entry("zombie", 95, 4, 4);  // 错误！

// 正确：包含命名空间
SpawnEntry entry("minecraft:zombie", 95, 4, 4);  // 正确
```

### 6. Builder 模式使用

**问题**：忘记调用 `build()`。

```cpp
// 错误：未调用 build()
MobSpawnInfo info = MobSpawnInfo::Builder()
    .addSpawn(classification, entry);  // 类型不匹配！

// 正确：调用 build()
MobSpawnInfo info = MobSpawnInfo::Builder()
    .addSpawn(classification, entry)
    .build();
```

## 涉及的测试用例

测试文件位于 `tests/common/world/gen/WorldGenSpawnerTest.cpp`：

```cpp
// MobSpawnInfo 基本功能测试
TEST_F(WorldGenSpawnerTest, PlainsSpawnInfo);
TEST_F(WorldGenSpawnerTest, ForestSpawnInfo);
TEST_F(WorldGenSpawnerTest, DesertSpawnInfo);
TEST_F(WorldGenSpawnerTest, OceanSpawnInfo);

// SpawnEntry 测试
TEST_F(WorldGenSpawnerTest, SpawnEntryDefaults);
TEST_F(WorldGenSpawnerTest, SpawnEntryCustomValues);
TEST_F(WorldGenSpawnerTest, SpawnEntryWithCosts);

// Biome 集成测试
TEST_F(WorldGenSpawnerTest, BiomeSpawnInfo);
```

运行测试：

```powershell
./build/bin/Release/mc_tests.exe --gtest_filter="WorldGenSpawnerTest.*"
```

## 参考

- MC 1.16.5 `MobSpawnInfo` 类
- MC 1.16.5 `Biome` 类的生成配置
- MC 1.16.5 各生物群系的实体生成权重和数量
