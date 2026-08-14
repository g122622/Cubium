## 用语规范

在和用户对话、向用户汇报、向用户询问等场景下，必须使用严肃且清晰的语言风格，避免口语化。

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
有时候，任务（构建、agent等）可能会被自动后台化，这时请使用 TaskOutput 阻塞等待。

在windows上，只允许执行下面唯一构建命令：
```bash
./scripts/configure.sh build
```

在macos上：
```bash
cmake --build --preset macos-relwithdebinfo -- -j10
```

如果仍然对于构建过程存在疑难问题，可参考`docs/BUILD.md`

## 运行

```bash
./build/bin/RelWithDebInfo/minecraft-client --quick-play-new # 客户端（会同时启动内置服务端）
./build/bin/RelWithDebInfo/minecraft-server # 服务端（命令行参数由 gflags 解析，可用 --gametest 无头跑测试、--config 指定配置）
./build/bin/RelWithDebInfo/minecraft-server.exe --gametest # 服务端无头跑集成测试
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*GameTestServer*" # GameTestServer自动化集成测试
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="你的测试过滤器" # 其他单元测试

```

运行过程中的日志也会一并输出到控制台，如果你要抓日志来调试，请编译后直接执行上述命令即可，尽量不要让用户手动去执行。

日志一般非常大，请尽量不要读取全文，只抓取你需要的部分。

## 测试

如果你需要运行测试，请尽量不要运行整个mc-tests，因为它包含巨量测试用例，会严重超时。

## 代码格式化

提交代码之前，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

- 可能出现找不到clang-format的情况，此时需要手动指定路径。
（在windows上，我的vs安装在D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64/bin/clang-format.exe）
- 只允许格式化.cpp和.hpp文件，其他文件严禁通过上述命令格式化。
- .gen.cpp/.gen.hpp文件是自动生成的，禁止格式化。

## git 规范

### 提交信息格式
```
<type>(<scope>): <subject>
<body>
```

### 合并方式
不允许使用线性历史（rebase）

### 分支规范

如果不是特别大的feature/refactor等，不要新开分支，直接提交并推送到origin/main。

## 子代理使用规范

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

### MacOS
　
当前资源包路径：~\minecraft_reborn\resourcepacks\Vanilla
当前数据包路径：~\minecraft_reborn\datapacks\Vanilla
可供参考的MC源码路径：~/dev/MC/java/Minecraft1.21.11/
（如果项目中代码是参考1.16.5的，必须迁移到1.21.11）
Moonrise优化模组路径：~\dev\MC\Mods\Moonrise

## 其他

【重要】你时间充足，上下文也充足，必须始终选择最彻底、最耗时、最正确、最准确的做法。

【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果你顺手发现了某段代码中的逻辑不完整、缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。

【参考】

**基岩版**

- 逆向头文件 E:\dev\MC\LeviLamina\src\mc\scripting\modules\gametest

- 逆向头文件 E:\dev\MC\LeviLamina\src\mc\gametest

- 官方文档 E:\dev\MC\Mods\minecraft-creator\creator\ScriptAPI\minecraft\server-gametest

- 示例 E:\dev\MC\minecraft-gametests（介绍：This repo contains sample GameTest behavior files for Minecraft Bedrock Edition. Minecraft supports GameTests - a combination of JavaScript + MCStructures - for validating facets of Minecraft behavior. You can use GameTests to validate facets of your creations, as well!）

**java版**

D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft\gametest
