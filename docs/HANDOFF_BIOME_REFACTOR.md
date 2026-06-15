# Biome 模块架构重构 — 交接文档

> 日期：2026-06-14
> 分支：main（最新提交：edb4fb60f）
> 范围：`src/common/world/biome/`

---

## 一、总体目标

对 biome 模块进行架构重构，解决以下问题：
- Biome.hpp 职责过重（已拆分完成）
- BiomeFactory 与 BiomeRegistry 耦合（**进行中**）
- 命名空间不一致（已完成）
- NetherBiomeSource 命名误导（已完成）
- 冗余代码（已完成）
- 代码规范问题（**部分完成**）

---

## 二、已完成的工作（已提交：edb4fb60f）

### 步骤1：Biome.hpp 三文件拆分 ✅
- 拆出 `BiomeIds.hpp` / `BiomeIds.cpp`（BiomeId 类型别名 + Biomes 命名空间常量）
- 拆出 `BiomeClimate.hpp` / `BiomeClimate.cpp`（BiomeClimate 结构体、温度噪声函数、applyTemperatureModifier）
- 精简 `Biome.hpp` / `Biome.cpp`（只保留 Biome 类定义）
- 删除了 Biome::Category 枚举及代理 getter
- 更新了 `Biomes.hpp` 聚合头文件

### 步骤3：IBiomeSource 重命名 + fillBiomeContainer 上提 ✅
- `BiomeSource` → `IBiomeSource`
- `fillBiomeContainer()` 公共循环逻辑上提到基类 `IBiomeSource`

### 步骤4：NetherBiomeBuilder 重命名 ✅
- `NetherBiomeSource` → `NetherBiomeBuilder`
- 文件重命名：`NetherBiomeSource.hpp/cpp` → `NetherBiomeBuilder.hpp/cpp`

### 步骤5：删除冗余代码 ✅
- 删除 `BiomeEffects.cpp`（空文件）
- 删除 `Biome::Category` 枚举
- 删除 `isOceanOrRiverBiome()`
- 删除 Biome 上的代理 getter（waterColor 等）
- 删除 `getTemperature()` / `getMusic()`

### 步骤7：命名空间迁移 ✅
- 核心类型迁移到 `mc::world::biome` 命名空间
- 旧命名空间添加兼容别名（`namespace mc { using Biome = ::mc::world::biome::Biome; }` 等）

### 步骤9：中间编译验证 + 提交 ✅
- 提交 edb4fb60f

---

## 三、进行中的工作：步骤2 — BiomeFactory 分离

### 已完成
- **BiomeFactory.hpp 已创建**（`src/common/world/biome/BiomeFactory.hpp`，558行）
  - 包含所有 BiomeFactory 函数声明
  - 命名空间 `mc::world::biome::BiomeFactory`
  - 底部兼容别名 `namespace mc { namespace BiomeFactory = ::mc::world::biome::BiomeFactory; }`

### 待完成（共7个子任务）

#### 任务21：创建 BiomeFactoryOverworld.cpp

从 `BiomeRegistry.cpp` 第 225-1452 行提取。包含：

- 匿名命名空间辅助函数 `getBlockState(Block*)`（第227-232行）
- 基础生物群系（createPlains ~ createDeepFrozenOcean）：第235-858行
- 阶段1生物群系（createWarmOcean ~ createGiantSpruceTaigaHills）：第865-1224行
- 阶段2生物群系（createSunflowerPlains ~ createSnowyTaigaHills）：第1230-1452行

**文件模板：**
```cpp
// 版权头（与项目其他文件一致）

#include "BiomeFactory.hpp"
#include "BiomeEffects.hpp"
#include "BiomeAmbientSounds.hpp"
#include "BiomeGenerationSettings.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"

namespace mc {
namespace world {
namespace biome {

namespace BiomeFactory {

namespace {
// 辅助函数：获取方块状态
const BlockState* getBlockState(Block* block)
{
    return block ? &block->defaultState() : nullptr;
}
} // namespace

// ... 所有主世界工厂函数 ...

} // namespace BiomeFactory

} // namespace biome
} // namespace world
} // namespace mc
```

**需要额外 include 的原因：**
- `BiomeEffects.hpp`：用于 BiomeEffects::Builder、GrassColorModifier、颜色常量（BADLANDS_GRASS_COLOR 等）
- `BiomeAmbientSounds.hpp`：用于 MoodSoundAmbience::defaultCaveMood()（主世界默认洞穴心境音效）
- `BiomeGenerationSettings.hpp`：用于 BiomeGenerationSettings::createPlains() 等静态工厂
- `VanillaBlocks.hpp`：用于获取 BlockState（GRASS_BLOCK、DIRT、SAND 等）
- `MobSpawnInfo.hpp`：用于 MobSpawnInfo::createPlains() 等静态工厂

