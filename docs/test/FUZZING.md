# 协议模糊测试（tests/fuzz）

以 libFuzzer harness 对网络协议解析链路做覆盖率引导的模糊测试。目标是在**不需要真实
socket、不需要世界、不需要存档**的前提下，把"入站字节 → IR 包"的解码链路跑满，从而
发现畸形报文导致的内存安全问题与协议处理缺陷。

---

## 1. 为什么需要这一层

仓库原有三类测试各自的覆盖缺口：

| 测试 | 覆盖范围 | 明确**不**覆盖 |
|---|---|---|
| `tests/unit`（GoogleTest） | 逻辑正确性；网络走 `LocalTransport` 直传 IR 包**对象** | 不经 codec、不见畸形字节 |
| `tests/integrated`（基岩 GameTest） | 进程内 QuickJS 行为测试 | 无玩家、不联网 |
| `tests/e2e/bot`（mineflayer） | **合法**客户端的字节 → codec → 业务全链路 | 只发合法报文，不探索畸形输入 |
| **`tests/fuzz`** | **任意畸形字节**下的解码链路 | 不覆盖业务逻辑（见 §7 的路线图） |

单测验证"合法输入得到正确结果"，fuzz 验证"任意输入不会崩、不会爆内存"。二者互补：
本轮 fuzz 首次运行即发现 2 处单测与 e2e 都不可能发现的远程内存耗尽缺陷（见 §6）。

---

## 2. 目标清单

| 目标 | 覆盖对象 | 输入语义 |
|---|---|---|
| `fuzz_varint_framing` | `pipeline/VarintFraming`（VarInt21 长度前缀切帧） | 整段字节当作一条 TCP 流，按 1/7/全量三种分块粒度喂入 |
| `fuzz_compression` | `pipeline/CompressionHandlers` → `crypto/ZlibCodec` | 第 1 字节 = 阈值（-1 表示禁用），其余为压缩层字节 |
| `fuzz_cipher` | `pipeline/CipherHandlers` → `crypto/AesCfb8` | 前 `kSharedSecretBytes` 字节 = 密钥，其余为待处理字节 |
| `fuzz_java_codec` | Java 1.21.11 五阶段 × 两流向全部包表 | 第 1 字节选阶段，第 2 字节选流向，其余为 `packetID + payload` |
| `fuzz_java_codec_sb` | 同上，**仅 Serverbound**（服务端解码不可信客户端输入） | 同左，流向强制 Serverbound |
| `fuzz_java_codec_cb` | 同上，**仅 Clientbound**（客户端解码服务端输入） | 同左，流向强制 Clientbound |

**为什么把 sb/cb 拆成独立目标**：libFuzzer 一遇 OOM/崩溃即中止整个运行。Clientbound
方向已存在会触发 OOM 的缺陷，若与 Serverbound 混在同一目标内，前者会持续遮蔽后者的
探索。拆开后每个方向各自独立收敛，语料也互不污染。

`fuzz_java_codec_sb` 是**安全上最要紧**的目标：它对应"任意 Java 客户端连上服务端后
发送畸形包"这一真实攻击面。

---

## 3. 构建

### 3.1 前置条件

- Windows + 项目默认 clang 工具链（VS 自带 LLVM，本机为 clang 20.1.8）。
- **必须**经 `scripts/configure.{bat,sh,ps1}` 注入 Visual Studio 开发环境后再构建。
  否则 ASan 运行时所需的 Windows SDK 默认库（`dbghelp`/`ole32`/`shell32` …）不可见，
  链接期报 `LNK2019: unresolved external symbol __imp_SymLoadModuleEx` 一类错误。
- libFuzzer 运行时需以**本项目一致的 CRT** 自行构建，因此需要一份 compiler-rt 源码；
  路径由 preset 的 `MC_FUZZ_LIBFUZZER_SOURCE_DIR` 指定（见 §7.1）。

### 3.2 构建命令

```bash
# 配置（复用 build/ 目录，不新建构建树；fuzz 目标全部 EXCLUDE_FROM_ALL，
# 因此不会影响日常 `./scripts/configure.sh build`）
./scripts/configure.sh --preset windows-clang-fuzz

# 构建全部 fuzz 目标（preset 内已限定 targets，不会触发整项目构建）
cmake --build --preset windows-clang-fuzz

# 只构建单个目标
cmake --build --preset windows-clang-fuzz --target fuzz_java_codec_sb
```

