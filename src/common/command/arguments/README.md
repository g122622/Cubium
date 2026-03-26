# Command Arguments 模块

命令参数解析模块，为 Minecraft 命令系统提供类型安全的参数解析器。

## 目录结构

```
src/common/command/arguments/
├── ArgumentType.hpp      # 参数类型基类 + 基础参数类型（字符串、整数、浮点、布尔、枚举）
├── EntityArgument.hpp    # 实体选择器参数类型（@p, @a, @e, @r, @s）
├── GameModeArgument.hpp  # 游戏模式、资源位置、坐标参数类型
└── ItemArgument.hpp      # 物品参数类型
```

## 文件详解

### ArgumentType.hpp

**职责**：定义参数类型基类和基础参数类型实现。

**核心类**：

#### `ArgumentType<T>` - 参数类型基类模板

```cpp
template<typename T>
class ArgumentType {
public:
    virtual ~ArgumentType() = default;

    // 核心方法：解析参数值
    [[nodiscard]] virtual T parse(StringReader& reader) = 0;

    // 获取参数类型名称（用于帮助信息）
    [[nodiscard]] virtual String getTypeName() const = 0;

    // 获取示例值列表（用于命令提示）
    [[nodiscard]] virtual std::vector<String> getExamples() const;
};
```

#### 内置参数类型

| 类名 | 解析类型 | 说明 | 示例 |
|------|---------|------|------|
| `StringArgumentType` | `String` | 三种模式：SingleWord、QuotablePhrase、GreedyPhrase | `"hello"`, `"\"hello world\""` |
| `IntegerArgumentType` | `i32` | 支持范围检查 | `123`, `-456` |
| `FloatArgumentType` | `f32` | 支持范围检查 | `3.14`, `-2.5` |
| `BoolArgumentType` | `bool` | 布尔值解析 | `true`, `false` |
| `EnumArgumentType<T>` | `T` | 枚举类型模板 | 自定义枚举值 |

**工厂方法模式**：

每个参数类型都提供静态工厂方法：

```cpp
// 字符串参数
auto word = StringArgumentType::word();        // 单词模式
auto phrase = StringArgumentType::string();    // 可引号短语
auto greedy = StringArgumentType::greedyString(); // 贪婪模式

// 整数参数（支持范围限制）
auto anyInt = IntegerArgumentType::integer();
auto positiveInt = IntegerArgumentType::integer(0);  // >= 0
auto rangeInt = IntegerArgumentType::integer(0, 100); // [0, 100]

// 浮点数参数（支持范围限制）
auto anyFloat = FloatArgumentType::floatArg();
auto rangeFloat = FloatArgumentType::floatArg(0.0f, 1.0f);

// 布尔参数
auto boolArg = BoolArgumentType::boolArg();
```

**枚举参数使用示例**：

```cpp
enum class Color { Red, Green, Blue };

auto colorArg = std::make_shared<EnumArgumentType<Color>>();
colorArg->add("red", Color::Red);
colorArg->add("green", Color::Green);
colorArg->add("blue", Color::Blue);
```

---

### EntityArgument.hpp

**职责**：处理实体选择器和玩家名称解析。

**核心类**：

#### `EntitySelector` - 实体选择器

封装实体选择逻辑，包含以下属性：

| 属性 | 类型 | 说明 |
|------|------|------|
| `type` | `EntitySelectorType` | 选择器类型 |
| `limit` | `i32` | 最大选择数量 |
| `isSelf` | `bool` | 是否选择自己 |
| `includesNonPlayers` | `bool` | 是否包含非玩家实体 |
| `single` | `bool` | 是否选择单个实体 |
| `username` | `String` | 玩家名称（直接指定时） |

**选择器类型**：

| 类型 | 符号 | 说明 |
|------|------|------|
| `SinglePlayer` | `@p` | 最近的单个玩家 |
| `AllPlayers` | `@a` | 所有玩家 |
| `AllEntities` | `@e` | 所有实体 |
| `RandomPlayer` | `@r` | 随机玩家 |
| `Self` | `@s` | 自己（命令执行者） |

#### `EntityArgumentType` - 实体参数类型

支持四种模式：