**辅助函数提取计划（可选优化，非必须）：**
计划提取的辅助函数：
1. `addDefaultOverworldAmbientSounds(Biome&)` — 设置默认洞穴心境音效（约10处重复）
2. `setOverworldSurfaceBlocks(Biome&, const BlockState*, const BlockState*, const BlockState*)` — 设置地表/次地表/水下方块

**注意：** 辅助函数提取是可选优化，直接复制现有代码也能编译通过。建议先完成分离并编译通过，再考虑辅助函数提取。

---

#### 任务22：创建 BiomeFactoryNether.cpp

从 `BiomeRegistry.cpp` 第 1454-1663 行提取。包含：
- createNetherWastes（第1458行）
- createSoulSandValley（第1499行）
- createCrimsonForest（第1540行）
- createWarpedForest（第1581行）
- createBasaltDeltas（第1624行）

**文件模板同上，但需要匿名命名空间中的 `getBlockState` 辅助函数。**

**下界生物群系的特点：**
- 都使用 `BiomeClimate::Precipitation::None`
- 都设置 BiomeEffects（雾颜色各不同）
- 都设置 BiomeAmbientSounds（包含 loopSound + moodSound + additionsSound + music）
- 不调用 `setCreatureSpawnProbability`

**辅助函数提取计划（可选）：**
- `createNetherAmbientSounds(ResourceLocation loopSound)` — 创建下界通用环境音效模式

---

#### 任务23：创建 BiomeFactoryEnd.cpp

从 `BiomeRegistry.cpp` 第 1665-2077 行提取。包含：
- 末地生物群系（createTheEnd ~ createEndBarrens）：第1669-1793行
- MC 1.18+ 新生物群系（createMeadow ~ createPaleGarden）：第1799-2077行

**文件模板同上，同样需要 `getBlockState` 辅助函数。**

**末地生物群系的特点：**
- 都使用 `BiomeClimate::Precipitation::None`
- 都设置相同的 BiomeEffects（fogColor=10518688, waterColor=4159204, waterFogColor=329011）
- 都使用 `MoodSoundAmbience::defaultCaveMood()`

**辅助函数提取计划（可选）：**
- `createEndBiomeEffects()` — 创建末地通用 BiomeEffects

**MC 1.18+ 生物群系特点：**
- 大部分使用 `BiomeClimate` 设置降水类型
- 使用十六进制颜色值设置 BiomeEffects
- 大部分使用 `BiomeGenerationSettings::createDefault()`
- 只有 createLushCaves 和 createCherryGrove 有自定义生成设置
- 只有 createLushCaves 有自定义环境音效

---

#### 任务24：更新 BiomeRegistry.hpp

**需要做的修改：**
1. 删除第 88-613 行（BiomeFactory 命名空间声明 + 兼容别名中的 BiomeFactory 部分）
2. 添加 `#include "BiomeFactory.hpp"`

**修改后的 BiomeRegistry.hpp 大致结构：**
```cpp
#pragma once

#include "Biome.hpp"
#include "BiomeIds.hpp"
#include "BiomeFactory.hpp"  // 新增
#include <vector>

namespace mc {
namespace world {
namespace biome {

class BiomeRegistry {
    // ... 不变 ...
};

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
using BiomeRegistry = ::mc::world::biome::BiomeRegistry;
// 注意：删除 namespace BiomeFactory = ... 这一行
// 因为 BiomeFactory.hpp 中已有兼容别名
} // namespace mc
```

**注意：** 兼容别名中 `namespace BiomeFactory = ::mc::world::biome::BiomeFactory;` 已经在 `BiomeFactory.hpp` 底部，所以 BiomeRegistry.hpp 中只需删除它，不需要重复。

---

#### 任务25：更新 BiomeRegistry.cpp

**需要做的修改：**
1. 删除第 221-2083 行（BiomeFactory 命名空间实现块 + 外层命名空间闭合括号）
2. 删除不再需要的 include：
   - `#include "BiomeEffects.hpp"` — 只被工厂函数使用
   - `#include "common/world/block/registry/VanillaBlocks.hpp"` — 只被工厂函数使用
