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

## 内部模块关系

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

## 上下游外部依赖关系

**被谁依赖（下游）：**
- `server/command/commands/` - 所有具体命令实现（gamemode、tp、give 等）
- `server/command/support/` - 命令支持类（PlayerResolver、CommandMetadata 等）
- `server/interaction/SignCommandHelper.cpp` - 告示牌命令交互
- `client/network/ClientPlayVisitor.cpp` - 客户端命令树处理（`ir::play::Commands` 分支，当前为 opaque TODO(Phase6)）

**依赖了谁（上游）：**
- 无外部依赖，仅依赖标准库和项目基础类型

## 设计参考

本模块参考 Minecraft Java Edition 1.16.5 的 Brigadier 命令框架设计：
- `CommandDispatcher` ≈ `com.mojang.brigadier.CommandDispatcher`
- `CommandNode` ≈ `com.mojang.brigadier.tree.CommandNode`
- `LiteralCommandNode` / `ArgumentCommandNode` ≈ 同名类
- `CommandContext` / `StringReader` / `ArgumentType` ≈ 同名类

## 容易踩的坑

### 参数类型模板参数

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

### 参数获取类型匹配

使用 `getArgument<T>()` 时，类型必须与注册时的参数类型完全匹配。

```cpp
i32 value = ctx.getArgument<i32>("value");  // 正确
// f32 value = ctx.getArgument<f32>("value");  // 错误！会抛出 std::bad_any_cast
```

### 命令别名使用重定向

**问题**：复制子子树创建别名会导致命令树、帮助输出和建议不同步。

**解决**：命令别名应使用 `CommandNode::setRedirect(...)` 而不是复制子子树。`/teleport` 和 `/xp` 现在依赖重定向，因此命令树、帮助输出和建议都保持同步。

### Tab 补全入口点

**问题**：在命令中硬编码 Tab 列表会导致补全不灵活。

**解决**：`CommandDispatcher::getSuggestions()` 是规范的 Tab 补全入口点。通过 `CommandNode::setCustomSuggestions(...)` 附加动态补全数据，而不是在命令中硬编码 Tab 列表。

### 命令名称来源

**问题**：重新引入单独的手动名称列表会导致别名不同步。

**解决**：`CommandRegistry::getCommandNames()` 派生自调度器树。帮助输出自动跟踪别名和未来的命令注册，所以不要重新引入单独的手动名称列表。

### CommandTreePacket 同步

**问题**：客户端补全必须在登录后从 `onCommandTree()` 重建，并在断开连接时再次清除，否则聊天建议会变得陈旧。

**解决**：`CommandTreePacket` 是客户端的权威命令快照，必须正确处理重建和清除。

### CommandTreePacket 封装

**问题**：`CommandTreePacket` 只序列化包体，双重封装会导致内部头看起来像空 JSON 字符串。

**解决**：
- 旧 1.16.5 字节协议下服务器代码须用恰好一次包头封装；新网络层（IR + Java wire codec）已不再使用 12 字节头封装，命令树改走 `ir::play::Commands` codec
- 客户端代码必须在调用 `handleCommandTree()` 之前剥离外部网络头（旧路径）

### 异常处理

`CommandException` 包含错误位置 `cursor`，可用于高亮错误位置。

### 命令源生命周期

`CommandContext` 持有命令源的引用（非指针），确保命令源在执行期间有效。

### 命令执行返回值

命令回调返回 `i32`，表示影响的实体数量。返回 0 表示失败。
