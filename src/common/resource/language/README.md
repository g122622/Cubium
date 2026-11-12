# 多语言子系统（Language）

从资源包加载语言表，为 `TranslationTextComponent` 与命令错误消息提供翻译查询；同时作为服务端
把翻译组件字符串化的唯一出口（控制台/日志/断言）。

## 目录结构

```text
language/
└── LanguageManager.hpp/cpp   # 语言表加载（资源包多包合并）、翻译查询、占位符替换、语言发现
```

## 内部模块关系

- 上游：`resource/repository/PackRepository`（资源包读取）、`resource/PackType`（ClientResources）、
  `util/json/JsonRead`（安全 JSON 解析）、`core/Result`。
- 下游：`util/text/TranslationTextComponent`（翻译组件）、`common/command/errors/CommandErrors`
  （错误消息字符串化）、服务端启动装配（`MinecraftServer::initializeLanguageResources`）。
- 加载流程：按资源包优先级从低到高合并（高优先级覆盖低优先级）→ 非默认语言先加载 en_us 兜底 →
  目标语言覆盖 → 锁外一次性替换翻译表 → 锁外触发语言变更回调。

## 上下游外部依赖关系

- 语言数据只来自资源包 `assets/<ns>/lang/<code>.json`；数据包（`data/`）不承载语言文件。
- 服务端只加载 `en_us`：面向玩家的文本以翻译组件原样下发，由 Java 客户端按自己的语言表本地化；
  服务端字符串表仅用于控制台/日志等必须落成字符串的出口。

## 容易踩的坑

- **不要在持锁状态下调用 `get()` / 语言变更回调**：加载路径在锁外解析、锁内只做替换，回调也在锁外；
  否则会与读写锁自死锁（旧实现正是 `loadLanguage` 持锁后调用 `loadFromJson`）。
- **`get()` 在未加载任何翻译时返回键本身**：不要让玩家可见文本依赖服务端语言表兜底，必须发送翻译组件。
- **多命名空间按包合并**：同名键后写覆盖先写；高优先级资源包必须后加载。
- **`currentLanguage()` 按值返回**：`/reload` 会并发替换语言表，不要持有其内部引用。
- **占位符**：`%s` 顺序参数、`%1$s` 位置参数（从 1 开始）、`%%` 转义；参数不足时保留原占位符。
