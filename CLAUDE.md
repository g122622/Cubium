## 用语规范

在和用户对话、向用户汇报、向用户询问等场景下，必须使用严肃且清晰的语言风格，避免口语化。和用户对话的时候一律使用简体中文回应用户。

向用户询问的时候，必须向用户补充大量背景信息，以便对相关子系统没那么了解的用户也能快速了解到问题背景和本质。

## 文档参考

如果你的任务涉及代码修改、文档更新、编译等，必须完整阅读下面三个文件的内容：

/README.md
/docs/CODE_CONVENTIONS.md
/docs/PROJECT_CONVENTIONS.md

如果你的任务涉及代码修改、文档更新、编译等，且你的上下文中没有完整包含上述三个文件的内容，你必须立即补读这三个文件的内容。

## 构建

【重要】由于本项目代码量已达百万级，构建时间很长，任务的超时等待时间必须30分钟以上！你在等待构建完成的过程中，不允许做其他任何事情。必须保证`run_in_background=false`
等待子代理（agent）完成的过程中，不允许做其他任何事情。必须保证`run_in_background=false`
有时候，任务（构建、agent、子代理等）可能会被自动后台化，这时请必须使用 TaskOutput 工具阻塞等待，不要和子代理同时做事情。

在windows上，只允许执行下面唯一构建命令：
```bash
./scripts/configure.sh build
```

在macos上：
```bash
cmake --build --preset macos-relwithdebinfo -- -j{当前机器核心数减去2}
# 当前机器核心数要通过命令获取
```

如果仍然对于构建过程存在疑难问题，可参考`docs/BUILD.md`

## 运行

```bash
./build/bin/RelWithDebInfo/minecraft-client --quick-play-new # 客户端（会同时启动内置服务端）
./build/bin/RelWithDebInfo/minecraft-server --profiler_enabled=false # 服务端（命令行参数由 gflags 解析，可用 --gametest 无头跑测试、--config 指定配置） 建议绝大多数场景下明确传入--profiler_enabled=false，除非是在CPU性能分析场景下才需要启用这个flag
./build/bin/RelWithDebInfo/minecraft-server.exe --gametest # 服务端无头跑集成测试
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*GameTestServer*" # GameTestServer自动化集成测试
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="你的测试过滤器" # 其他单元测试

```

运行过程中的日志也会一并输出到控制台，如果你要抓日志来调试，请编译后直接执行上述命令即可，尽量不要让用户手动去执行。

日志一般非常大，请尽量不要读取全文，只抓取你需要的部分。

## 测试

当遇到测试错误的时候，未经过我的允许，不得擅自将代码回退到基线版本并重编译！因为这会非常耗时间！

如果你需要运行测试，请尽量不要运行整个mc-tests和整个集成测试，因为它包含巨量测试用例，会严重超时。你只需要运行你的修改涉及到的少量测试就行了，因为每晚会有个夜间的ci，它会利用晚上跑全量测试，如果这个全量测试有问题，我会收集起来，定期找你反馈。

你可能会碰到很多测试错误，它们要么是以前留下的存量问题，要么是测试并行执行的时候相互之间出现了状态污染，总之它们大概率不是你本次任务的问题，不要以此为理由将代码回退到基线版本并重编译！这很耗时！

## 代码格式化

提交代码之前，必须使用clang-format对你修改的文件进行格式化：

```
运行 scripts/format/clang_format_all.ts # 这个脚本会自动定位clang-format并自动格式化文件 必须node24版本（可以在同一行命令中先nvm切换到24）
```

## clang-tidy 抑制注释（NOLINT）写法

【重要】`// NOLINTNEXTLINE` 只对**紧邻的下一行**生效。若中间夹了任何其他注释行（哪怕是解释该抑制
原因的说明），抑制就会落到那条注释上，被抑制的诊断照常报出，且**没有任何提示**——表现为"写了 NOLINT
却依然报错"。

因此必须遵守：

1. **抑制行与它要抑制的代码行之间，不得有任何其他行**。需要解释原因时，把说明写在抑制行**之前**，
   并用一条空注释行 `//` 与抑制行隔开。
2. **抑制要精确对准报出诊断的那一行**。`performance-unnecessary-value-param` 的告警位置是**参数本身**，
   所以抑制必须写在**那个参数所在的参数行**的正上方——写在函数名行上方无效（clang-format 会把参数拆成
   多行，函数名行与参数行不再是同一行）。
3. **行尾 NOLINT 不可靠**：写在行尾的 `// NOLINT(...)` 经 clang-format 重排参数后会留在函数名行，
   而诊断在参数行，抑制失效；同理行尾 `NOLINTNEXTLINE` 抑制的是下一行而非本行。**统一使用参数行上方的
   独立 `// NOLINTNEXTLINE(检查名)` 行**。

