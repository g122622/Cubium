# Cubium 去异常化盘点报告

> 范围：`src/**`（排除 `tests/`、`*.ts`、`*.md`）。已在 Windows + Clang 20 + MSVC STL 上实测 `-fno-exceptions` 的编译行为。

---

## 一、总量

| 指标                          | 数量                                                                                             |
| ----------------------------- | ------------------------------------------------------------------------------------------------ |
| 显式 `throw`                  | **219 处 / 45 个文件**（其中 **113 处在头文件**）                                                |
| `try` 块                      | 208 处                                                                                           |
| `catch` 子句                  | 244 处（`catch(...)` 50 处）                                                                     |
| 自定义异常类                  | 4 个（`CommandException`、`FunctionInstantiationException`、`TemplateError`、`AssertException`） |
| 现有断言 `MC_ASSERT_RELEASE*` | 1008 处（默认 handler 直接 `std::abort`）                                                        |

**关键结论：`throw` 只占工作量的一半。** 真正的硬阻塞是 `try/catch`、头文件中的 `throw`、标准库隐式抛出、以及 nlohmann/spdlog/asio 等第三方库。

---

## 二、按错误性质分类（这决定改造方向）

按你"发现异常立即崩溃、但不要静默吞掉"的原则，219 处 `throw` 应分三类处理，不能一刀切：

| 类别                       | 含义                                                         | 应改为什么                                      | 数量     |
| -------------------------- | ------------------------------------------------------------ | ----------------------------------------------- | -------- |
| **A. 内部不变量/编程错误** | 重复注册、重复设置回调、未 open 就访问、属性不存在、参数越界 | `MC_ASSERT_RELEASE`（立即 abort）               | **≈39**  |
| **B. 外部不可信输入**      | 玩家命令文本、存档 NBT、数据包 JSON、网络字节                | `Result`（显式失败，向上传播；绝不 abort 整服） | **≈174** |
| **C. Result 自身契约**     | `Result::value()` 在 failed 时 `throw`                       | `MC_ASSERT_RELEASE`（调用方违约 = 崩溃）        | **6**    |

> B 类不能崩溃：一条恶意 `/data` 命令或一个损坏存档让服务器 abort，等于拒绝服务。这与"暴露问题"不矛盾——A 类崩溃暴露 bug，B 类返回 Error 并带 warn 日志。

---

## 三、模块盘点

