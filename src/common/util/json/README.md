# 外部输入的 JSON 读取边界

nlohmann-json 在本项目里既是**值类型**也是**解析器**。它的问题在于错误用异常表达：
`json::parse` 抛 `parse_error`，`.get<T>()` / `.at()` 抛 `type_error` / `out_of_range`。
项目正在推进 `-fno-exceptions`（见 `docs/iterations/exception禁用指南.md`），异常一旦
出现在 header 或热路径上就是硬阻塞；即使不开该开关，nlohmann 抛出的异常从数据包加载
路径逃逸也会终止进程——一条坏 JSON 就能打死整服。

本目录提供的 `util::json` 是**外部输入的读取边界**，只做两件事：非抛出解析、非抛出取值。

## 目录结构

```
src/common/util/json/
├── JsonRead.hpp/cpp  # 非抛出解析与类型化取值（parse / getXxx / getXxxOr / findField / hasField）
└── README.md         # 本文档
```

## 规则：什么时候必须用 util::json

**必须**（外部不可信输入：数据包 JSON、存档文件、网络载荷里的 JSON 文本、命令里的 SNBT 转 JSON）：

- 解析：`util::json::parse(text)` / `util::json::parse(stream)` —— 不要用 `json::parse` / `operator>>` / `file >> json`
- 取值：`getInt32` / `getInt64` / `getFloat32` / `getFloat64` / `getBool` / `getString` /
  `getObject` / `getArray` —— 不要用 `.get<T>()` / `.at()`
- 可选字段带默认值：`getInt32Or` / `getInt64Or` / `getFloat32Or` / `getFloat64Or` /
  `getBoolOr` / `getStringOr` / `getObjectOrEmpty` —— 不要用 `json::value(key, default)`
- 可选字段无默认值：`findField`（返回 `const json*`）/ `hasField`，判空后使用

**不必**（这些不是"读取外部输入"，nlohmann 直接使用是正常且推荐的）：

- 构造输出用的 JSON（`nlohmann::json j; j["k"] = v;`）、`dump()` 序列化
- 遍历（`for (const auto& e : arr)`）、`contains()` / `is_object()` / `is_array()` / `size()`
- 对**自己刚构造出来**的 JSON 取值（类型由代码保证，不会失败）
- 与 NBT 系统互转（`nbt::jsonToNbt` / `nbt::nbtToJson`）

即：`util::json` 管「从不可信字节/文本到 JSON 值」和「从 JSON 值到 C++ 标量」这两步，
**不**接管 JSON 的内存表示与序列化。

## 现状与剩余工作

`util::json` 已覆盖全部 39 处 `catch(json::parse_error)` 所对应的解析点，以及 8 处
"解析失败回退纯文本"的 `catch(json::exception)`。业务代码中仍有大量直接 `nlohmann::json`
使用，其中**不合规**的部分是：

- 约 1117 处 `.get<T>()`：类型不符会抛 `type_error`（`-fno-exceptions` 下 abort）
- 约 19 处 `catch(json::exception)`：靠异常兜底（访问控制列表、记分板存档、皮肤缓存、
  LootSerializers、命令快照等）

这些点当前都有异常边界（不会崩），但都违反上面"必须"一条。迁移时按子系统推进，
不要一次性全改：每处改造都要顺带确认失败语义（是「跳过该条目」还是「整批失败」）。

## 容易踩的坑

### 1. `parse(text)` 的空输入

`nlohmann::json::parse("")` 在 `allow_exceptions=false` 下返回 discarded，本层据此返回
`InvalidData`。不要指望"空文本解析成 null"——需要可选语义时由调用方先判空。

### 2. `getObject` / `getArray` 返回引用

它们返回 `Result<const nlohmann::json&>`，引用指向**传入的 json**，不是副本。不要把它
存进比源 json 活得更久的变量里。

### 3. 浮点 getter 接受整数

数据包里 `1` 和 `1.0` 是两种 JSON 类型（`number_integer` / `number_float`），但业务上都是
"数字"。`getFloat32/getFloat64` 只校验 `is_number()`，与 nlohmann 的隐式转换语义一致。

### 4. 整数 getter 不接受浮点

`getInt32/getInt64` 只接受 `is_number_integer()`。`3.0` 会被判为类型不符——这是有意的：
数据包里本该是整数的字段写成浮点，通常是数据错误，应当报出来而不是静默截断。

### 5. `is_number_integer()` 拦不住越界

nlohmann 的 `number_integer` 是 **int64**、`number_unsigned` 是 **uint64**，
`json.get<i32>()` 在数值超出 int32 范围时会抛 `out_of_range`——`is_number_integer()`
只保证"是整数"，不保证"装得下"。同理 `get<f32>()` 在数值超出 f32 范围时会抛
`out_of_range`。这正是本层 `getInt32` / `getFloat32` 存在的理由：它们先按宽类型取，
再自行判定范围并返回 `Error`。

### 6. 非负整数字面量是 `number_unsigned`

`nlohmann::json::parse("42")` 得到的是 `number_unsigned` 而不是 `number_integer`。
因此**不要**用 `field_value->type() == value_t::number_integer` 这种方式判断"是不是整数"，
必须用 `is_number_integer()`（它对 `number_unsigned` 同样为真）。

### 7. 不要为单点需求加薄封装

`util::json` 已经是统一入口。若某处只是"少写一行判空"，直接用 `findField` + 判空即可，
不要再包一个只调用一次的函数（见项目规范「关于配置和传参」与薄封装禁令）。