正确写法（抑制写在目标参数行正上方，且中间无其他行）：

```cpp
// heldItem 必须按值传入：ItemUseContext 构造函数对该 stack 做 const_cast 后持有可写指针，
// onItemUse 内 getItemStackMut() 会直接改它；若指向权威槽，物品侧的自改会绕过下方
// itemId/damage 对比，导致外层再 shrink 一次（双重消耗）。
Result<BlockUseResult> BlockInteractionManager::handleItemUseOn(
    PlayerId playerId,
    const BlockPos& pos,
    const Vector3& hitPos,
    Direction face,
    Hand hand,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    ItemStack heldItem)
```

错误写法（抑制与签名之间夹了注释行 → 抑制落到注释行上，静默失效）：

```cpp
// NOLINTNEXTLINE(performance-unnecessary-value-param) — heldItem 必须按值传入：
// ItemUseContext 构造函数对该 stack 做 const_cast 后持有可写指针……
Result<BlockUseResult> BlockInteractionManager::handleItemUseOn(
    PlayerId playerId, const BlockPos& pos, ...)
```

改完必须用 `./scripts/tidy/tidy.sh --checks='-*,<检查名>' <文件>` 实测确认抑制生效（输出中不应再出现该诊断）。

## git 规范

### 提交信息格式
```
<type>(<scope>): <subject>
<body>
```

### 合并方式
不允许使用线性历史（rebase）

### 分支规范

常规修改默认在独立分支提交、推送，并通过 Pull Request 合入 `main`；分支默认使用 `codex/` 前缀。用户明确要求直接提交，或任务明确适合直接更新主分支时，可以直接在 `main` 提交并推送到 `origin/main`，这会绕过审核流程。

### pr规范

采用 PR 流程时，提交代码并完成所有工作之后，使用 gh 工具发起 PR。本项目使用了若干机器人进行自动代码审查、以及clang-tidy进行静态分析，因此需要在 PR 发起之后等待至少 10 分钟（这是为了等待自动化机器人完成代码审查和静态分析完毕），机器人的审查结论会以评论的形式反馈在 PR 上。等待机器人完成审查之后，读取这些评论并判断是否需要采纳，并等待静态分析出结果；如需修改代码，则提交修改后的代码（此时不用等机器人再次审查，直接操作 gh 工具把这个分支合入 main）。直接在 `main` 提交并推送时，不需要发起 PR。

## 子代理使用规范

【重要】最新规定，目前一律不准启动任何子代理。

【重要】不允许让子代理执行编译命令，因为多个子代理执行编译命令会导致构建系统出现大量严重问题甚至锁死，必须由你来执行编译命令！你必须在子代理的提示词当中显式说明这个问题。

## 不允许启动workflow

## 重要的外部路径（用于参考和辅助错误排查）

### Windows

当前资源包路径：C:\Users\Administrator\minecraft_reborn\resourcepacks\Vanilla
当前数据包路径：C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla
可供参考的MC Java版源码路径：D:\Minecraft\MC研究\Minecraft1.21.11源码
可供参考的基岩版源码路径：E:\dev\MC\LeviLamina （注意，全是逆向出来的hpp文件，没有cpp文件）
（如果项目中代码是参考1.16.5的，必须迁移到1.21.11）
Moonrise优化模组路径：E:\dev\MC\Mods\Moonrise
ConcurrentUtil源码路径：D:\MiscProjects\ConcurrentUtil

### WSL

和windows一致，直接换成 /mnt/c /mnt/d /mnt/e 等路径即可。

### MacOS
　
当前资源包路径：~\minecraft_reborn\resourcepacks\Vanilla
当前数据包路径：~\minecraft_reborn\datapacks\Vanilla
可供参考的MC源码路径：~/dev/MC/java/Minecraft1.21.11/
（如果项目中代码是参考1.16.5的，必须迁移到1.21.11）
Moonrise优化模组路径：~\dev\MC\Mods\Moonrise

## 其他

【重要】你时间充足，上下文也充足，必须始终选择最彻底、最正确、最准确的做法。

【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。

可以先不考虑本项目的客户端，因为本项目的客户端已经停止维护。目前仅支持用java版客户端连接本项目服务端。

【参考】

**基岩版**

- 逆向头文件 E:\dev\MC\LeviLamina\src\mc\scripting\modules\gametest

- 逆向头文件 E:\dev\MC\LeviLamina\src\mc\gametest

- 官方文档 E:\dev\MC\Mods\minecraft-creator\creator\ScriptAPI\minecraft\server-gametest

- 示例 E:\dev\MC\minecraft-gametests（介绍：This repo contains sample GameTest behavior files for Minecraft Bedrock Edition. Minecraft supports GameTests - a combination of JavaScript + MCStructures - for validating facets of Minecraft behavior. You can use GameTests to validate facets of your creations, as well!）

**java版**

D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\gametest