| 模块                     | throw   | 主要位置                                                                                                                                                  | 性质                                                  | 当前边界                                                                                                                   | 难度     | 可行性   | 建议                                           |
| ------------------------ | ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- | -------- | -------- | ---------------------------------------------- |
| **命令子系统**           | **144** | `StringReader.hpp` 14、`EntityArgument.cpp` 31、`GameModeArgument.hpp` 25、`NbtPath*` 19、`ArgumentType.hpp` 12、`server/command` 15、`server/function` 9 | B                                                     | `CommandDispatcher::parse/execute`、`CommandRegistry::execute` 已把异常收敛为 `ParseResults`/`Result<i32>`/`CommandResult` | **高**   | **中**   | 最后做；先统一边界                             |
| **NBT 库**               | **29**  | `Nbt.cpp` 20、`Nbt.hpp` 7、`NbtInternal.hpp` 2                                                                                                            | B                                                     | `NbtIo.cpp` 网络路径已 Result 化；**本地存档路径 4 处裸调用无兜底**                                                        | 中高     | 高       | 先补边界，再决定是否内部 Result 化             |
| **property 系统**        | 13      | `StateContainer` 6、`StateHolder` 5、`IntegerProperty` 2                                                                                                  | A                                                     | 无                                                                                                                         | **极低** | **极高** | 直接换 assert                                  |
| **Result 自身**          | 6       | `Result.hpp:313/321/329/515/523/531`                                                                                                                      | C                                                     | —                                                                                                                          | 低       | 高       | 改成断言（需全库审计 1897 处 `.value()`）      |
| **ServerWorld/维度回调** | 6       | `ServerWorld.hpp` 5、`ServerDimensionManager.hpp` 1                                                                                                       | A                                                     | 无                                                                                                                         | 极低     | 极高     | 直接换 assert                                  |
| **storage**              | 4       | `SingleLevelStorageManager.cpp` 1247-1281                                                                                                                 | A（"Storage not open"）+ B（SectionManager 创建失败） | 无                                                                                                                         | 低       | 高       | 前者 assert，后者 Result                       |
| **注册表**               | 4       | `BlockRegistry.hpp` 3、`ItemRegistry.hpp` 1                                                                                                               | A                                                     | 无                                                                                                                         | 极低     | 极高     | 直接换 assert                                  |
| **噪声/地形参数**        | 7       | `NoiseSettings.hpp` 5、`Noises.cpp` 1、`Column.hpp` 1                                                                                                     | A/B                                                   | `NoiseSettings::create` 生产代码**未使用**（仅测试）；数据驱动走 `DimensionSettings::fromJson` 已有 Result                 | 低       | 高       | 删死代码或改 assert                            |
| **server/function**      | 9       | `StringTemplate.cpp` 6、`MacroFunction.cpp` 3                                                                                                             | B                                                     | `MacroFunction::execute` 已 catch → warn + failureCount；`FunctionLoader` 已 catch                                         | 低-中    | 高       | 改 Result                                      |
| **杂项**                 | 5       | `PerlinNoiseSoA` bad_alloc、`NibbleArray`、`LootCallbackInvoker`、`RateLimiter`、`Assert`                                                                 | A                                                     | —                                                                                                                          | 极低     | 极高     | 前 4 个 assert；`RateLimiter` 全库零引用直接删 |
| **客户端**               | 1       | `TemplateError.hpp`                                                                                                                                       | —                                                     | —                                                                                                                          | —        | —        | 客户端已停维护，排除出构建                     |

### 各重点模块的改造面

**命令子系统（占 66%）**：`ArgumentType<T>::parse(StringReader&) -> T` 是纯虚接口，25+ 个参数类型实现；`StringReader` 32 个 API 中 14 个会抛；全库 180 处 `reader.readXxx()` 调用；80 个命令实现文件；42 个测试文件、68 个 `EXPECT_THROW`。工作量估计 **2–4 周**。且命令解析不在热路径（玩家输入/命令方块频率低），改造收益主要是"彻底禁异常"而非性能。

**NBT（13%）**：约 15 个递归 `read`/`read_content` 静态函数 + SNBT 模板 `load_text<T>`，3616 行。18 处裸调用点中 9 处有 try 兜底、**4 处完全无兜底**（见第六节）。

**property（6%）**：改 assert 不影响 1459 个 `with/get` 调用点——把 throw 换成断言即可，调用点零改动。这是性价比最高的一块。

---

## 四、`-fno-exceptions` 的真实阻塞清单（比 219 处 throw 更关键）

### 4.1 编译层

1. **头文件中的 113 处 `throw`**：任何包含这些头文件的 TU 在 `-fno-exceptions` 下都无法编译。**必须全部清空**（`Result.hpp` 被 684 个文件包含；`Nbt.hpp`、`StringReader.hpp` 等）。
2. **11 个头文件中的 `try/catch`**：`CommandDispatcher.hpp`、`CommandNode.hpp`、`CommandContext.hpp`、`IdDispatchCodec.hpp`、`Brain.hpp`、`DateTimeUtils.hpp` 等，同样在头文件内，必须清除。
3. **Windows 需额外加 `-D_HAS_EXCEPTIONS=0`**：实测 Clang 20 + MSVC STL 下，`-fno-exceptions` **不会**自动置该宏，不加会导致 STL 头文件中的 `throw` 编译失败。加宏后 STL 的 `optional::value()`、`any_cast`、`variant::get` 等会走 `abort`/`__fastfail`（**正好符合"立即崩溃"的偏好**）。
4. **PCH**：`pch_common.hpp` 必须与所有 TU 使用同一组异常选项重编。

