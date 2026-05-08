# Command System

命令系统模块，提供 Minecraft 风格的命令解析、执行、建议和重定向框架。

## 目录结构

```
common/command/
├── CommandDispatcher.hpp    # 命令分发器（解析、执行、建议、重定向）
├── CommandNode.hpp          # 命令节点（树结构、重定向、自定义建议）
├── CommandContext.hpp       # 命令上下文
├── CommandResult.hpp        # 执行结果
├── CommandSource.hpp        # 命令源（玩家/控制台）
├── ICommandSource.hpp       # 命令源接口
├── StringReader.hpp         # 字符串读取器
├── arguments/               # 参数类型
│   ├── ArgumentType.hpp     # 参数类型基类 + 内置类型
│   ├── EntityArgument.hpp   # 实体选择器参数
│   ├── GameModeArgument.hpp # 游戏模式/位置参数
│   └── ItemArgument.hpp     # 物品参数
├── exceptions/              # 异常定义
│   └── CommandExceptions.hpp
└── suggestions/             # 自动补全
    └── Suggestions.hpp
```

## 模块职责

### 核心组件

#### CommandDispatcher.hpp
**命令分发器** - 核心解析和执行引擎

- 注册命令节点到命令树
- 解析命令字符串（支持 `/` 前缀自动跳过）
- 执行已解析的命令
- 提供命令路径查询、歧义检测和建议生成
- 优先遍历字面量节点，再遍历参数节点，保持解析顺序更接近原版 Brigadier
- 支持节点重定向，可用于别名命令和未来的上下文切换命令

```cpp
CommandDispatcher<CommandSource> dispatcher;

// 注册命令
auto node = std::make_shared<LiteralCommandNode<CommandSource>>("gamemode");
node->setCommand([](CommandContext<CommandSource>& ctx) { return 1; });
dispatcher.registerCommand(node);

// 执行命令
auto result = dispatcher.execute("gamemode", source);
```

#### CommandNode.hpp
**命令节点** - 构建命令树的基本单元

包含三种节点类型：
- `RootCommandNode` - 根节点
- `LiteralCommandNode` - 字面量节点（如 `gamemode`）
- `ArgumentCommandNode` - 参数节点（如 `<mode>`）

特性：
- 支持子节点添加
- 权限检查（`RequirementPredicate`）
- 命令执行回调
- 重定向支持
- 自定义建议提供器
- 节点示例值（用于帮助和补全）

```cpp
auto literal = std::make_shared<LiteralCommandNode<Source>>("test");
literal->setCommand([](CommandContext<Source>&) { return 1; });
literal->setRequirement([](const Source& s) { return s.hasPermission(2); });
```

#### CommandContext.hpp
**命令上下文** - 存储解析和执行过程中的信息

- 命令源引用
- 原始输入字符串
- 解析的参数（通过 `getArgument<T>()` 获取）
- 当前节点路径

```cpp
template<typename S>
class CommandContext {
    T getArgument<T>(const std::string& name) const;
    T getArgumentOr<T>(const std::string& name, const T& defaultValue) const;
    bool hasArgument(const std::string& name) const;
};
```

#### StringReader.hpp
**字符串读取器** - 游标式字符串解析器

功能：
- 维护当前位置游标
- 支持回退
- 提供各种类型的解析方法

```cpp
StringReader reader("123 45.6 \"hello world\"");
i32 num = reader.readInt();        // 123
reader.skipWhitespace();
f64 dec = reader.readDouble();     // 45.6
reader.skipWhitespace();
std::string str = reader.readString();  // "hello world"
```

### 参数类型

#### ArgumentType.hpp
**参数类型基类 + 内置类型**

| 类型 | 描述 |
|------|------|
| `StringArgumentType` | 字符串（SingleWord/QuotablePhrase/GreedyPhrase） |
| `IntegerArgumentType` | 整数（支持范围检查） |
| `FloatArgumentType` | 浮点数（支持范围检查） |
| `BoolArgumentType` | 布尔值（true/false） |
| `EnumArgumentType<T>` | 枚举（模板类） |