3. 保留的 include：
   - `#include "BiomeRegistry.hpp"`
   - `#include "common/perfetto/TraceEvents.hpp"`
   - `#include <algorithm>`

**修改后的 BiomeRegistry.cpp 大致结构：**
```cpp
// 版权头

#include "BiomeRegistry.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace biome {

// BiomeRegistry 实现（第38-219行的内容，不变）

} // namespace biome
} // namespace world
} // namespace mc
```

---

#### 任务26：更新 Biomes.hpp

**需要做的修改：**

在 `Biomes.hpp` 中添加 `#include "BiomeFactory.hpp"`：

```cpp
#pragma once

// 生物群系系统聚合头文件

// 核心定义
#include "Biome.hpp"
#include "BiomeClimate.hpp"
#include "BiomeIds.hpp"
#include "BiomeRegistry.hpp"
#include "BiomeFactory.hpp"   // 新增
#include "BiomeSource.hpp"

// Climate 参数系统
#include "climate/Climate.hpp"

// 生物群系源实现
#include "source/EndBiomeSource.hpp"
#include "source/MultiNoiseBiomeSource.hpp"
#include "source/NetherBiomeBuilder.hpp"
#include "source/OverworldBiomeBuilder.hpp"
```

---

#### 任务27：更新 CMakeLists.txt

**文件路径：** `src/common/CMakeLists.txt`

在第 938 行（`world/biome/BiomeRegistry.cpp`）之后添加三个新文件：

```
world/biome/BiomeFactoryOverworld.cpp
world/biome/BiomeFactoryNether.cpp
world/biome/BiomeFactoryEnd.cpp
```

**注意：** CMakeLists.txt 使用显式文件列表（不是 GLOB），必须手动添加。

---

#### 任务28：格式化文件 + 更新 README

对所有修改/新建的文件执行 clang-format：
```bash
"D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" -i <文件路径>
```

更新 `src/common/world/biome/README.md`，添加 BiomeFactory 相关文件说明。

---

## 四、未开始的步骤

### 步骤6：代码规范修复（部分完成）

**已完成：**
- ✅ Biome 构造函数 `name` 参数：`const std::string&` → `std::string_view`
- ✅ BiomeTag::contains：`m_biomeIds.find(id) != m_biomeIds.end()` → `m_biomeIds.contains(id)`
- ✅ Biome::setSpawnInfo：增加移动重载

**未完成：**
- ❌ **BIOME_NULL 哨兵值**：在 `OverworldBiomeBuilder.hpp` 中
  `static constexpr BiomeId BIOME_NULL = static_cast<BiomeId>(-1)`
  → 改为 `static constexpr BiomeId BIOME_NULL = std::numeric_limits<BiomeId>::max()`
  并添加注释说明哨兵值含义

- ❌ **BiomeSource::findBiome 默认参数**：检查 IBiomeSource 中是否有 `stopOnFirst = false` 默认值，如有则移除

### 步骤8：更新 README.md

需要更新以下文件：
- `src/common/world/biome/README.md`
- `src/common/world/biome/climate/README.md`
- `src/common/world/biome/source/README.md`
- `src/common/world/README.md`（biome 部分）

### 步骤10：最终构建验证 + 提交

1. 执行构建：`./scripts/configure.sh build`（超时需30分钟以上）
2. clang-format 所有修改文件
3. 全局搜索验证：
   - `mc::Biome` 无遗漏旧命名空间引用
   - `mc::BiomeFactory` 无遗漏
   - `NetherBiomeSource` 无遗漏
   - `BiomeEffects.cpp` 已删除
   - `Biome::Category` 已删除
4. 提交代码

---

## 五、关键代码参考

### BiomeRegistry.cpp 中 BiomeFactory 代码的范围

| 内容 | 行范围 | 目标文件 |
|------|--------|----------|
| 匿名命名空间辅助函数 `getBlockState` | 227-232 | 三个 .cpp 都需要（各放一份） |
| 基础主世界工厂函数 | 235-858 | BiomeFactoryOverworld.cpp |
| 阶段1工厂函数 | 865-1224 | BiomeFactoryOverworld.cpp |
| 阶段2工厂函数 | 1230-1452 | BiomeFactoryOverworld.cpp |
| 下界工厂函数 | 1458-1663 | BiomeFactoryNether.cpp |
| 末地工厂函数 | 1669-1793 | BiomeFactoryEnd.cpp |
| MC 1.18+ 工厂函数 | 1799-2077 | BiomeFactoryEnd.cpp |

