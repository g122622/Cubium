# 服务端交互式终端（--interactive）

`minecraft-server --interactive` 时启用的终端界面：左侧滚动日志、右侧实时状态面板、底部命令输入框。
用于在不启动 Java 客户端的情况下观察世界状态并执行命令。

## 目录结构

```text
server/application/console/
├─ ServerConsole.hpp/cpp            # 门面：attach/start/stop，持有全部部件与 UI 线程
├─ ServerConsoleOptions.hpp         # 构造参数（服务器名/版本/世界名/端口/缓冲容量常量）
├─ ui/
│  ├─ TerminalUi.hpp/cpp            # FTXUI ScreenInteractive + Loop 驱动（事件驱动 + 10 FPS 补帧）、剪贴板写入
│  ├─ ConsoleLayout.hpp/cpp         # 三区布局与键位分发（输入框 → 面板 → 日志回看）
│  ├─ LogViewComponent.hpp/cpp      # 日志区：日志+事件合并渲染、预折行、跟随/回看、滚轮
│  ├─ StatusPanelComponent.hpp/cpp  # 右侧面板：服务器/指标/自定义/内存/在线玩家，按高度折叠
│  ├─ PanelDetailStore.hpp          # 「自定义指标」行的跨线程容器（/panel 写，面板读）
│  ├─ CommandLineComponent.hpp/cpp  # 输入框：历史、Emacs 键、Tab 补全、Enter 提交
│  └─ CompletionPopupComponent.hpp/cpp # 输入框上方的候选列表
├─ buffer/
│  ├─ LogRingBuffer.hpp/cpp         # 线程安全定长日志环形缓冲（默认 2000 行）
│  └─ EventRingBuffer.hpp/cpp       # 事件环形缓冲（默认 500 条）+ § 样式码剥离
├─ bridge/
│  ├─ ConsoleLogConsumer.hpp/cpp    # LogManager 的 ILogLineConsumer → 日志缓冲
│  ├─ GameEventFeed.hpp/cpp         # 订阅 ServerEventBus → 事件缓冲
│  ├─ TerminalCommandSource.hpp/cpp # 控制台命令源：回执进事件区（不写日志）
│  ├─ ConsoleCommandQueue.hpp/cpp   # UI → tick 线程的命令队列
│  └─ CompletionRequestQueue.hpp/cpp# UI → tick 线程的同步补全请求（带超时）
├─ input/
│  └─ CommandCompletion.hpp/cpp     # 补全候选状态机、粘贴规范化、命令/聊天判定（纯逻辑，可单测）
├─ render/
│  ├─ LineWrapper.hpp/cpp           # 按显示宽度折行 / 宽度计算 / 制表符展开（纯逻辑，可单测）
│  ├─ PanelStats.hpp/cpp            # 面板统计文本：千位分隔、Top-N 排行、方块状态与 NBT 单行化
│  └─ PanelStatsCollector.hpp/cpp   # /panel 的世界扫描：区块状态、方块分布、群系分布
└─ metrics/
   └─ ServerMetricsSnapshot.hpp/cpp # 指标快照 + ServerInfo（tick 线程写，UI 线程读）
```

## 面板内容与刷新

右侧面板分五段：`Server`（版本/端口/世界/运行时长）、`Metrics`（TPS/tick/玩家/会话/区块/
可 tick 区块/实体/方块实体）、`Panel`（世界状态摘要，按需刷新）、`Memory`（工作集与提交量，
一位小数 + MiB 单位）、`Players`。

`Panel` 段的内容由 `/panel` 命令采集（权限 2）：

| 命令 | 作用 |
|---|---|
| `/panel`、`/panel refresh` | 扫描全部维度：区块状态（已加载/可 tick/已后处理）、方块 Top-N、群系 Top-N |
| `/panel block <x> <y> <z>` | 打印该方块的方块状态与方块实体 NBT（单行、截断） |
| `/panel places [数量]` | 取每列最低的非空气方块，随机抽取若干处打印坐标与方块状态 |
| `/panel biome <x> <y> <z>` | 打印该坐标的运行时群系（Voronoi 缩放后，不要求区块已加载） |

面板标题行的 `[refresh panel]` 按钮（鼠标点击）等价于把 `/panel` 填进输入框并聚焦，
再按 Enter 提交——点击本身不产生副作用，输入的文本仍可编辑。无交互式终端时 `/panel`
仍可执行，结果以普通回执出现在日志里。

方块/群系统计是抽样结果（纵向每 4 格一层、只统计非空气方块），输出中会写明抽样比例。

## 内部模块关系

```text
                   ┌─────────────────── UI 线程 ───────────────────┐
日志消费线程 ─────▶ │ ConsoleLogConsumer ─▶ LogRingBuffer ─┐          │
(spdlog 异步)       │                                    ├─▶ LogView
事件线程 ─────────▶ │ GameEventFeed ────▶ EventRingBuffer ┘          │
(ServerEventBus)    │                                              │
                    │ CommandLine ──submit──▶ ConsoleCommandQueue ─┼──┐
                    │ CommandLine ──Tab─────▶ CompletionRequestQueue ┼──┤
                    │ StatusPanel ◀── ServerMetricsStore ◀───────────┼──┤
                    └──────────────────────────────────────────────┘  │
                                                                       ▼
                    ┌────────────────── tick 线程 ──────────────────────┐
                    │ ServerConsole::_onPostTick                      │
                    │   1. _collectMetrics   → ServerMetricsStore      │
                    │   2. _drainCommands    → TerminalCommandSource   │
                    │   3. _drainCompletionRequests → Suggestions      │
                    └──────────────────────────────────────────────────┘
```