### 4.2 标准库隐式异常源（不会出现在 `throw` 搜索结果里）

| API                                | 出现次数                 | `-fno-exceptions` 下行为                           | 处理建议                                                                |
| ---------------------------------- | ------------------------ | -------------------------------------------------- | ----------------------------------------------------------------------- |
| `std::optional::value()`           | ≈751（非 Result）        | MSVC STL 走 `_RAISE` → **abort/fastfail**          | 内部不变量可接受；外部输入路径改用 `value_or`/先判空                    |
| `.at()`                            | 160                      | 调 DLL 的 `_Xout_of_range`，**仍会抛** → terminate | 改用 `[]` + assert                                                      |
| `std::get<variant>`                | 163                      | MSVC STL → abort                                   | 先 `holds_alternative`/`get_if`                                         |
| `std::any_cast`                    | 34                       | MSVC STL → abort                                   | 改指针形式 `any_cast<T>(&)`                                             |
| `std::stoi/stol/stof` 等           | 58                       | DLL 内抛出 → terminate                             | **改用 `std::from_chars`**（解析种子/坐标/时间等外部输入，必须 Result） |
| `std::filesystem` 抛异常重载       | ≈187（85 已带 `ec`）     | DLL 内抛出 → terminate                             | 全部改 `error_code` 重载                                                |
| `std::regex`                       | 17                       | 构造失败抛 `regex_error`                           | 替换为手写校验（BanIp/PardonIp）                                        |
| `dynamic_cast<T&>`                 | 299                      | 失败 → `__cxa_bad_cast` → terminate                | 改指针形式 + 判空                                                       |
| `std::future/promise`              | 31                       | `future::get()` 会重抛任务异常                     | 任务内不抛即可；用 Result 传递                                          |
| nlohmann `json::parse`/`.get<T>()` | **434 个文件 / 1364 处** | 自动 `JSON_THROW → std::abort()`                   | 见 4.3                                                                  |

### 4.3 第三方库

| 库                                 | 使用面                                | 现状                                                                                   | 处理                                                                                                                           |
| ---------------------------------- | ------------------------------------- | -------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| **nlohmann-json**                  | 434 文件，69 个文件 `catch json::...` | `-fno-exceptions` 下自动 abort；坏 JSON = 崩服                                         | 数据包加载路径改用 `json::parse(s, nullptr, false)` + `is_discarded()`，或自定义 `JSON_THROW_USER`；**这是最大的一块外部改造** |
| **spdlog**                         | 425 文件                              | 编译库（`SPDLOG_COMPILED_LIB`），内部 `throw`                                          | vcpkg 重编时加 `SPDLOG_NO_EXCEPTIONS`，或切 header-only + 该宏                                                                 |
| **fmt**                            | 240 文件                              | 自动检测，无异常时 `FMT_THROW → assert_fail`（abort）                                  | 无需改动                                                                                                                       |
| **asio**                           | 62 文件                               | 需 `ASIO_NO_EXCEPTIONS` **且必须自定义 `asio::detail::throw_exception`**，否则链接失败 | 提供 abort 版本；建议把 asio 调用隔离到单独 TU                                                                                 |
| **simdjson**                       | 141 处                                | 自动 `SIMDJSON_EXCEPTIONS=0`；代码里有 2 处 `catch simdjson_error` 需删                | 改用错误码 API                                                                                                                 |
| **RocksDB**                        | 178 文件                              | 预编译库带异常；头文件 `rocksdb/multi_scan.h` 含 `throw`（若被 include 会编译失败）    | 库内异常跨边界 → terminate（可接受）；避免 include 该头                                                                        |
| **gtest**                          | tests                                 | `GTEST_HAS_EXCEPTIONS=0` 时 `EXPECT_THROW` 宏体仍含 `try/catch` → **测试编译失败**     | 测试 target 保留异常；但**头文件必须清干净**，否则测试 TU 也会带到 throw                                                       |
| **Perfetto/Tracy/OffsetAllocator** | 内置源码                              | 实际编译路径无 `throw`                                                                 | 无需改动                                                                                                                       |
| **ftxui/openal/glfw**              | 客户端                                | 客户端已停维护                                                                         | 排除                                                                                                                           |