### BiomeRegistry.cpp 中 BiomeRegistry 代码的范围（保留）

| 内容 | 行范围 |
|------|--------|
| include 和命名空间开始 | 24-32 |
| BiomeRegistry::instance() | 38-42 |
| BiomeRegistry 构造函数 | 44-46 |
| BiomeRegistry::initialize() | 48-219 |
| 命名空间结束 | 第281-283行（但实际是2081-2083行，需要改） |

### 命名空间结构

所有新文件必须使用：
```cpp
namespace mc {
namespace world {
namespace biome {

namespace BiomeFactory {
// 函数实现
} // namespace BiomeFactory

} // namespace biome
} // namespace world
} // namespace mc
```

### 关键 include 依赖

| 头文件 | 提供的符号 | 使用场景 |
|--------|-----------|----------|
| `BiomeFactory.hpp` | 所有 createXxx() 声明 | 每个新 .cpp 必须 include |
| `BiomeEffects.hpp` | BiomeEffects::Builder, GrassColorModifier, 颜色常量 | 设置视觉效果 |
| `BiomeAmbientSounds.hpp` | MoodSoundAmbience, SoundAdditionsAmbience, BiomeMusic | 设置环境音效 |
| `BiomeGenerationSettings.hpp` | BiomeGenerationSettings::createXxx() | 设置生成参数 |
| `VanillaBlocks.hpp` | VanillaBlocks::GRASS_BLOCK 等 | 获取 BlockState |
| `MobSpawnInfo.hpp` | MobSpawnInfo::createXxx() | 设置生物生成 |

### 已有的兼容别名机制

在 `BiomeFactory.hpp` 底部：
```cpp
namespace mc {
namespace BiomeFactory = ::mc::world::biome::BiomeFactory;
} // namespace mc
```

在 `BiomeRegistry.hpp` 底部（修改后）：
```cpp
namespace mc {
using BiomeRegistry = ::mc::world::biome::BiomeRegistry;
// 不再包含 BiomeFactory 别名（已在 BiomeFactory.hpp 中）
} // namespace mc
```

---

## 六、构建与格式化命令

```bash
# 构建（超时30分钟以上）
./scripts/configure.sh build

# 格式化单个文件
"D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" -i <文件路径>

# 格式化所有新建文件
"D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe" -i src/common/world/biome/BiomeFactory.hpp src/common/world/biome/BiomeFactoryOverworld.cpp src/common/world/biome/BiomeFactoryNether.cpp src/common/world/biome/BiomeFactoryEnd.cpp src/common/world/biome/BiomeRegistry.hpp src/common/world/biome/BiomeRegistry.cpp src/common/world/biome/Biomes.hpp
```

---

## 七、重要约束

1. **不允许子代理执行编译命令** — 多个子代理同时编译会导致构建系统锁死
2. **必须用 clang-format 格式化** 所有修改的文件后才能提交
3. **CMakeLists.txt 使用显式文件列表** — 新 .cpp 文件必须手动添加
4. **构建超时需30分钟以上**
5. **兼容别名必须保留** — 旧代码仍使用 `mc::Biome`、`mc::BiomeFactory` 等

---

## 八、风险与注意事项

1. **getBlockState 辅助函数**：三个新 .cpp 文件都需要这个匿名命名空间中的辅助函数。每个文件各自放一份（匿名命名空间不跨翻译单元共享），或者考虑在 BiomeFactory.hpp 中声明一个内部链接辅助函数。

2. **BiomeRegistry.cpp 第 281-283 行的命名空间闭合**：删除 BiomeFactory 代码后，BiomeRegistry.cpp 自己也需要命名空间闭合括号，注意保留：
   ```cpp
   } // namespace biome
   } // namespace world
   } // namespace mc
   ```

3. **BiomeRegistry.hpp 的兼容别名**：修改后只保留 `BiomeRegistry` 的别名，`BiomeFactory` 的别名已在 `BiomeFactory.hpp` 中。

4. **VanillaBlocks.hpp 的 include**：BiomeRegistry.cpp 删除工厂函数后不再需要此 include，但 BiomeRegistry.cpp 的 `_registerDefaultBiomes()` 方法可能通过 BiomeFactory 间接使用，确认删除后编译是否通过。

5. **先完成分离再提取辅助函数**：建议先直接复制代码到三个 .cpp 文件，确保编译通过，再进行辅助函数提取（addDefaultOverworldAmbientSounds 等）以减少代码重复。