产物位于 `build/bin/fuzz/RelWithDebInfo/`。构建脚本会自动部署 vcpkg 依赖 DLL 与
`clang_rt.asan_dynamic-x86_64.dll`。

### 3.3 相关 CMake 开关

| 开关 | 默认 | 作用 |
|---|---|---|
| `MC_BUILD_FUZZERS` | OFF | 进入 `tests/fuzz`。**默认关闭**，避免影响常规构建 |
| `MC_FUZZ_ASAN` | ON | 给插桩 TU 施加 ASan/UBSan；关闭后仅保留覆盖率插桩 |
| `MC_FUZZ_LIBFUZZER_SOURCE_DIR` | 空 | `compiler-rt/lib/fuzzer` 源码目录；Windows 上必需（见 §7.1） |

### 3.4 增量构建

fuzz 目标的插桩静态库 `mc_fuzz_net` 只含网络子系统 TU（约 28 个），与之无关的项目
代码不会被重编。改动 `tests/fuzz/` 下的 harness 只需重链，通常在分钟级内完成。

---

## 4. 运行

```bash
cd build/bin/fuzz/RelWithDebInfo

# 基本运行（无限跑，Ctrl+C 停止）
./fuzz_java_codec_sb.exe corpus/fuzz_java_codec_sb

# 限定迭代数（CI / 快速回归常用）
./fuzz_java_codec_sb.exe -runs=1000000 -max_len=512 corpus/fuzz_java_codec_sb

# 复现单个输入（不 fuzz，只执行）
./fuzz_java_codec_sb.exe -runs=1 path/to/input

# 关闭 libFuzzer 的分配上限，让 ASan 直接报"requested allocation size …"
# （用于在 OOM 场景下拿到 ASan 的定位信息而非 libFuzzer 的 malloc 上限中止）
./fuzz_java_codec_sb.exe -runs=1 -malloc_limit_mb=0 -rss_limit_mb=0 path/to/input

# 覆盖率报告
./fuzz_java_codec_sb.exe -runs=0 -print_coverage=1 corpus/fuzz_java_codec_sb
```

**常用 flag 说明**

| flag | 说明 |
|---|---|
| `-runs=N` | 迭代 N 次后退出；`-runs=1 <file>` 为复现模式 |
| `-max_len=N` | 单输入长度上限。默认 4096；解码类目标用 256~512 已足够 |
| `-malloc_limit_mb` | **单次**分配上限，缺省回退为 `-rss_limit_mb`（默认 2048MB）。这是本项目
发现 OOM 类缺陷的主要手段 |
| `-rss_limit_mb` | 进程 RSS 上限 |
| `-print_final_stats=1` | 退出时打印统计 |
| `-artifact_prefix=DIR/` | 崩溃/超限产物落盘目录，默认为当前目录 |

**产物命名**：`oom-<sha1>`（分配超限）、`crash-<sha1>`（崩溃）、`timeout-<sha1>`（超时）。
它们就是可直接复现的最小输入，应移入 `corpus/` 作为回归种子。

**实测吞吐**（本机 clang 20.1.8 + ASan，单核）：帧层 6k~12k exec/s，Java codec 约 25k exec/s。

---

## 5. 目录结构

```
tests/fuzz/
├── CMakeLists.txt              # 插桩库 + 目标定义（受 MC_BUILD_FUZZERS 控制）
├── support/
│   ├── FuzzSupport.hpp/cpp     # 进程级一次性初始化（注册表 + 五阶段包表）
├── targets/
│   ├── FuzzVarintFraming.cpp   # 帧层
│   ├── FuzzCompression.cpp     # 压缩层
│   ├── FuzzCipher.cpp          # 加密层
│   └── FuzzJavaCodec.cpp       # Java 全阶段 codec（经 MC_FUZZ_FLOW_MODE 生成 sb/cb/两者）
└── corpus/                     # 回归种子（崩溃/超限产物移入此处）
    ├── fuzz_java_codec_cb/
    └── fuzz_java_codec_sb/
```

---

## 6. 插桩策略：局部插桩

`tests/fuzz/CMakeLists.txt` 把网络子系统的解码链路 TU **再编译一份**为插桩静态库
`mc_fuzz_net`（SanitizerCoverage + ASan/UBSan），其余符号（方块/物品注册表、NBT、
世界数据、脚本系统）从已编译的 `mc_common` 惰性拉取。