> 测试中还有 1370 处 `throw` 和 119 个 `EXPECT_THROW`。若全局 `-fno-exceptions`，测试必须改用死亡测试（`EXPECT_DEATH`）或改为验证 `Result` 失败。建议生产 target 禁异常、测试 target 保留异常（按 target 设置编译选项），但**共享头文件仍必须无 `throw`**。

---

## 五、静默吞异常清单（按你的新要求必须处理）

当前 `catch(...)` 50 处，其中真正"无日志、返回默认值/继续"的有：

| 位置                                        | 行为                                                       | 问题                                                              |
| ------------------------------------------- | ---------------------------------------------------------- | ----------------------------------------------------------------- |
| `CommandContext.hpp:170`                    | `getArgumentOr` catch(...) → defaultValue                  | 掩盖 `bad_any_cast` 类型错误                                      |
| `GameRules.cpp:722`                         | `_getIntFromNbt` catch(...) → defaultValue                 | 坏存档静默使用默认规则                                            |
| `LanguageManager.cpp:307`                   | catch(...) → 保留原文                                      | 格式化错误被吞                                                    |
| `JavaWorldReader.cpp:159`                   | catch(...) → `continue`                                    | **区域文件读取失败静默跳过整个区块**                              |
| `MinecraftModuleFactory.cpp:2953`           | catch(...) → 降级字符串                                    | 基岩包解析错误被吞                                                |
| `LocationPredicate.cpp:83`                  | catch(...) → `false`                                       | 维度名解析失败静默不匹配                                          |
| `WorldNameSanitizer.cpp:160`                | catch(...) → `false`                                       | —                                                                 |
| `ServerSettings.cpp:146`                    | catch(...) → 哈希字符串                                    | 种子解析失败静默换算法                                            |
| `DataAccessor.cpp:365`                      | `CommandStorage::load` catch(...) → 跳过                   | **命令存储损坏条目静默丢弃**                                      |
| `CommandDispatcher.hpp:806`                 | 空 catch（建议候选试探）                                   | 属正常控制流，建议加注释并只捕获 `CommandException`               |
| `StandaloneServer.cpp:483`                  | 主循环 catch std::exception → critical + `m_running=false` | **服务器"悄悄停止"而不是崩溃退出**                                |
| `UniversalWorkerPool` / `AreaTaskScheduler` | catch → error 日志后继续                                   | 线程边界兜底；按新偏好建议改 `critical + abort`（任务内异常=bug） |

另外，`BaseApplicationEntry.cpp:246` 捕获后返回 1（走正常清理退出），不是 abort。若要求"立即崩溃"，应改为打印后 `std::abort()`。

---

## 六、现存最优先缺陷：异常从 `Result` 函数逃逸

这 4 处函数签名返回 `Result<...>`，但内部**直接调用会抛异常的 NBT 读取且无 `try`**。当前一个损坏存档就能让异常越过 Result 边界，最终由顶层 catch 处理（服务器退出/线程终止）；在 `-fno-exceptions` 下会直接编译失败：

1. `SaveFormatDetector::detect` — `SaveFormat.cpp:115/154`（整个函数 0 处 try）
2. `LevelDatCodec::_readGzipNbt` — `LevelDatCodec.cpp:142`
3. `JavaAnvilBackend::loadPlayer` — `JavaAnvilBackend.cpp:183`（全文件无 try/catch）
4. `ScoreboardSaveData::deserializeNbtFromBytes` — `ScoreboardSaveData.cpp:731`