```cpp
auto intArg = IntegerArgumentType::integer(0, 100);  // 0-100 范围
auto strArg = StringArgumentType::greedyString();    // 读取剩余所有内容
auto enumArg = std::make_shared<EnumArgumentType<Color>>();
enumArg->add("red", Color::Red).add("blue", Color::Blue);
```

#### EntityArgument.hpp
**实体选择器参数**

支持的选择器：
- `@p` - 最近玩家
- `@a` - 所有玩家
- `@e` - 所有实体
- `@r` - 随机玩家
- `@s` - 自己
- 玩家名称/UUID

```cpp
auto playerArg = EntityArgumentType::player();      // 单个玩家
auto entitiesArg = EntityArgumentType::entities();  // 多个实体
```

#### GameModeArgument.hpp
**游戏模式/位置参数**

| 类型 | 描述 |
|------|------|
| `GameModeArgumentType` | 游戏模式（survival/creative/adventure/spectator） |
| `ResourceLocationArgumentType` | 资源位置（namespace:path） |
| `BlockPosArgumentType` | 方块位置（支持 `~` 相对坐标） |
| `Vec3ArgumentType` | 向量位置（浮点坐标） |
| `RotationArgumentType` | 旋转角度（yaw, pitch） |

#### ItemArgument.hpp
**物品参数**

| 类型 | 描述 |
|------|------|
| `ItemArgumentType` | 物品（如 `minecraft:stone`） |
| `ItemPredicateArgumentType` | 物品谓词 |

### 异常处理

#### CommandExceptions.hpp
**命令异常定义**

错误类型枚举 `CommandErrorType`：
- 分发器错误：`DispatcherUnknownCommand`, `DispatcherUnknownArgument`, `DispatcherExpectedLiteral`
- 参数错误：`IntegerExpected`, `IntegerTooLow`, `FloatExpected`, `BoolExpected`
- 实体选择器错误：`EntityNotFound`, `PlayerNotFound`, `EntityTooMany`, `EntitySelectorInvalid`
- 权限错误：`PermissionDenied`

```cpp
class CommandException : public std::runtime_error {
    CommandErrorType type() const;
    const std::string& message() const;
    i32 cursor() const;  // 错误位置
    CommandException withInput(std::string_view input) const;
};
```

### 自动补全

#### Suggestions.hpp
**自动补全建议系统**

- `SuggestionsBuilder` 支持按 token 范围构建建议，避免把后续输入误当作当前补全前缀
- `CandidateSuggestionProvider` 之类的提供器可以挂到参数节点上
- `CommandDispatcher::getSuggestions()` 会自动汇总当前节点下的字面量和参数建议

```cpp
SuggestionsBuilder builder("gam", 0);
builder.suggest("gamemode").suggest("give");
Suggestions suggestions = builder.build();

// 候选词过滤
template<typename S>
class CandidateSuggestionProvider : public ISuggestionProvider<S>;
```

## 文件依赖关系

```
CommandDispatcher.hpp
    ├── Suggestions.hpp
    ├── CommandNode.hpp
    ├── CommandContext.hpp
    ├── StringReader.hpp
    └── CommandResult.hpp

CommandNode.hpp
    ├── Suggestions.hpp
    ├── CommandExceptions.hpp
    ├── CommandResult.hpp
    ├── StringReader.hpp
    └── ArgumentType.hpp

CommandContext.hpp
    ├── CommandNode.hpp
    ├── CommandResult.hpp
    └── StringReader.hpp

CommandSource.hpp
    ├── ICommandSource.hpp
    └── CommandExceptions.hpp

ArgumentType.hpp
    ├── StringReader.hpp
    └── CommandExceptions.hpp

EntityArgument.hpp / GameModeArgument.hpp / ItemArgument.hpp
    ├── ArgumentType.hpp
    ├── CommandContext.hpp
    └── CommandExceptions.hpp
```

## 使用方法

### 1. 创建命令分发器