**为什么不用整库插桩**：本项目代码量百万级，整库带插桩重编代价极高；而解析逻辑高度
集中在 `network/` 子树内，局部插桩即可获得该子树的完整覆盖率反馈。

**三个关键设计点**

1. **用 STATIC 而非 OBJECT 库承载插桩 TU**。静态库成员按未解析符号惰性拉取，于是
   (a) 插桩副本的符号优先于 `mc_common` 内的未插桩同名副本被选中（插桩库在链接行更靠前）；
   (b) 未被 harness 引用的 TU 不进入链接，不产生开销。
   OBJECT 库会把全部 `.obj` **无条件**塞进链接，既拖入无关符号又放大重复符号风险。
2. **生成的查找表不重复编译**。`backend/java/generated/*.gen.cpp` 是 `src/common`
   目录作用域 `add_custom_command` 的产物，跨目录引用会引入构建顺序问题；且它们只是
   静态查找表、无解析逻辑，插桩价值为零。改由 `mc_common` 提供，并用
   `add_dependencies(mc_fuzz_net mc_common)` 保证烘焙步骤先完成。
3. **fuzz 目标一律 `EXCLUDE_FROM_ALL`**，不进入 `all`，因此 `./scripts/configure.sh build`
   的日常构建完全不受影响。

---

## 7. 已确认缺陷（本轮 fuzz 首次产出）

> 两处均为 `reserve()` 直接使用线上读到的计数、且只校验非负、无上界。
> 其共同点是：**畸形报文的字节数很少，却能命令对端一次性申请数 GB 内存**。

### 7.1 Serverbound：`ContainerClick` 可远程打崩服务端（高）

- **位置**：`src/common/network/backend/java/codecs/JavaPlayCodecs.hpp:149-167`
  （`play_detail::readHashedStack`），由 `containerClickCodec`（同文件 :567）调用。
- **触发**：`HashedPatchMap.addedCount` / `removedCount` 经 VarInt 从线上读入，只判
  `count < 0`，随后 `v.addedHashes.reserve(addedCount)`。
- **实测**：27 字节输入 → `malloc(3070230559)`（3.07 GB）。
- **复现种子**：`corpus/fuzz_java_codec_sb/oom-container-click-hashed-stack.bin`。
- **影响**：`container_click` 是 Serverbound 包，由**服务端**解码不可信客户端输入。
  攻击者连上服务端、完成登录进入 Play 阶段后即可触发，属远程拒绝服务。

### 7.2 Clientbound：`MapItemData` 可让客户端爆内存（中）

- **位置**：`src/common/network/backend/java/codecs/JavaPlayCodecsExtended.hpp:1039`
  （`mapItemDataCodec` 的 decode lambda）。
- **触发**：`decorations` 列表的 `count` 只判 `count < 0`，随后 `decos.reserve(count)`。
- **实测**：14 字节输入 → `malloc(21474836479)`（21.4 GB）。
- **复现种子**：`corpus/fuzz_java_codec_cb/oom-map-decorations-huge-reserve.bin`。
- **影响**：`map_item_data` 是 Clientbound 包，由**客户端**解码服务端输入。当前客户端
  已停止维护，故实际暴露面限于"恶意服务端攻击我方客户端"。

### 7.3 系统性模式（静态扫描结果，尚未逐一实测）

对 `backend/java/codecs/` 下"线上计数直接驱动 `reserve`"的模式做静态扫描，共 15 处。
守护程度**不一致**，这正说明人工审查容易漏、而覆盖率引导的 fuzz 能抓：

| 位置 | 现有校验 | 判定 |
|---|---|---|
| `JavaPlayCodecs.hpp:1100`（palette size） | `size < 0 \|\| size > 4096` → 拒绝 | ✅ 正确范式 |
| `JavaPlayCodecs.hpp:1826/1827`（UpdateAttributes） | 仅 `MC_ASSERT_RELEASE(count >= 0)` | ❌ 无上界 |
| `JavaPlayCodecs.hpp:1834/1835`（modifiers） | 仅 `MC_ASSERT_RELEASE(count >= 0)` | ❌ 无上界 |
| `JavaPlayCodecsExtended.hpp:1421` | 待核 | ⚠ |
| `JavaPlayCodecs.hpp:1119 / 1193 / 1205 / 1247 / 1271 / 1284 / 1347 / 1363 / 1981 / 2029` | 待核 | ⚠ |