已有正确兜底的对照：`PlayerSaveData.cpp:763`、`EntityDeserializer.cpp:196`、`BlockEntityDeserializer.cpp:248`、`ScoreboardDataManager.cpp:108`（均有 try→Result）。

---

## 七、执行进度（滚动更新）

| 阶段 | 状态 | 落地内容 |
| --- | --- | --- |
| 0. 低风险断言化 | ✅ 完成 | property/ServerWorld/维度/storage/registry/噪声/杂项共约 40 处改断言；删除 `RateLimiter`、`loot::registryOf`、storage 的 const `_sectionManager`/`_hasSectionManager` 等死代码 |
| 1. Result 契约 | ✅ 完成 | `Result.hpp` 6 处 `value()` 改断言（消息带原始错误）；审计全库 try 块内 `.value()`，确认无调用点依赖异常 |
| 2 + 3. NBT | ✅ 完成（合并实施） | 直接对 NBT 库 Result 化，未采用"薄封装"过渡：`NbtInternal`/`Nbt.hpp`/`Nbt.cpp` 内 `throw` 归零，`operator>>(compound_tag&)` 改为 `readCompoundInto`，`parseMojangson` 返回 `Result`；22 处读取调用点全部改为检查 `failed()` |
| 4. 函数/宏 | ✅ 完成 | `StringTemplate`/`MacroFunction` 改 `Result`，删除 `FunctionInstantiationException` |
| 5. 隐式异常源 | 🟡 部分完成 | `std::sto*`（18 文件）全部换为 `util::parseXxx`（from_chars）；`std::regex` 全部移除（改逐字符判定 + `util::isValidIpv4`）；`std::any_cast` 全部改指针形式。**未做**：`.at()`（libc++ 在 `-fno-exceptions` 下已退化为 abort，不阻塞编译）、`filesystem` 抛异常重载、`dynamic_cast<T&>`、`std::future` |
| 6. 第三方隔离 | 🟡 部分完成 | 新增 `util::json` 非抛出访问层；39 处 `catch(json::parse_error)` 全部消除（loader 循环、整函数 try、文件流解析、组件 JSON 共 39 处）；地图/旗帜/变体注册表等 8 处`catch(json::exception)` 改为解析失败回退。**未做**：其余 `.get<T>()`（约 1112 处）与 19 处`catch(json::exception)`（访问控制列表、记分板存档、皮肤缓存、LootSerializers 等），这些点都有异常边界（不会崩），但 `-fno-exceptions` 下会 abort；spdlog/asio/simdjson 配置未动 |
| 7. 开关就绪 | 🟡 部分完成 | 已加 `MC_ENABLE_NO_EXCEPTIONS` 选项（只作用于生产 target，测试保留异常）；`AssertException`/`throwAssertHandler` 已按 `__cpp_exceptions` 条件编译。**未做**：清空头文件剩余 throw（几乎全在命令子系统） |
| 8. 命令子系统 | ⬜ 未开始 | 剩余头文件 `throw` 约 66 处全部集中在这里（`GameModeArgument` 24、`StringReader` 14、`ArgumentType` 12 等） |

### 阶段 8 待决的设计问题

命令子系统不能用现有 `Error` 直接替代 `CommandException`：解析错误需要携带
`CommandErrorType`（决定翻译键/客户端提示）与**游标位置**（客户端高亮出错处），
而 `Error` 目前只有 `code`/`message`/`source` 三个字符串字段。三种候选：

1. 给 `mc::Error` 增加可选的 `i32 offset` 与错误类别字段（全局影响 `Result`，但保持单一错误类型）；
2. 为命令子系统引入专用错误类型并配 `Result` 特化（与"统一 Result/Error 体系"冲突）；
3. 保持 `CommandException` 作为**值类型**（不再 throw），用 `Result<T>` + 该类型做 payload。

