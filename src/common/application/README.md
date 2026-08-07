# Common Application 模块

客户端/服务端共享的进程入口抽象与异步日志基础设施。

## 目录结构树

```
src/common/application/
├── BaseApplicationEntry.hpp   # 进程入口抽象基类（模板方法 run()）
├── BaseApplicationEntry.cpp   # 公共启动流程实现 + banner/buildInfo + 公共 gflags flag(--verbose)
├── LogManager.hpp             # 异步日志管理器单例声明
├── LogManager.cpp             # spdlog async_logger(overrun_oldest) + 溢出监控线程
└── README.md
```

## 内部模块关系

```
BaseApplicationEntry（模板方法基类，mc::application 命名空间）
    ^                          ^
    | 继承                      | 继承
    |                          |
ServerApplicationEntry      ClientApplicationEntry（分别在 server/client application 目录）
    |                          |
    | 持有                      | 持有
    v                          v
StandaloneServer           ClientApplication
```

- `BaseApplicationEntry::run(argc, argv)` 是模板方法，依次执行：CrashHandler install → LogManager initialize → gflags 解析 → onFlagsParsed → banner/buildInfo → prepareRun → runApplication（try/catch 统一清理）。差异点下沉为 protected 虚函数 hook。
- `LogManager` 是 Meyers 单例，`initialize()` 建异步 logger + 溢出监控线程，`shutdown()` 停监控线程 + flush 队列。须在 `ProfilerManager::shutdown()` 之前 shutdown。

## 上下游外部依赖关系

**本模块依赖：**
- `common/util/assert/CrashHandler` - 崩溃处理器 install/setCleanupCallback
- `common/profiler/ProfilerManager` - 错误/崩溃路径 stop+shutdown Perfetto
- `minecraft-reborn/version.h` - banner/buildInfo 消费的版本/Git/构建宏
- 第三方 `spdlog`（异步 logger）、`gflags`（命令行解析）

**被依赖方：**
- `server/application/ServerApplicationEntry` 继承 `BaseApplicationEntry`
- `client/application/ClientApplicationEntry` 继承 `BaseApplicationEntry`
- `server/main.cpp` / `client/main.cpp` 仅构造对应 entry 并调 `run()`

## 容易踩的坑

### 1. gflags DEFINE_* 必须在全局作用域
`DEFINE_bool/DEFINE_string` 宏展开为全局变量 + 注册器，不能放类内/namespace 内（否则链接期找不到 `FLAGS_*`）。故 flag 定义分散在三个 .cpp 的 TU 顶层：`BaseApplicationEntry.cpp`（公共 `--verbose`）、`ServerApplicationEntry.cpp`（`--config`/`--gametest`）、`ClientApplicationEntry.cpp`（`--config`/`--skip-integrated`/`--quick-play`/`--quick-play-new`/`--benchmark-exit-after-initialize`）。gflags 把连字符规范化为下划线，故 `--quick-play-new` 命中 `DEFINE_bool(quick_play_new,...)`，保持连字符命令行风格不变。

### 2. gflags 未知 flag 会报错退出
`ParseCommandLineFlags(&argc, &argv, true)` 第三参数 `true` 表示遇未知 flag 报错 `exit(1)`。这会暴露给 server 误传 client 专属 flag（如 `--quick-play-new`）的调用错误。`--help` 由 gflags 内建处理（打印全部 flag 后 `exit(1)`）。

### 3. LogManager 须最早初始化、最先 shutdown
`BaseApplicationEntry::run()` 中 `LogManager::initialize()` 必须在 banner 之前（保证后续 spdlog 调用走异步）；`LogManager::shutdown()` 必须在 `ProfilerManager::shutdown()` 之前（避免日志消费线程访问已销毁资源）。`shutdown()` 会 flush 队列，确保退出前剩余日志落盘。

### 4. 溢出告警必须绕过 spdlog 队列
异步 logger 用 `overrun_oldest`（丢旧保新，主线程永不阻塞）。日志风暴时丢日志无感，故 LogManager 起监控线程采样 `thread_pool()->overrun_counter()` 增量，发现丢日志时经 `fprintf(stderr)` 直接输出告警——刻意绕过 spdlog 队列，否则告警自身可能被 overrun 丢弃，达不到"一定可见"的目的。

### 5. GameTest 接入不能上提到本模块
`_initializeServerGameTest`/`_cleanupServerGameTest` 依赖 `mc_test` 符号，而 `mc_test` 仅 `minecraft-server` exe 链接，client exe 不链接。本模块编入 `mc_common`（两 exe 都链接），故 GameTest 接入必须留在 `server/application/ServerApplicationEntry.cpp`。详见 `server/application/README.md` 第 12 节。