| 模式 | 说明 |
|------|------|
| `SingleEntity` | 单个实体 |
| `MultipleEntities` | 多个实体 |
| `SinglePlayer` | 单个玩家 |
| `MultiplePlayers` | 多个玩家 |

**解析示例**：

```cpp
auto playerArg = EntityArgumentType::player();   // 单个玩家
auto playersArg = EntityArgumentType::players(); // 多个玩家
auto entityArg = EntityArgumentType::entity();   // 单个实体
auto entitiesArg = EntityArgumentType::entities(); // 多个实体

// 解析
StringReader reader("@p[type=cow,limit=5]");
EntitySelector selector = entityArg->parse(reader);
```

**选择器参数**：

支持 `[参数=值,...]` 格式的选择器参数：

| 参数 | 别名 | 说明 |
|------|------|------|
| `limit` | `c` | 限制选择数量 |

---

### GameModeArgument.hpp

**职责**：定义游戏模式、资源位置、坐标等参数类型。

**包含的参数类型**：

#### `GameModeArgumentType` - 游戏模式参数

解析游戏模式名称，支持多种格式：

| 模式 | 完整名称 | 缩写 | 数字 |
|------|---------|------|------|
| Survival | `survival` | `s` | `0` |
| Creative | `creative` | `c` | `1` |
| Adventure | `adventure` | `a` | `2` |
| Spectator | `spectator` | `sp` | `3` |

```cpp
auto gameModeArg = GameModeArgumentType::gameMode();
StringReader reader("creative");
GameMode mode = gameModeArg->parse(reader); // GameMode::Creative
```

#### `ResourceLocationArgumentType` - 资源位置参数

解析命名空间资源标识符：

```cpp
auto locArg = ResourceLocationArgumentType::resourceLocation();

// minecraft:stone -> ResourceLocation("minecraft", "stone")
// stone -> ResourceLocation("minecraft", "stone")  // 默认命名空间
```

#### `BlockPosArgumentType` - 方块位置参数

支持三种坐标格式：

| 格式 | 示例 | 说明 |
|------|------|------|
| 绝对坐标 | `100 64 -200` | 固定坐标 |
| 相对坐标 | `~ ~ ~` | 相对于执行位置 |
| 局部坐标 | `^ ^ ^` | 相对于视线方向 |

```cpp
auto posArg = BlockPosArgumentType::blockPos();
StringReader reader("100 64 -200");
Vector3i pos = posArg->parse(reader);
```

#### `Vec3ArgumentType` - 向量位置参数

与 `BlockPosArgumentType` 类似，但返回浮点坐标：

```cpp
auto vecArg = Vec3ArgumentType::vec3();
StringReader reader("~1.5 ~-0.5 ~5");
Vector3d pos = vecArg->parse(reader);
```

#### `RotationArgumentType` - 旋转参数

解析 yaw 和 pitch 角度：

```cpp
auto rotArg = RotationArgumentType::rotation();
StringReader reader("90 -45");
Vector2f rot = rotArg->parse(reader); // yaw=90, pitch=-45
```

---

### ItemArgument.hpp

**职责**：处理物品相关的参数解析。

**核心类**：

#### `ItemInput` - 物品输入包装器

封装物品ID和可选的NBT数据：

```cpp
class ItemInput {
public:
    ItemId itemId() const;          // 获取物品ID
    bool isValid() const;           // 检查是否有效
    const Item* getItem() const;    // 获取物品对象
    std::unique_ptr<ItemStack> createStack(i32 count) const; // 创建物品堆
};
```

#### `ItemArgumentType` - 物品参数类型

解析物品标识符：

```cpp
auto itemArg = ItemArgumentType::item();

// minecraft:stone -> ItemInput(stone的ItemId)
// diamond_sword -> ItemInput(diamond_sword的ItemId)
```

#### `ItemPredicateArgumentType` - 物品谓词参数类型

用于检查物品是否匹配特定条件（当前与 `ItemArgumentType` 相同，预留扩展）：

```cpp
auto predArg = ItemPredicateArgumentType::itemPredicate();
```

---

## 模块架构

