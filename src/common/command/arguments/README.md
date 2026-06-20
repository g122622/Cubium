# Command Arguments 模块

命令参数解析模块，为 Minecraft 命令系统提供类型安全的参数解析器。

## 目录结构

```
src/common/command/arguments/
├── ArgumentType.hpp            # 参数类型基类 + 基础参数类型（字符串、整数、浮点、布尔、枚举）
├── BlockStateArgument.hpp      # 方块状态参数类型（支持属性解析）
├── EntityArgument.hpp/.cpp     # 实体选择器参数类型（@p, @a, @e, @r, @s）
├── FunctionArgument.hpp/.cpp   # 函数参数类型（支持 # 标签前缀和函数名解析）
├── GameModeArgument.hpp        # 游戏模式、资源位置、坐标参数类型（含 Vec2ArgumentType、Vec3ArgumentType、RotationArgumentType 等）
├── ItemArgument.hpp            # 物品参数类型 + ItemInput 包装器
├── ItemSlotArgument.hpp        # 物品槽位参数类型 + ItemSlot 索引类
├── NbtPath.hpp/.cpp            # NBT 路径类和节点实现
├── NbtPathArgumentType.hpp/.cpp # NBT 路径/复合标签/标签参数类型
└── TimeArgument.hpp            # 时间参数类型（支持 s/d/t 后缀，解析为 tick 数）
```

## 内部模块关系

```
ArgumentType.hpp (基类模板)
    ├── EntityArgument.hpp      → 继承 ArgumentType<EntitySelector>
    ├── FunctionArgument.hpp    → 继承 ArgumentType<FunctionArgumentResult>（# 标签前缀 + 函数名解析）
    ├── GameModeArgument.hpp    → 继承 ArgumentType<T> (GameMode/ResourceLocation/Vector3i/Vector3d/Vector2d/Vector2f)
    ├── BlockStateArgument.hpp  → 继承 ArgumentType<BlockStateInput>
    ├── ItemArgument.hpp        → 继承 ArgumentType<ItemInput>
    ├── ItemSlotArgument.hpp    → 继承 ArgumentType<ItemSlot>（槽位名称→索引映射）
    ├── NbtPathArgumentType.hpp → 继承 ArgumentType<NbtPath>
    └── TimeArgument.hpp        → 继承 ArgumentType<i32>（时间字符串→tick 数，支持 s/d/t 后缀）

FunctionArgument.hpp
    └── FunctionArgumentResult  → 解析结果数据类（ResourceLocation + isTag 标志，延迟解析）
    └── FunctionArgumentType    → 解析器（# 前缀检测 → 标签引用，否则 → 函数引用）
    └── FunctionSuggestionProvider (server/command/support/) → Tab 补全建议

NbtPath.hpp
    └── NbtPathArgumentType.hpp → 使用 NbtPath 作为返回类型

ItemSlotArgument.hpp
    └── ItemSlotArgumentType    → ItemSlot 索引类 + 解析器

EntityArgument.hpp
    └── EntityArgument.cpp      → 实现复杂的选择器解析逻辑
    └── EntitySelector           → 选择器数据模型（含 createAabb/hasVolume 等体积过滤方法）
```

## 上下游外部依赖关系

**本模块依赖：**
- `common/command/StringReader` - 游标式字符串读取
- `common/command/CommandContext` - 存储解析后的参数
- `common/command/exceptions/CommandExceptions` - 定义错误类型和异常
- `common/core/Types` - 基础类型定义（GameMode 等）
- `common/resource/ResourceLocation` - 资源位置解析
- `common/item/core/ItemRegistry` - 物品查找
- `common/world/block/BlockRegistry` - 方块查找
- `common/util/nbt/Nbt` - NBT 标签系统
- `common/util/math/Vector2`、`Vector3` - 向量类型

**被依赖方：**
- `common/command/CommandNode` - 使用参数类型构建命令树
- `common/command/CommandDispatcher` - 调用参数解析器
- 各具体命令实现（如 `/gamemode`、`/give`、`/summon`、`/data` 等）