需先定方案，再动约 180 处调用点与 42 个测试文件。

---

## 八、建议路线图（原始估算）

| 阶段                        | 内容                                                                                                                                | 规模                   | 预估       |
| --------------------------- | ----------------------------------------------------------------------------------------------------------------------------------- | ---------------------- | ---------- |
| **0. 低风险断言化**         | property 13、ServerWorld/维度 6、storage 4、registry 4、NoiseSettings 5、Column/NibbleArray/Perlin/Loot 4；删除死代码 `RateLimiter` | 约 40 处，调用点零改动 | 1–2 天     |
| **1. Result 契约**          | `Result.hpp` 6 处 `throw` → 断言；扫描 1897 处 `.value()`，确认没有依赖可捕获异常                                                   | 1 文件 + 审计          | 1 天       |
| **2. NBT 边界补齐**         | 修第六节 4 个逃逸点；所有 NBT 读取统一走 `Result` 包装（扩展现有 `NbtIo`）                                                          | ~10 处                 | 1–2 天     |
| **3. NBT 库内部 Result 化** | 29 处 throw、15 个递归 read 函数                                                                                                    | 3.6K 行                | 3–5 天     |
| **4. 函数/宏 + 杂项**       | `StringTemplate`/`MacroFunction` 9 处                                                                                               | 小                     | 1–2 天     |
| **5. 隐式异常源清理**       | `stoi→from_chars`、`.at()→[]`、`filesystem→ec`、`regex`、`any_cast`、`dynamic_cast&`                                                | 分散 ~900 处           | 3–7 天     |
| **6. 第三方隔离**           | nlohmann 非抛出解析、spdlog/asio 配置、RocksDB 边界、测试 target 策略                                                               | 大                     | 3–5 天     |
| **7. 头文件清空 + 开关**    | 清除 113 处头文件 throw、11 个 try/catch；加 `-fno-exceptions -D_HAS_EXCEPTIONS=0`；PCH 重编；分 target 灰度                        | —                      | 2–3 天     |
| **8. 命令子系统**           | 144 处、25+ 参数类型、80 命令、42 测试                                                                                              | 47.5K 行               | **2–4 周** |

**推荐的核心策略**：
- 对 A 类（内部不变量）统一用 `MC_ASSERT_RELEASE`——默认 handler 已 `abort`，符合"立即崩溃暴露问题"。
- 对 B 类（外部输入）统一用 `Result`，禁止用 `catch(...){}` 降级；必须 warn/error 日志 + 向上传播。
- 第三方库用"异常边界 TU"隔离：把 nlohmann/asio/RocksDB 调用收进少数几个单独编译（保留异常）的 `.cpp`，对外只暴露 `Result`。这样 `-fno-exceptions` 可以覆盖绝大多数 TU。
- 命令子系统放最后，因为它的异常已经是"子系统内局部控制流"，边界完整；过早改造会阻塞其他模块。

---

## 九、结论

- **技术上完全可行**，但 `-fno-exceptions` 的难点不在 219 处 `throw`，而在 **113 处头文件 throw + 208 个 try 块 + 4 个第三方库 + 约 900 处隐式抛异常的标准库 API**。
- **性价比最高的第一批**是阶段 0+1+2（约 50 处改动，2–4 天），能立刻消除"编程错误静默/异常逃逸"问题，并为后续铺路。
- **命令子系统占 66% 的 throw，但只应作为最后一步**；它的正确形态是 `Result`（玩家输入错误不是崩溃理由），而不是 assert。
- 若目标只是"不再有未捕获异常导致进程异常退出"，做完阶段 0–2 即可获得 80% 收益；若目标是真的 `-fno-exceptions` 全量编译，则必须完成阶段 3–8，总量约 **5–8 周**（命令子系统是大头）。

如果你确认，我建议立即开始阶段 0（约 40 处断言化 + 删除 `RateLimiter` 死代码），这是零风险、纯收敛的改动。