**修复方向**：每个元素在线上至少占 1 字节，因此**任何计数的合法上界都不可能超过
`buf.readableBytes()`**。在 `reserve` 前统一加此校验（或按字段给显式上限，如 palette
的 4096），即可系统性消除该模式。`JavaPlayCodecs.hpp:1100` 是应予推广的现有范式。

---

## 8. 容易踩的坑

> 本节是本文档最值得优先阅读的部分。以下每一条都是实际踩过的。

### 8.1 libFuzzer 官方预编译运行时是 /MT，与本项目 /MD 硬冲突

clang 自带的 `clang_rt.fuzzer-x86_64.lib` 以**静态 CRT** 构建，其 `.drectve` 段含：

```
/FAILIFMISMATCH:"RuntimeLibrary=MT_StaticRelease" /DEFAULTLIB:libcmt.lib
```

而本项目（含 vcpkg `x64-windows` 依赖链）全量使用**动态 CRT（/MD）**，链接期硬错误：

```
lld-link: error: /failifmismatch: mismatch detected for 'RuntimeLibrary':
>>> clang_rt.fuzzer-x86_64.lib(FuzzerCrossOver.cpp.obj) has value MT_StaticRelease
>>> .../BiomeLoader.cpp.obj has value MD_DynamicRelease
```

注意：**ASan/UBSan 的运行时没有该指令，与 /MD 兼容**——只有 libFuzzer 运行时受影响
（可用 `llvm-readobj --coff-directives <lib>` 核对）。

**解法**：用本项目一致的 CRT（/MD）把 libFuzzer 运行时从 compiler-rt 源码重编一遍
（25 个源文件，约 1 分钟），源目录由 `MC_FUZZ_LIBFUZZER_SOURCE_DIR` 指定。官方配方见
`compiler-rt/lib/fuzzer/CMakeLists.txt`：`clang_rt.fuzzer = LIBFUZZER_SOURCES +
FuzzerMain.cpp`；`FuzzerInterceptors.cpp` 属**独立**运行时，不纳入。各平台专属源文件
自带 `#if LIBFUZZER_*` 守卫，可整体编译。

### 8.2 ASan + MSVC STL 的容器注解会造成 failifmismatch

只插桩部分 TU 时，带 ASan 的 TU 与不带 ASan 的 TU 对 MSVC STL 的
`<string>`/`<vector>`/`<optional>` 发出不同的 `/FAILIFMISMATCH:"annotate_*=0|1"`：

```
lld-link: error: /failifmismatch: mismatch detected for 'annotate_string'
lld-link: error: /failifmismatch: mismatch detected for 'annotate_optional'
```

**解法**：给插桩 TU 定义总开关宏 `_DISABLE_STL_ANNOTATION`（会自动定义
`_DISABLE_STRING_ANNOTATION` / `_DISABLE_VECTOR_ANNOTATION` /
`_DISABLE_OPTIONAL_ANNOTATION`）。宏定义见 MSVC STL 内部头
`<__msvc_sanitizer_annotate_container.hpp>`；上游同样的处理见
`compiler-rt/test/fuzzer/lit.cfg.py:88-95`（那里只禁了 string+vector，新版 STL 增加
optional 后需用总开关）。代价：失去插桩 TU 内 STL 容器的 ASan container-overflow 检查。

### 8.3 OBJECT 库会无条件塞入全部对象，别拿它当依赖

`mc_gen` / `mc_server_storage` / `mc_bedrock_addon` 中前两者是 **OBJECT** 库。把它们
加进 fuzz 目标会无条件拖入世界生成、存档乃至只存在于 `minecraft-server` 可执行文件里的
符号，表现为：

```
lld-link: error: undefined symbol: mc::scoreboard::ScoreboardDataManager::~ScoreboardDataManager(void)
```

**解法**：只链接 STATIC 库（按未解析符号惰性拉取）。`fuzz_java_codec*` 确实需要
`mc_bedrock_addon`（`mc_common` 的 `Entity.cpp` 引用基岩 Addon 的
`ScriptHandleRegistry`/`BlockComponentRegistry`），而它是 STATIC，可以安全链接——
用 `EXTRA_LIBS` 逐目标显式声明。

### 8.4 不要给 fuzz 目标安装 `CrashHandler`

`mc::assert::CrashHandler` 看上去能提供更好的崩溃栈，但它会**丢失复现用例**：