UI 线程**从不访问世界状态**（玩家、区块、命令树）：所有世界访问都在 tick 线程完成，UI 只读快照与缓冲。

## 上下游外部依赖关系

- 依赖：`common/application/LogManager`（日志行消费者注入点）、`server/event/ServerEventBus`、
  `server/command/{CommandRegistry,ServerCommandSource}`、`common/profiler/*`（Perfetto 插桩）。
- 被依赖：`server/application/ServerApplicationEntry.cpp`（`--interactive` 分支）、
  `server/command/commands/PanelCommand.cpp`（`/panel` 通过 `MinecraftServer::serverConsole()` 写面板，
  并复用 `render/PanelStats*`）。
- 第三方：FTXUI（vcpkg 组件目标 `ftxui::component`），只有 `minecraft-server` 链接它；
  `mc_common` / `mc_tests` / 差分探针均不链接 FTXUI。`render/` 子目录刻意不依赖 FTXUI，
  因为 `/panel` 命令要编进 `minecraft-client`（内置服）与 `mc_tests`——新增文件若引入了 FTXUI
  头，这两个目标都会链接失败。

## 容易踩的坑

1. **FTXUI 实际版本是 v6.1.9，不是 portfile 里写的 7.0.0 语义**。本机 vcpkg 基线解析到 v6，API 与 7.x 差异明显：
   入口类是 `ftxui::ScreenInteractive`（7.x 改名 `App`），头文件是 `ftxui/component/screen_interactive.hpp`；
   数组下标是 `screen.PixelAt()`（7.x 叫 `CellAt`）。改动 UI 代码前先看
   `build/vcpkg_installed/x64-windows/include/ftxui/` 里的真实头文件。
2. **FTXUI 的导出配置在 Windows 上带 `INTERFACE_COMPILE_OPTIONS "/utf-8"`**（MSVC 语法）。
   本项目用 clang++，直接 link 会报 `clang++: error: no such file or directory: '/utf-8'`。
   `src/server/CMakeLists.txt` 在 `find_package(ftxui)` 后清空了三个 ftxui 目标的该项，别删。
3. **`Event::mouse()` 在 v6 是非 const 成员**，持有 `const Event&` 时读鼠标要 `const_cast`（见 `LogViewComponent`）。
4. **v6 的 `Maybe()` 是组件装饰器，不能装饰 `Element`**。DOM 层要用条件表达式直接选择
   `emptyElement()` 或真实元素（见 `ConsoleLayout::OnRender`）。
5. **`yframe` 会把子节点撑到 `min_y`**，在 `vbox` 里套 `yframe` 会让内容溢出容器。
   日志区的可见行窗口由自己按 `scrollOffset` 切，不要依赖 frame 的自动滚动。
6. **Ctrl+C 在 TUI 里是按键，不是 SIGINT**。`ScreenInteractive::ForceHandleCtrlC(false)` +
   `CommandLineComponent` 把 Ctrl+C/Ctrl+Q 转成 `TerminalUi::requestExit()`，
   退出后由入口回到原有的 `server.stop()` 收尾链。
7. **补全查询必须回到 tick 线程**：命令节点的 requirement 会读 `IServer`，与执行命令同属一个线程约束。
   `CompletionRequestQueue` 用"两段等待"（各 500ms）避免服务端卡死时把 UI 线程永久卡住。
8. **日志区同时渲染日志与事件**：事件行带 `[chat]/[player]/[cmd]/[error]/[game]` 前缀区分；
   给整行套色会让选择复制混入颜色码，故只给事件行着色。
9. **FTXUI 的 `text()` 不自动折行，超宽内容被直接截断**，也不提供滚动条。日志区必须在紧凑行上
   自己按显示宽度折行（`render/LineWrapper`），宽度取布局给出的 xflex 区域宽；`yframe`/`vscroll_indicator`
   都不能替代（前者撑高容器，后者只画指示条）。折行时补一个"上面还有未显示内容"的标题栏提示，
   否则用户会误以为服务端没有输出。
10. **所有跨线程的重绘都必须走 `TerminalUi::requestRedraw()`**（`PostEvent(Event::Custom)`）。
    UI 循环现在是事件驱动 + 10 FPS 补帧：日志/事件/命令回执各自会唤醒，但指标快照这类
    "定时刷新但无事件"的数据源如果忘了调用，就要等下一次按键才会显示（旧实现靠 60 FPS 空转轮询
    掩盖了这个缺失，代价是空闲时持续整屏重绘、终端闪烁）。
11. **面板段落按可用高度折叠**：行数多于面板高度时整段省略（只保留玩家段优先），
    避免小窗口下把玩家列表挤出屏幕。新增段落时记得参与 `StatusPanelComponent::OnRender`
    的高度预算分配。