```cpp
#include "common/command/CommandDispatcher.hpp"
#include "common/command/arguments/ArgumentType.hpp"

using namespace mc::command;

CommandDispatcher<CommandSource> dispatcher;
```

### 2. 注册简单命令

```cpp
// 使用构建器
auto node = literal<CommandSource>("hello")
    .executes([](CommandContext<CommandSource>& ctx) {
        ctx.getSource().sendFeedback("Hello, World!");
        return 1;
    })
    .build();

dispatcher.registerCommand(node);
```

### 3. 注册带参数的命令

```cpp
auto node = literal<CommandSource>("gamemode")
    .then(argument<CommandSource, GameMode>(
        "mode",
        std::make_shared<ArgumentCommandNode<CommandSource, GameMode>>(
            "mode",
            GameModeArgumentType::gameMode()
        )
    ).executes([](CommandContext<CommandSource>& ctx) {
        GameMode mode = ctx.getArgument<GameMode>("mode");
        // 设置游戏模式...
        return 1;
    }).build())
    .build();

dispatcher.registerCommand(node);
```

### 4. 执行命令

```cpp
CommandSource source = CommandSource::forConsole(server);
Result<CommandResult> result = dispatcher.execute("gamemode creative", source);

if (result.success()) {
    i32 affectedCount = result.value().result();
} else {
    source.sendError(result.error().message());
}
```

### 5. 使用命令源

```cpp
// 控制台命令源
CommandSource consoleSource = CommandSource::forConsole(server);

// 玩家命令源
CommandSource playerSource(entity, position, rotation, world, 2, player.getName(), server, &player);

// 检查权限
if (source.hasPermission(2)) {
    // 执行管理员命令
}

// 创建派生命令源
CommandSource newSource = source.withPosition(newPos).withRotation(newRot);
```

## 设计参考

本模块参考 Minecraft Java Edition 1.16.5 的 Brigadier 命令框架设计：

- `CommandDispatcher` ≈ `com.mojang.brigadier.CommandDispatcher`
- `CommandNode` ≈ `com.mojang.brigadier.tree.CommandNode`
- `LiteralCommandNode` ≈ `LiteralCommandNode`
- `ArgumentCommandNode` ≈ `ArgumentCommandNode`
- `CommandContext` ≈ `CommandContext`
- `StringReader` ≈ `StringReader`
- `ArgumentType` ≈ `ArgumentType`

## 容易踩的坑

### 1. 参数类型模板参数

`ArgumentCommandNode` 需要两个模板参数：命令源类型和参数类型。

```cpp
// 错误
auto arg = std::make_shared<ArgumentCommandNode<CommandSource>>("value", ...);

// 正确
auto arg = std::make_shared<ArgumentCommandNode<CommandSource, i32>>(
    "value",
    IntegerArgumentType::integer()
);
```

### 2. 构建器链式调用

构建器的 `then()` 方法需要传递 `build()` 后的节点，或使用构建器直接调用。

```cpp
// 方式1：构建后传递
auto childNode = literal<Source>("sub").build();
literal<Source>("parent").then(childNode);

// 方式2：传递构建器（需要模板支持）
literal<Source>("parent").then(literal<Source>("sub"));
```

### 3. 命令执行返回值

命令回调返回 `i32`，表示影响的实体数量。返回 0 表示失败。

```cpp
node->setCommand([](CommandContext<Source>& ctx) {
    // 返回影响的实体数
    return 1;
});
```

### 4. 异常处理

`CommandException` 包含错误位置 `cursor`，可用于高亮错误位置。

```cpp
try {
    i32 value = reader.readInt();
} catch (const CommandException& e) {
    // e.cursor() 是错误位置
    // e.message() 是错误消息
    // e.type() 是错误类型
}
```

### 5. 命令源生命周期

`CommandContext` 持有命令源的引用（非指针），确保命令源在执行期间有效。

```cpp
// CommandContext 持有 S&
template<typename S>
class CommandContext {
    S& m_source;  // 引用，非指针
};
```