```mermaid
graph TB
    subgraph "arguments 模块"
        A[ArgumentType.hpp<br/>基类 + 基础类型]
        E[EntityArgument.hpp<br/>实体选择器]
        G[GameModeArgument.hpp<br/>游戏模式/坐标]
        I[ItemArgument.hpp<br/>物品参数]
    end

    subgraph "依赖模块"
        SR[StringReader<br/>字符串读取器]
        CC[CommandContext<br/>命令上下文]
        CE[CommandExceptions<br/>异常定义]
        RL[ResourceLocation<br/>资源位置]
        IR[ItemRegistry<br/>物品注册表]
    end

    A --> SR
    A --> CE
    A --> CC

    E --> SR
    E --> CE
    E --> CC

    G --> SR
    G --> CE
    G --> RL

    I --> SR
    I --> CE
    I --> IR

    style A fill:#e1f5fe
    style E fill:#fff3e0
    style G fill:#e8f5e9
    style I fill:#fce4ec
```

---

## 整体职责

### 输入

- `StringReader`：命令字符串读取器，提供游标式解析
- 原始命令字符串片段

### 输出

- 解析后的类型化值（`T` 模板参数）
- 解析失败时抛出 `CommandException`

### 依赖项

| 模块 | 用途 |
|------|------|
| `common/command/StringReader` | 游标式字符串读取 |
| `common/command/CommandContext` | 存储解析后的参数 |
| `common/command/exceptions/CommandExceptions` | 定义错误类型和异常 |
| `common/core/Types` | 基础类型定义 |
| `common/resource/ResourceLocation` | 资源位置解析 |
| `common/item/ItemRegistry` | 物品查找 |

---

## 使用方法

### 1. 定义命令参数

```cpp
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"

// 创建命令节点
auto rootNode = std::make_shared<LiteralCommandNode<ServerPlayer>>("gamemode");

// 添加参数节点
auto modeArg = std::make_shared<ArgumentCommandNode<ServerPlayer, GameMode>>(
    "mode",
    GameModeArgumentType::gameMode()
);

modeArg->setCommand([](CommandContext<ServerPlayer>& ctx) {
    GameMode mode = GameModeArgumentType::getGameMode(ctx, "mode");
    ctx.getSource().setGameMode(mode);
    return 1;
});

rootNode->addChild(modeArg);
```

### 2. 从命令上下文获取参数

```cpp
// 方式一：通过参数类型静态方法
GameMode mode = GameModeArgumentType::getGameMode(context, "mode");
Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos");
ItemInput item = ItemArgumentType::getItem(context, "item");

// 方式二：通过上下文模板方法
GameMode mode = context.getArgument<GameMode>("mode");
```

### 3. 自定义参数类型

```cpp
class ColorArgumentType : public ArgumentType<Color> {
public:
    [[nodiscard]] Color parse(StringReader& reader) override {
        i32 start = reader.getCursor();
        String name = reader.readUnquotedString();

        if (name == "red") return Color::Red;
        if (name == "green") return Color::Green;
        if (name == "blue") return Color::Blue;

        reader.setCursor(start);
        throw CommandException(
            CommandErrorType::Unknown,
            "Unknown color: " + name,
            start
        );
    }

    [[nodiscard]] String getTypeName() const override {
        return "color";
    }

    [[nodiscard]] std::vector<String> getExamples() const override {
        return {"red", "green", "blue"};
    }
};
```

---

## 容易踩的坑

### 1. 游标回退

**问题**：解析失败时必须回退游标到起始位置。

**正确做法**：

```cpp
[[nodiscard]] T parse(StringReader& reader) override {
    i32 start = reader.getCursor();  // 记录起始位置

    // ... 解析逻辑 ...

    if (解析失败) {
        reader.setCursor(start);  // 回退游标！
        throw CommandException(..., start);
    }

    return result;
}
```

### 2. 选择器模式验证

**问题**：`@e` 选择器不能用于只接受玩家的参数。

**解决**：`EntityArgumentType` 会在 `validateSelector` 中检查：

```cpp
// 这会抛出 PlayerTooMany 异常
auto playerArg = EntityArgumentType::player();
StringReader reader("@e[limit=1]");
playerArg->parse(reader);  // 抛出异常
```

### 3. 相对坐标和局部坐标

**问题**：`BlockPosArgumentType` 当前实现只返回绝对坐标，未实现相对/局部坐标的计算。

**注意**：`~` 和 `^` 前缀的坐标需要在命令执行时根据执行者位置/朝向进行计算，这需要额外的上下文信息。

