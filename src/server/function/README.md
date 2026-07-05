#函数系统(Function System)

## 概述

函数系统实现了 MC Java 的数据包函数功能，包括 `.mcfunction` 文件的加载、注册、执行和调度，以及函数标签的加载与执行。支持普通函数（CommandFunction）和宏函数（MacroFunction，`$(var)` 占位符语法）。

对应 MC Java 的：
- `ServerFunctionLibrary` → `FunctionLoader`
- `ServerFunctionManager` → `FunctionManager`
- `TimerQueue<MinecraftServer>` → `TimerQueue`
- `FunctionCallback` / `FunctionTagCallback` → TimerQueue 回调
- `CommandFunction` / `MacroFunction` / `StringTemplate` / `FunctionBuilder` → 同名类

## 目录结构

```
function/
├── CommandFunction.hpp/cpp  - 命令函数（普通 .mcfunction 文件，无 $ 宏行）
├── MacroFunction.hpp/cpp    - 宏函数（含 $ 宏行的 .mcfunction 文件，支持 $(var) 占位符）
├── StringTemplate.hpp/cpp   - $(var) 宏变量语法解析与替换
├── IFunction.hpp            - 函数统一接口（CommandFunction 和 MacroFunction 的基类）
├── FunctionLoader.hpp/cpp   - 从数据包加载 .mcfunction 文件和函数标签 JSON
├── FunctionManager.hpp/cpp  - 函数管理器（注册、查找、执行、tick/load 标签）
├── TimerQueue.hpp/cpp       - 定时器队列（调度延迟执行）
└── README.md                 - 本文档
```

## 数据流

```
.mcfunction 文件
    │
    ▼
FunctionLoader.loadFromDataPackRepository()
    │  第一阶段：解析行、处理注释/连接/$ 宏行
    │  - 若文件含 $ 宏行：构建 MacroFunction（StringTemplate + entries + parameters）
    │  - 否则：构建 CommandFunction（commands 列表）
    ▼
FunctionManager.registerFunction() / registerMacroFunction()
    │  按 ResourceLocation 索引存储（统一为 IFunction 接口）
    ▼
FunctionManager.execute()
    │  - CommandFunction: 逐行通过 CommandRegistry 执行，忽略 arguments
    │  - MacroFunction: 先用 arguments 实例化 $(var) 占位符（带 LRU 缓存），再逐行执行
    ▼
CommandRegistry::execute() → ServerCommandSource

函数标签 JSON 文件 (data/<namespace>/tags/functions/*.json)
    │
    ▼
FunctionLoader.loadFunctionTags()
    │  解析 JSON、处理 replace 语义和 # 标签引用
    ▼
FunctionManager.registerTag()
    │  按 ResourceLocation 索引存储函数 ID 列表
    ▼
FunctionManager.executeTagFunctions() / /function #tag
```

## 调度流程

```
/schedule function <id> <time> [append|replace]
    │
    ▼
TimerQueue.schedule(id, currentTick + time, callback)
    │
    ▼ (每个 tick)
TimerQueue.tick(currentTick)
    │  执行到期事件的回调
    ▼
FunctionManager.execute(id, source)
```

## .mcfunction 文件格式

