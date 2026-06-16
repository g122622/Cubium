# Command Arguments 测试

本目录包含命令参数类型的单元测试。

## 文件结构

```
tests/common/command/arguments/
├── BlockStateArgumentTest.cpp   # 方块状态参数解析测试
├── EntityArgumentTest.cpp       # 实体选择器参数解析测试
└── ItemSlotArgumentTest.cpp     # 物品槽位参数解析测试
```

## 测试文件详解

### BlockStateArgumentTest.cpp

测试 `BlockStateArgumentType` 的方块状态解析功能：

- 方块 ID 解析（带/不带命名空间）
- 属性解析（单个/多个属性）
- 属性验证（无效属性、无效值、重复属性）
- 错误处理

### EntityArgumentTest.cpp

测试 `EntityArgumentType` 和相关类的实体选择器解析功能：

#### FloatRange 测试（FloatRangeTest）

| 测试名称 | 测试内容 |
|----------|----------|
| `DefaultUnbounded` | 默认构造为无界范围 |
| `SetMinOnly` | 仅设置最小值 |
| `SetMaxOnly` | 仅设置最大值 |
| `SetBothBounds` | 设置双边值 |
| `TestWithinRange` | 范围内值测试 |
| `TestSquared` | 平方距离测试 |

#### IntRange 测试（IntRangeTest）

| 测试名称 | 测试内容 |
|----------|----------|
| `DefaultUnbounded` | 默认构造为无界范围 |
| `SetBothBounds` | 设置双边值 |
| `TestWithinRange` | 范围内值测试 |

#### EntitySelector 基础测试（EntitySelectorTest）

| 测试名称 | 测试内容 |
|----------|----------|
| `DefaultConstructor` | 默认构造函数 |
| `SelfFactory` | `@s` 工厂方法 |
| `NearestPlayerFactory` | `@p` 工厂方法 |
| `AllPlayersFactory` | `@a` 工厂方法 |
| `AllEntitiesFactory` | `@e` 工厂方法 |
| `RandomPlayerFactory` | `@r` 工厂方法 |
| `ByUsernameFactory` | 用户名工厂方法 |
| `SetDistance` | 设置距离范围 |
| `SetLevel` | 设置等级范围 |
| `SetCoordinates` | 设置坐标 |
| `SetDimensions` | 设置尺寸 |
| `SetSort` | 设置排序方式 |
| `SetEntityType` | 设置实体类型 |
| `SetEntityTypeNegated` | 设置取反实体类型 |
| `AddTags` | 添加标签 |
| `SetGameMode` | 设置游戏模式 |
| `SetTeam` | 设置队伍 |

#### EntitySelector 角度范围测试（EntitySelectorRotationTest）

| 测试名称 | 测试内容 |
|----------|----------|
| `DefaultRotationRangesAreUnbounded` | 默认角度范围为无界 |
| `SetXRotation` | 设置俯仰角范围 |
| `SetYRotation` | 设置偏航角范围 |

#### EntityArgumentType 解析测试（EntityArgumentParseTest）

| 测试名称 | 测试内容 |
|----------|----------|
| `ParseByUsername` | 解析用户名 |
| `ParseAtP` | 解析 `@p` |
| `ParseAtA` | 解析 `@a` |
| `ParseAtE` | 解析 `@e` |
| `ParseAtR` | 解析 `@r` |
| `ParseAtS` | 解析 `@s` |
| `ParseInvalidSelectorThrows` | 无效选择器抛出异常 |
| `ParseDistanceRange` | 解析距离范围 |
| `ParseDistanceMinOnly` | 解析距离最小值 |
| `ParseDistanceMaxOnly` | 解析距离最大值 |
| `ParseLevelRange` | 解析等级范围 |
| `ParseLimit` | 解析数量限制 |
| `ParseSort` | 解析排序方式 |
| `ParseCoordinates` | 解析坐标 |
| `ParseDimensions` | 解析尺寸 |
| `ParseName` | 解析名称 |
| `ParseNameNegated` | 解析取反名称 |
| `ParseGameMode` | 解析游戏模式 |
| `ParseGameModeNegated` | 解析取反游戏模式 |
| `ParseType` | 解析实体类型 |
| `ParseTypeNegated` | 解析取反实体类型 |
| `ParseTag` | 解析标签 |
| `ParseTagNegated` | 解析取反标签 |
| `ParseTeam` | 解析队伍 |

#### 角度解析测试（EntityArgumentRotationParseTest）

| 测试名称 | 测试内容 |
|----------|----------|
| `ParseXRotationRange` | 解析俯仰角范围 `-45..45` |
| `ParseXRotationMinOnly` | 解析俯仰角最小值 `-30..` |
| `ParseXRotationMaxOnly` | 解析俯仰角最大值 `..30` |
| `ParseXRotationExactValue` | 解析精确俯仰角 `0` |
| `ParseYRotationRange` | 解析偏航角范围 `170..-170` |
| `ParseYRotationWraparound` | 解析跨越边界偏航角 `90..-90` |
| `ParseYRotationMinOnly` | 解析偏航角最小值 `180..` |
| `ParseYRotationMaxOnly` | 解析偏航角最大值 `..-90` |
| `ParseBothRotations` | 同时解析两个角度参数 |
| `ParseRotationWithOtherParams` | 角度与其他参数组合 |
| `XRotationAngleTestMatchesParsed` | 解析后的角度测试匹配 |
| `YRotationWraparoundAngleTest` | 跨越边界角度测试 |

## 测试覆盖要点

### 角度范围环绕处理

角度范围测试重点验证 `-180/180` 度边界环绕逻辑：

- `[170..-170]` 表示接近正北方向（170° 到 -170°，跨越 180°）
- 当 `min > max` 时使用 OR 逻辑：`value >= min || value <= max`
- 普通范围使用 AND 逻辑：`value >= min && value <= max`

### 取反参数解析

测试验证 `!` 前缀的正确处理：

- `name=!Steve` - 排除名称为 Steve 的实体
- `gamemode=!creative` - 排除创造模式玩家
- `type=!minecraft:player` - 排除玩家
- `tag=!foo` - 排除带有 foo 标签的实体

### 范围解析格式

测试支持的范围格式：

| 格式 | 示例 | 说明 |
|------|------|------|
| 精确值 | `10` | min = max = 10 |
| 最小值到最大值 | `10..20` | min = 10, max = 20 |
| 仅最小值 | `10..` | min = 10, max = 无界 |
| 仅最大值 | `..20` | min = 无界, max = 20 |
| 负数范围 | `-45..45` | min = -45, max = 45 |
| 跨越边界 | `170..-170` | min = 170, max = -170 |