### 4. 物品解析失败

**问题**：物品ID不存在时会抛出异常。

**解决**：确保物品已在 `ItemRegistry` 中注册，或捕获异常进行处理。

### 5. 枚举参数的生命周期

**问题**：`EnumArgumentType` 使用链式调用配置，注意对象生命周期。

**正确做法**：

```cpp
// 方式一：分开创建和配置
auto enumArg = std::make_shared<EnumArgumentType<Color>>();
enumArg->add("red", Color::Red);
enumArg->add("green", Color::Green);

// 方式二：使用 shared_ptr 确保生命周期
auto enumArg = std::shared_ptr<EnumArgumentType<Color>>(
    (new EnumArgumentType<Color>())
        ->add("red", Color::Red)
        ->add("green", Color::Green)
);
```

---

## 测试用例

相关测试位于 `tests/common/command/test_command_dispatcher.cpp`：

| 测试类 | 测试内容 |
|--------|---------|
| `StringReaderTest` | 字符串读取、引号处理、数字解析、范围检查 |
| `ArgumentTypeTest` | 字符串参数、整数参数、浮点参数、布尔参数、枚举参数 |

**示例测试代码**：

```cpp
TEST_F(ArgumentTypeTest, IntegerArgument) {
    auto intArg = IntegerArgumentType::integer(0, 100);

    StringReader reader1("50");
    EXPECT_EQ(intArg->parse(reader1), 50);

    StringReader reader2("150");
    EXPECT_THROW(intArg->parse(reader2), CommandException);

    StringReader reader3("-10");
    EXPECT_THROW(intArg->parse(reader3), CommandException);
}

TEST_F(ArgumentTypeTest, EnumArgument) {
    enum class TestEnum { A, B, C };

    auto enumArg = std::make_shared<EnumArgumentType<TestEnum>>();
    enumArg->add("a", TestEnum::A);
    enumArg->add("b", TestEnum::B);
    enumArg->add("c", TestEnum::C);

    StringReader reader1("a");
    EXPECT_EQ(enumArg->parse(reader1), TestEnum::A);

    StringReader reader3("invalid");
    EXPECT_THROW(enumArg->parse(reader3), CommandException);
}
```

---

## 与 Minecraft 1.16.5 的对应关系

| MC 类 | 本项目类 | 说明 |
|-------|---------|------|
| `ArgumentType<T>` | `ArgumentType<T>` | 完全对应 |
| `StringArgumentType` | `StringArgumentType` | 三种模式对应 |
| `IntegerArgumentType` | `IntegerArgumentType` | 范围检查对应 |
| `FloatArgumentType` | `FloatArgumentType` | 范围检查对应 |
| `BoolArgumentType` | `BoolArgumentType` | 对应 |
| `EntityArgument` | `EntityArgumentType` | 选择器解析对应 |
| `EntitySelector` | `EntitySelector` | 选择器封装对应 |
| `GameModeArgument` | `GameModeArgumentType` | 游戏模式对应 |
| `BlockPosArgument` | `BlockPosArgumentType` | 坐标解析对应 |
| `Vec3Argument` | `Vec3ArgumentType` | 向量解析对应 |
| `ItemArgument` | `ItemArgumentType` | 物品解析对应 |
| `ResourceLocationArgument` | `ResourceLocationArgumentType` | 资源位置对应 |

---

## 扩展计划

当前模块已实现核心参数类型，未来可扩展：

1. **更多选择器参数**：`type`, `sort`, `distance`, `x`, `y`, `z`, `dx`, `dy`, `dz`, `scores`, `tag`, `team`, `name`, `nbt`
2. **NBT 参数类型**：解析 NBT 标签
3. **组件参数类型**：解析文本组件（JSON）
4. **范围参数类型**：`IntRange`, `FloatRange`
5. **时间参数类型**：解析时间字符串（如 `10s`, `5m`, `1h`）
6. **角度参数类型**：解析角度（支持度数和弧度）

---

## 更新历史

| 日期 | 版本 | 变更 |
|------|------|------|
| 2024-01 | 1.0 | 初始版本，包含基础参数类型 |
| 2024-02 | 1.1 | 添加实体选择器参数支持 |
| 2024-03 | 1.2 | 添加坐标和旋转参数类型 |
