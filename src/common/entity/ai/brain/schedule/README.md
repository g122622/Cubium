# 活动类型

Brain 作息表（`Schedule`）与任务分组所用的**活动标识**。活动是 Brain 中"实体当前在做什么"
这一层的名词，`Schedule` 按时间选活动，活动再映射到一组任务。

## 目录结构树

```text
schedule/
├── Activity.hpp      # 活动类型：字符串键 + 预计算哈希，按值比较
└── Activity.cpp      # 15 个预定义活动的静态成员定义
```

## 内部模块关系

`Activity` 是轻量值类型：内部只持有一个 `std::string m_key` 与预计算的 `size_t m_hash`，
`operator==` / `!=` / `<` 全部按 key 比较，因此可以直接作为 `std::map` / `unordered_map`
的键。预定义活动是 `static` 成员而非 `enum`，新增活动只需在 `.hpp` 声明、`.cpp` 用
`_registerActivity("名字")` 定义即可，不需要改动任何 switch 或表。

`Activity` 本身不持有任务；活动到任务的映射在 `Brain` 的 `m_tasks[priority][activity]` 里。

## 上下游外部依赖关系

上游只依赖标准库（`<string>`、`<unordered_map>`、`<unordered_set>`、`<functional>`）。
下游是 `Brain`（按活动索引任务表）、`Schedule`（时间线选活动）与各实体的
`registerBrainGoals`（把任务挂到某个活动下）。

## 容易踩的坑

- **活动名是字符串，拼错不会编译报错。** `Activity::IDLE` 与 `Activity("idle")` 相等，
  但 `Activity("Idle")` 是另一个活动——大小写敏感，且 `_registerActivity` 不校验重复。
  新增活动时请沿用已有命名风格（全小写 + 下划线）。
- **`static` 成员的初始化顺序**：15 个 `Activity` 是跨翻译单元的静态对象，
  在其它翻译单元的静态初始化里引用它们可能拿到未构造的对象。需要静态注册的场景应
  改到显式的初始化函数中，不要在全局对象的构造函数里读 `Activity::XXX`。
- **活动与任务表是耦合的**：`Brain::registerActivity` 只有在该活动下至少注册了一个任务时
  才会创建 `m_tasks[priority][activity]` 条目。如果只用 `Schedule` 引用某个活动而不注册
  任务，该活动在事件调度（`_tryEventActivity`）里不可见。给时间线新增活动时，
  必须同时确认它下面挂了任务。
- 预定义活动共 15 个：`core` / `idle` / `work` / `play` / `rest` / `meet` / `panic` /
  `raid` / `pre_raid` / `hide` / `fight` / `celebrate` / `admire_item` / `avoid` / `ride`。
  目前唯一接入 Brain 的实体是村民，它注册了 **10 个**：
  时间线活动 `idle` / `work` / `meet` / `rest`（幼年另有 `play`），
  以及事件活动 `avoid` / `fight` / `hide` / `panic` / `raid`。
  **事件活动虽然注册了，但缺少上游记忆写入者**，所以实际很难被选中——
  例如 `fight` 依赖 `ATTACK_TARGET`，而全仓没有任何代码对村民设置攻击目标。
  给某个实体新增活动时，要同时确认「谁写入触发它的记忆」。
- **5 个预定义活动目前没有任何使用者**：`core` / `pre_raid` / `celebrate` /
  `admire_item` / `ride`。它们只在 `Activity.cpp` 里定义，全仓没有其他引用
  （既没有 `registerActivity`，也不在 `Schedule` 时间线里）。这属于为后续实体预留的
  类型，不是死代码——`Activity` 是值类型，未被引用的 `static` 成员不产生运行开销，
  但**不要误以为它们已经生效**。