- 每行一条命令（不含 `/` 前缀）
- `#` 开头的行为注释
- `\` 结尾的行与下一行连接
- `$` 开头的行为宏函数行，使用 `$(var)` 引用形参
  - 整个文件若含至少一个 `$` 宏行，将被注册为 MacroFunction
  - 宏行示例：`say Hello, $(name)!`（`$(name)` 在执行时被实参替换）
  - 变量名只允许 `[A-Za-z0-9_]`
  - 替换后命令长度上限 2,000,000 字符
- 空行被忽略
- 命令长度上限 2,000,000 字符

## 宏函数实例化

宏函数执行流程（对应 MC 1.21.11 MacroFunction#instantiate）：

1. 按 parameters 顺序从 CompoundTag 中取出每个形参对应的 NBT tag
2. 将 tag stringify 为字符串（不同类型规则不同）：
   - Float/Double → `%.15g` 格式化（去尾零）
   - Byte/Short → (int) 值的十进制字符串
   - Long → 值的十进制字符串（无 L 后缀）
   - String → 原始字符串
   - 其他（Int/Compound/List/...）→ SNBT 文本
3. 用字符串列表作为 cache key 查 LRU 缓存（上限 8 条），命中则直接返回
4. 未命中则遍历 entries，对每个 MacroEntry 用 StringTemplate::substitute 替换占位符
5. 缓存最多保留 MAX_CACHE_ENTRIES = 8 条，超出时淘汰最旧

## 函数标签格式

```json
{
  "replace": false,
  "values": [
    "minecraft:foo/bar",
    "#minecraft:tick",
    {"id": "minecraft:optional_func", "required": false},
    {"id": "#minecraft:optional_tag", "required": false}
  ]
}
```

- `values`（必需）：条目列表，每个条目可以是字符串或对象
  - 字符串格式：`"namespace:path"`（直接引用函数）或 `"#namespace:tag"`（引用标签）
  - 对象格式：`{"id": "namespace:path", "required": true/false}`（支持 required 语义）
- `required` 语义（对应 MC Java 的 TagEntry）：
  - `required: true`（默认）：引用的函数/标签必须存在，不存在时输出警告
  - `required: false`：引用的函数/标签不存在时静默跳过
  - 字符串格式条目默认 `required: true`
- `replace`（可选，默认 `false`）：是否替换之前数据包的同名标签内容
- 文件路径：`data/<namespace>/tags/functions/<path>.json`

## 特殊标签

- `minecraft:tick` - 每个服务器 tick 自动执行
- `minecraft:load` - 服务器启动或 /reload 后首次 tick 执行

## /function 命令

```
/function <name>
/function <name> <arguments>
/function <name> with <entity <target>> [path]
/function <name> with <block <pos>> [path]
/function <name> with <storage <id>> [path]
/function #<tag>
/function #<tag> <arguments>
/function #<tag> with ...
```

- `<name>`：函数 ID（如 `minecraft:foo/bar`）或标签 ID（如 `#minecraft:tick`）
- `<arguments>`：内联 SNBT 复合标签（如 `{key:value}`），传给宏函数实例化
- `with <entity|block|storage>`：从实体/方块实体/命令存储取 NBT 数据作为参数
  - `[path]` 可选 NBT 路径，路径必须指向 CompoundTag

权限等级：2（游戏管理员）。

## 与 MC Java 的差异

1. **宏函数支持**：已实现。`$(var)` 占位符语法、CompoundTag 参数实例化、LRU 缓存、StringTemplate 解析均与 MC 1.21.11 行为一致。
2. **多数据包标签合并**：已实现。使用 `DataPackRepository::listResourceStacks()` 获取同一标签在所有数据包中的内容，逆序遍历（从低优先级到高优先级）以匹配 MC Java 的 `TagLoader.load()` 行为，默认追加条目，`replace=true` 时清空已有条目后追加。
3. **调度事件持久化**：TimerQueue 的调度事件在服务器关闭时保存到 level.dat 的 `ScheduledEvents` 字段，在服务器启动时从 level.dat 加载恢复（与 MC Java 行为一致）。
4. **ExecutionContext 简化**：MC Java 有复杂的 ExecutionContext / Frame / CallFunction 系统，当前实现直接逐行执行命令，没有递归深度限制和帧控制。
5. **required=true 验证**：当 `required=true` 的条目（函数或标签引用）缺失时，整个标签会被丢弃（不注册）并记录错误日志，与 MC Java 的 `TagLoader.build()` 行为一致。`required=false` 的缺失条目则静默跳过。

## 集成点

- **IServer**：通过 `functionManager()` 访问 FunctionManager
- **MinecraftServer**：拥有 FunctionManager 实例，在 initializeRegistries() 中加载函数
- **MinecraftServer::tick()**：调用 FunctionManager::tick() 执行 tick/load 标签和 TimerQueue::tick()
- **FunctionCommand**：通过 FunctionManager 执行函数，支持 `#` 前缀标签引用、内联 SNBT 参数和 `with` 子命令
- **ScheduleCommand**：通过 TimerQueue 调度函数
- **ReloadCommand**：重新加载函数和标签
- **PlayerAdvancements**：通过 FunctionManager 执行进度奖励函数