### 6. 参数获取类型匹配

使用 `getArgument<T>()` 时，类型必须与注册时的参数类型完全匹配。

```cpp
// 注册时使用 IntegerArgumentType
auto arg = IntegerArgumentType::integer();

// 获取时必须使用 i32
i32 value = ctx.getArgument<i32>("value");  // 正确
// f32 value = ctx.getArgument<f32>("value");  // 错误！会抛出 std::bad_any_cast
```

### 7. 命令别名使用重定向

**问题**：复制子子树创建别名会导致命令树、帮助输出和建议不同步。

**解决方案**：命令别名应使用 `CommandNode::setRedirect(...)` 而不是复制子子树。`/teleport` 和 `/xp` 现在依赖重定向，因此命令树、帮助输出和建议都保持同步。

### 8. Tab 补全入口点

**问题**：在命令中硬编码 Tab 列表会导致补全不灵活。

**解决方案**：`CommandDispatcher::getSuggestions()` 是规范的 Tab 补全入口点。通过 `CommandNode::setCustomSuggestions(...)` 附加动态补全数据，而不是在命令中硬编码 Tab 列表。

### 9. 命令名称来源

**问题**：重新引入单独的手动名称列表会导致别名不同步。

**解决方案**：`CommandRegistry::getCommandNames()` 派生自调度器树。帮助输出自动跟踪别名和未来的命令注册，所以不要重新引入单独的手动名称列表。

### 10. CommandTreePacket 同步

**问题**：客户端补全必须在登录后从 `onCommandTree()` 重建，并在断开连接时再次清除，否则聊天建议会变得陈旧。

**解决方案**：`CommandTreePacket` 是客户端的权威命令快照，必须正确处理重建和清除。

### 11. CommandTreePacket 封装

**问题**：`CommandTreePacket` 只序列化包体，双重封装会导致内部头看起来像空 JSON 字符串。

**解决方案**：
- 服务器代码必须用 `ConnectionManager::encapsulatePacket()` 恰好包装一次
- 客户端代码必须在调用 `handleCommandTree()` 之前剥离外部网络头

## 测试用例

测试文件：`tests/common/command/test_command_dispatcher.cpp`

### 测试覆盖

| 测试套件 | 测试内容 |
|----------|----------|
| `StringReaderTest` | 基本读取、字符串读取、数字读取、布尔读取、空白跳过、期望匹配 |
| `CommandNodeTest` | 字面量节点、命令回调、权限检查、子节点管理、根节点 |
| `ArgumentTypeTest` | 字符串参数、整数参数（范围检查）、浮点参数、布尔参数、枚举参数 |
| `CommandResultTest` | 成功结果、失败结果 |
| `CommandExceptionTest` | 异常创建、简单异常、带输入的异常 |
| `SuggestionsTest` | 建议构建、建议应用、建议合并、建议比较 |
| `CommandDispatcherTest` | 分发器创建、命令注册、命令解析、命令执行、参数存储、未知参数错误 |
| `CommandSourceTest` | 静默命令源 |

### 运行测试

```powershell
./build/bin/Release/mc_tests.exe --gtest_filter="*Command*"
```

## 扩展建议

### 添加新的参数类型

1. 继承 `ArgumentType<T>`
2. 实现 `parse()` 方法
3. 实现 `getTypeName()` 和 `getExamples()` 方法
4. 添加静态工厂方法

```cpp
class MyArgumentType : public ArgumentType<MyType> {
public:
    MyType parse(StringReader& reader) override {
        // 解析逻辑
    }
    
    std::string getTypeName() const override { return "my_type"; }
    
    std::vector<std::string> getExamples() const override {
        return {"example1", "example2"};
    }
    
    static std::shared_ptr<MyArgumentType> myArg() {
        return std::make_shared<MyArgumentType>();
    }
};
```

### 添加新的命令

在 `src/server/command/commands/` 目录下创建命令实现文件，参考现有命令如 `GameModeCommand.hpp`。