## 容易踩的坑

### 1. 游标回退

解析失败时必须回退 `StringReader` 游标到起始位置：

```cpp
[[nodiscard]] T parse(StringReader& reader) override {
    i32 start = reader.getCursor();  // 记录起始位置
    // ... 解析逻辑 ...
    if (解析失败) {
        reader.setCursor(start);  // 必须回退！
        throw CommandException(..., start);
    }
    return result;
}
```

### 2. 选择器模式验证

`@e` 选择器不能用于只接受玩家的参数，`EntityArgumentType` 会在 `validateSelector` 中检查并抛出 `PlayerTooMany` 异常。

### 3. 相对坐标和局部坐标

`BlockPosArgumentType` 当前实现只返回绝对坐标，未实现相对（`~`）和局部（`^`）坐标的计算。这需要命令执行时的额外上下文信息。

### 4. 物品/方块解析失败

物品ID或方块ID不存在时会抛出异常。确保相关注册表已初始化，或捕获异常进行处理。

### 5. 枚举参数的生命周期

`EnumArgumentType` 使用链式调用配置，注意对象生命周期：

```cpp
// 正确：分开创建和配置
auto enumArg = std::make_shared<EnumArgumentType<Color>>();
enumArg->add("red", Color::Red);

// 正确：使用 shared_ptr 确保生命周期
auto enumArg = std::shared_ptr<EnumArgumentType<Color>>(
    (new EnumArgumentType<Color>())->add("red", Color::Red)
);
```

### 6. 角度范围测试

`FloatRange::testAngle()` 用于 `x_rotation`/`y_rotation` 参数，需注意角度环绕问题：角度会被规范化到 [-180, 180)，当 `min > max` 时表示范围跨越 -180/180 边界（如 `[170..-170]` 表示接近正北方向）。

### 7. NBT 过滤和谓词过滤

`EntitySelector` 中的 `NbtCondition` 和 `PredicateCondition` 目前只实现了参数解析，过滤逻辑需要 Entity 类支持 NBT 序列化和 LootConditionManager 支持。

### 8. 相对/局部坐标的执行时计算

`BlockPosArgumentType` 和 `Vec3ArgumentType` 解析 `~` 和 `^` 前缀时，需要在命令执行时根据执行者位置/朝向进行计算，这不在参数解析阶段完成。

### 9. ItemSlot 槽位编号重叠

`ItemSlotArgument` 中 `player.cursor`(499) 与 `horse.chest`(499) 编号重叠，`player.crafting.0~3`(500-503) 与 `horse.0~3`(500-503) 编号重叠。原版中通过不同命令上下文区分，当前实现中 `player.crafting` 优先匹配，需在后续根据上下文细化。

### 10. EntitySelector 体积过滤（createAabb）

`EntitySelector::createAabb()` 按 MC 原版 `EntitySelectorParser.createAabb` 逻辑从 `dx/dy/dz` 构造选择 AABB：负值 delta 赋给 min 侧，正值赋给 max 侧，max 侧额外加 1.0。当无 `dx/dy/dz` 但有 `distance` 最大值时，从最大距离构造立方体 AABB。返回 `std::optional<AxisAlignedBB>`，无体积约束时为 `std::nullopt`。EntityResolver 中的体积过滤使用 `AABB.intersects(entity.boundingBox())` 进行碰撞箱相交检查，而非位置点包含检查——两者在实体体积较大或跨越选择边界时行为不同。

### 11. TimeArgumentType 单位映射

`TimeArgumentType` 解析数字加可选后缀并转换为 tick 数。单位映射表：`"d"` → 24000（天）、`"s"` → 20（秒）、`"t"` → 1（tick）、`""`（无后缀）→ 1（tick）。与 MC Java 版 `TimeArgument` 完全对齐。支持浮点数输入（如 `"1.5d"` → 36000 tick），最终通过 `std::round` 四舍五入为整数。无效后缀抛出 `CommandErrorType::Unknown` 异常；计算结果低于 minimum 抛出 `CommandErrorType::IntegerTooLow` 异常。
