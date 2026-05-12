# TODO 认领列表

本文档记录项目中被认领的 TODO 项，避免重复工作和冲突。

## 认领记录

| 时间 | 文件路径 | 行号 | TODO 内容摘要 | 状态 | 认领人 |
|------|----------|------|---------------|------|--------|
| 2026-05-13 | src/server/dimension/ServerDimension.cpp | 75 | 实现光照更新调用 | 已完成 | Claude |
| 2026-05-13 | src/common/entity/core/Entity.hpp | 44 | 彻底移除 LegacyEntityType 旧枚举，改用 mc::entity::EntityType | 暂停 | Claude |
| 2026-05-12 | src/common/entity/loot/LootFunctions.cpp | 1130 | 实现 SetStewEffectFunction::apply 方法 | 已完成 | Claude |
| 2026-05-13 | src/common/entity/entities/player/Player.cpp | 1589 | 触发入水粒子效果（水花飞溅）| 已完成 | Claude |

## 已完成的 TODO 详情

### 2026-05-13: ServerDimension 光照更新实现

**原始位置**: `src/server/dimension/ServerDimension.cpp:75`

**完成内容**:
1. 实现了 `ServerDimension::tick()` 中的光照更新调用
2. 参考 MC 1.16.5 `ServerChunkProvider.ChunkExecutor.driveOne()` 中的光照更新逻辑
3. 使用 `hasLightWork()` 检查是否有待处理的光照工作
4. 使用 `tick(std::numeric_limits<i32>::max(), type().hasSkyLight(), true)` 处理所有待处理的光照更新
5. 根据维度类型动态决定是否更新天空光照

## 暂停的 TODO 详情

### 2026-05-13: LegacyEntityType 移除（暂停）

**原始位置**: `src/common/entity/core/Entity.hpp:44`

**暂停原因**: 涉及范围过大，需要修改：
- Entity 基类构造函数签名
- 数十个实体子类构造函数
- 数百处 `legacyType()` 调用（约 60+ 处代码）
- EntityManager 接口
- EntityUtils::legacyTypeToTypeId() 函数
- 所有测试代码

**后续处理建议**: 这是一个架构级重构任务，建议作为独立任务处理。可以通过以下步骤逐步迁移：
1. 首先在 Entity 类中添加 `isPlayer()`, `isItem()`, `isType(const char*)` 等辅助方法
2. 逐步迁移 `legacyType() == LegacyEntityType::XXX` 调用到新方法
3. 修改 EntityManager::getEntitiesByType() 使用字符串参数
4. 最后移除 LegacyEntityType 枚举

## 状态说明

- **进行中**: 正在实现
- **暂停**: 暂时难以解决，等待依赖完成
- **已完成**: 已收敛完成
- **放弃**: 放弃此 TODO，原因见备注

## 已完成的 TODO 详情

### 2026-05-13: 入水粒子效果实现

**原始位置**: `src/common/entity/entities/player/Player.cpp:1589`

**完成内容**:
1. 在 `Entity` 基类中新增了 `doWaterSplashEffect()` 方法，参考 MC 1.16.5 `Entity.doWaterSplashEffect()`
2. 在 `Entity` 基类中新增了 `getSplashSound()` 和 `getHighspeedSplashSound()` 虚方法
3. 在 `Player` 类中覆盖了声音方法，返回玩家专用的溅水声音
4. 在 `Player` 类中覆盖了 `doWaterSplashEffect()`，添加观察者模式检查
5. 修改了 `Player::updateAirSupply()` 调用 `doWaterSplashEffect()` 而非直接播放声音

**实现细节**:
- 粒子数量基于实体宽度：`1 + width * 20`
- 气泡粒子 (Bubble)：位置在实体包围盒内随机，Y 坐标固定在水面上方，速度继承实体速度但 Y 方向减去随机值
- 水溅粒子 (Splash)：位置同气泡，速度继承实体速度
- 声音选择基于速度因子 f1：`sqrt(vx² * 0.2 + vy² + vz² * 0.2) * 0.2`
- f1 < 0.25 使用普通溅水声，否则使用高速溅水声
- 音调随机化：`1.0 + (rand - rand) * 0.4`

### 2026-05-12: SetStewEffectFunction::apply 实现

**原始位置**: `src/common/entity/loot/LootFunctions.cpp:1130`

**完成内容**:
1. 实现了 `SetStewEffectFunction::apply()` 方法，可以为谜之炖菜添加状态效果
2. 新增了 `EffectType` 字符串转换工具函数：
   - `getEffectById(i32)` - 从数值 ID 获取效果类型
   - `getEffectByResourceLocation(ResourceLocation)` - 从资源位置获取效果类型
   - `getEffectResourceLocation(EffectType)` - 获取效果的资源位置
   - `getEffectResourceName(EffectType)` - 获取效果的资源名称
   - `isInstantEffect(EffectType)` - 检查效果是否为瞬间效果
3. 编写了完整的单元测试
