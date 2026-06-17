#函数系统(Function System)

## 概述

函数系统实现了 MC Java 的数据包函数功能，包括 `.mcfunction` 文件的加载、注册、执行和调度，以及函数标签的加载与执行。

对应 MC Java 的：
- `ServerFunctionLibrary` → `FunctionLoader`
- `ServerFunctionManager` → `FunctionManager`
- `TimerQueue<MinecraftServer>` → `TimerQueue`
- `FunctionCallback` / `FunctionTagCallback` → TimerQueue 回调

## 目录结构

```
function/
├── CommandFunction.hpp/cpp  - 命令函数（解析后的 .mcfunction 文件）
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
    │  第一阶段：解析行、处理注释/连接/宏
    ▼
FunctionManager.registerFunction()
    │  按 ResourceLocation 索引存储
    ▼
FunctionManager.execute()
    │  逐行通过 CommandRegistry 执行
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
- `$` 开头的行为宏函数行（当前版本跳过并警告）
- 空行被忽略
- 命令长度上限 2,000,000 字符

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

## 与 MC Java 的差异

1. **无宏函数支持**：MC Java 的 `$variable` 宏语法需要 NBT CompoundTag 参数，当前命令系统不支持。
2. **多数据包标签合并**：当前只读取最高优先级数据包中的标签文件。MC Java 按数据包优先级从高到低遍历同名标签文件，`replace=true` 时清空已有条目后追加。完整的多数据包标签合并需要在 DataPackRepository 层面提供读取所有数据包中同一资源的方法。
3. **无持久化**：TimerQueue 的调度事件不会保存到存档（MC Java 会保存到 level.dat 的 ScheduledEvents）。
4. **ExecutionContext 简化**：MC Java 有复杂的 ExecutionContext / Frame / CallFunction 系统，当前实现直接逐行执行命令，没有递归深度限制和帧控制。
5. **required=true 行为差异**：MC Java 中当 `required=true` 的条目缺失时整个标签会被丢弃（不注册），当前实现仅输出警告并继续构建标签。函数条目的 `required` 验证尚未实现（函数加载顺序不确定），当前运行时所有不存在的函数引用仅输出警告跳过。

## 集成点

- **IServer**：通过 `functionManager()` 访问 FunctionManager
- **MinecraftServer**：拥有 FunctionManager 实例，在 initializeRegistries() 中加载函数
- **MinecraftServer::tick()**：调用 FunctionManager::tick() 执行 tick/load 标签和 TimerQueue::tick()
- **FunctionCommand**：通过 FunctionManager 执行函数，支持 `#` 前缀标签引用
- **ScheduleCommand**：通过 TimerQueue 调度函数
- **ReloadCommand**：重新加载函数和标签
- **PlayerAdvancements**：通过 FunctionManager 执行进度奖励函数
