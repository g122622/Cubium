# Command Arguments 模块

命令参数解析模块，为 Minecraft 命令系统提供类型安全的参数解析器。

## 目录结构

```
src/common/command/arguments/
├── ArgumentType.hpp            # 参数类型基类 + 基础参数类型（字符串、整数、浮点、布尔、枚举）
├── BlockStateArgument.hpp      # 方块状态参数类型（支持属性解析）
├── EntityArgument.hpp/.cpp     # 实体选择器参数类型（@p, @a, @e, @r, @s）
├── GameModeArgument.hpp        # 游戏模式、资源位置、坐标参数类型
├── ItemArgument.hpp            # 物品参数类型
├── NbtPath.hpp/.cpp            # NBT 路径类和节点实现
└── NbtPathArgumentType.hpp/.cpp # NBT 路径/复合标签/标签参数类型
```

## 内部模块关系

```
ArgumentType.hpp (基类模板)
    ├── EntityArgument.hpp      → 继承 ArgumentType<EntitySelector>
    ├── GameModeArgument.hpp    → 继承 ArgumentType<T> (GameMode/ResourceLocation/Vector3i/Vector3d/Vector2f)
    ├── BlockStateArgument.hpp  → 继承 ArgumentType<BlockStateInput>
    ├── ItemArgument.hpp        → 继承 ArgumentType<ItemInput>
    └── NbtPathArgumentType.hpp → 继承 ArgumentType<NbtPath>

NbtPath.hpp
    └── NbtPathArgumentType.hpp → 使用 NbtPath 作为返回类型

EntityArgument.hpp
    └── EntityArgument.cpp      → 实现复杂的选择器解析逻辑
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