| 路径 | 未装 | 装了 |
|---|---|---|
| `MC_ASSERT_RELEASE` 失败 | `Assert.cpp:199` → `std::abort()` → ASan 死回调 → libFuzzer 保存用例 | 同左（不受影响） |
| `std::terminate`（未捕获异常 / `bad_alloc`） | CRT 默认 → `abort()` → ASan → **保存用例** | `CrashHandler.cpp:932` → **`_exit(1)`** → 死回调不触发 → **丢失用例** |

而符号化栈本来就已经齐备：断言路径经 `AssertManager::captureStackTrace()` →
`CrashHandler::captureStackTrace(0)`（`Assert.cpp:181`，默认开启，可用 `MC_ASSERT_NO_STACK`
关闭）；崩溃路径由 ASan 的 VEH 先手（优先级高于 `SetUnhandledExceptionFilter`），
并由 libFuzzer 的 `PrintStackTrace()` 输出完整符号化栈。

### 8.5 崩溃栈其实一直都有——别把输出过滤掉

libFuzzer 的 `HandleMalloc`（`FuzzerLoop.cpp:125-136`）在 `DumpCurrentUnit` **之前**就调用
`PrintStackTrace()`。用 `Select-Object -Last N` 或按关键字过滤输出时极易把这段栈丢掉，
误判为"看不到调用栈"。抓现场时应把完整输出落盘再检索。

### 8.6 `-malloc_limit_mb` 缺省会回退为 `rss_limit_mb`

见 `FuzzerDriver.cpp:718-720`：`malloc_limit_mb` 未设置时取 `rss_limit_mb`（默认 2048MB）。
即**单次分配 ≥2GB 就会中止**。这既是发现 OOM 缺陷的主力手段，也意味着
`-rss_limit_mb=0` 会连同该检查一起关掉（复现时想看 ASan 的诊断信息，应显式设
`-malloc_limit_mb=0 -rss_limit_mb=0`，但那样就不再复现 OOM 中止本身）。

---

## 9. 排查手册

| 现象 | 可能原因 | 排查方向 |
|---|---|---|
| 链接报 `/failifmismatch ... RuntimeLibrary` | libFuzzer 官方运行时是 /MT | 设 `MC_FUZZ_LIBFUZZER_SOURCE_DIR` 走源码自建（§8.1） |
| 链接报 `/failifmismatch ... annotate_*` | ASan 与 MSVC STL 注解不一致 | 确认 `_DISABLE_STL_ANNOTATION` 已加（§8.2） |
| 链接报 `LNK2019 __imp_SymLoadModuleEx` | 未注入 VS 开发环境 | 用 `scripts/configure.*` 而非裸 `cmake`（§3.1） |
| 链接报 `undefined symbol`（服务端类） | 误链了 OBJECT 库 | 改用 STATIC 库（§8.3） |
| `ERROR: libFuzzer: out-of-memory` | 线上计数驱动了超大分配 | 按栈回溯定位 `reserve`，加 `readableBytes()` 上界（§7） |
| 复现时不再触发 | 缺陷依赖累积状态，或限流开关被关 | 见 §8.6；必要时改用 `-runs=1` 多次执行 |
| 覆盖率长时间不增长 | 缺少种子；或该方向无表（如 Java 握手无 Clientbound 表） | 先跑 `tests/e2e/bot` 生成种子；确认目标方向（§2） |
| 运行即报找不到 DLL | ASan 运行时/vcpkg DLL 未部署 | 确认 POST_BUILD 的 DLL 复制步骤已执行（§3.2） |

---

## 10. 后续路线

| 阶段 | 内容 | 状态 |
|---|---|---|
| A | 分层解码器 harness（帧化/压缩/加密/Java codec） | ✅ 已落地 |
| A+ | 区块线格式 harness（`VanillaChunkWire` / `ChunkSerializer`） | 待做 |
| A+ | 种子生成器（由 `tests/e2e/bot` 的 `bot-trace-*.jsonl` 反编码生成结构化种子） | 待做 |
| B | 整条入站流水线 harness（`Connection` + 假 `ITransport`，覆盖粘包/半包与阶段切换） | 待做 |
| C | 进程内状态机会话 harness（消息序列变异 + 状态反馈，含 `MinecraftServer` 无头骨架以覆盖 Play 业务层） | 待做 |
| D | WSL2/Linux 上基于 AFLnet 的真实网络 fuzz | 待做 |
| E | Cubium vs vanilla 差分对撞（抓"不崩溃但语义错误"的缺陷） | 待做 |
| F | 修复 §7 的缺陷并补回归用例 | 待批准 |
