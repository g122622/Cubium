---
name: mc-fix-test-error
description: 运行测试，认领并收敛项目代码中的一个测试错误
---

## 任务简介

运行测试，认领并收敛项目代码中的一个测试错误。项目中的测试错误产生原因多种多样，可能是因为：

- 被测试代码中存在bug
- 测试代码中存在bug
- 测试代码过时了，没有跟上被测试代码的变更
- 被测试代码中存在一些边界情况没有被覆盖到，导致测试代码在这些边界情况上失败了
等等。

### 运行测试的两种方式

项目已接入 CTest，**优先用 CTest 运行**（支持单用例限时、并行、按名筛选）。完整的测试体系说明见 [docs/TEST.md](../../../docs/TEST.md)，下面是常用命令速查：

```bash
cd build

# 推荐：通过 CTest 全量运行（Windows 多配置生成器须带 --build-config）
ctest --build-config RelWithDebInfo --output-on-failure -j8

# 按用例名筛选（正则匹配 TestSuite.TestCase）
ctest --build-config RelWithDebInfo -R 'ServerChunkManagerTest.GetChunkSync_MultipleChunks' --output-on-failure -V

# 直接运行可执行文件（不经 CTest；遇到第一个失败就停下，适合快速定位）
./build/bin/RelWithDebInfo/mc_tests --gtest_break_on_failure 2>&1 | tail -70
```

**单用例限时**：每个 gtest 用例都被 `gtest_discover_tests` 拆成独立 CTest 条目并附 `TIMEOUT`，默认 300 秒（`MC_TEST_TIMEOUT` CACHE 变量，`tests/CMakeLists.txt:2380`）。超时即判失败，用于及早暴露区块生成/光照等长耗时用例的 hang/flake。改超时：`cmake -B build -DMC_TEST_TIMEOUT=120 ...` 重配，或单次 `ctest --timeout 60` 临时覆盖。

> Windows 下 `ctest -N` 列不出用例是正常的（`DISCOVERY_MODE PRE_TEST`，运行期才探测），直接运行即可。详见 [docs/TEST.md](../../../docs/TEST.md)。

测试 target 共 5 个（`mc_tests` / `mc_resource_tests` / `mc_trident_tests` / `mc_command_tests` / `mc_village_tests`），均通过 `mc_register_gtests` 注册到 CTest。

## 任务详细流程

1. 认领其中一个你认为相对容易解决的错误。若用户已经指定了具体错误，则做用户的。
2. 选定了一个错误之后，请你先阅读相关readme文件，以初步了解内容和范围，然后启动3个子代理，彻查该错误涉及到的代码。不要限制子代理的输出字数，让它们尽可能详细地输出它们的探索结果。
3. 收敛的过程可能会非常复杂，可能需要你深入理解相关代码、派出子代理探索并复用已有基建、设计并输出一份合理的实现方案（但不要进入计划模式）、编写大量代码、编写新的更完善的测试用例、修复编译错误和测试失败等。你需要充分利用你的编程能力和项目理解能力来完成这个任务。如果你在收敛这个错误的过程中发现某个地方很难短时间内解决，我们非常鼓励你在这个地方先留下一个TODO（但必须加上明确且显眼的TODO注释，注释中要有明文`TODO`，便于全文搜索）。如果这个问题的原因非常显然，你可以不用启动任何子代理。
4. 如果被测代码没问题，只是测试代码出现了问题，说明这个代码对于使用者而言，可能是一个容易踩中的坑点，可以将其记录在相应目录下的readme文件中。
5. 最后，生成简体中文提交信息并提交代码，然后推送到远程仓库。注意：可能有人在和你并行工作，你可能需要处理潜在的合并冲突。
6. 如果拉取/推送代码的时候因为网络问题没有成功，那么你无需重试，将该提交留在本地后即可停下来了，以后我会帮你去做。另外，`/include/minecraft-reborn/version.h`这个文件如果git显示未提交，你不用理会，将其留在工作区即可，重点是处理其他文件。
7. 如果你在执行过程中发现这个错误过于复杂、实现时间太长等，你可以随时回滚代码，然后选下一个其他的错误去处理。

你需要随时留意：

1. 尽可能准确复刻`D:\Minecraft\MC研究\Minecraft1.21.11源码\net\minecraft`下的算法、逻辑、时序等，与原版任何一点点偏差都会导致偏离预期（因此你派出的子代理需要注意对比下mc与本项目源码）。但不要照搬，因为cpp和java的特性不同，项目的架构也不同，你需要根据cpp的特性和当前项目的架构进行合理的架构设计和调整。
2. 是否完整，没有遗漏类、文件、函数、数据、结构、逻辑分支等
3. 是否闭环、是否完整集成到游戏逻辑当中，没有形成无法被调用的“孤岛代码”

注意本项目基建已经相当完善（代码量已经100w+行），各种常数、常用工具函数、音频系统、粒子系统、资源包系统、命令系统、实体系统、物品系统、物理系统、碰撞、tick调度、存档等都已经有了相当完善的实现(另外world对象上面也挂了相当多的工具方法以便访问世界、操作世界等)，务必充分探索以实现复用，避免重复实现。

## 测试崩溃时查看完整调用栈

`mc_tests` 在 `main` 中安装了 `mc::assert::CrashHandler`（参考 `src/client/main.cpp`、`src/server/main.cpp`），崩溃（SEH 访问违例、除零、栈溢出、纯虚调用、`std::terminate`、`MC_ASSERT_RELEASE` 触发的 `abort`）时输出调用栈和寄存器到 stderr。

但 GoogleTest 默认安装自己的 SEH 处理器（`--gtest_catch_exceptions=1`），会抢先捕获 SEH 并打印 `unknown file: error: SEH exception with code 0x... thrown in the test body.` **不带栈**，CrashHandler 拿不到崩溃现场。要看到完整栈，必须禁用 gtest 的 SEH 捕获，让 CrashHandler 接管：

```bash
# 禁用 gtest SEH 捕获，崩溃时由 CrashHandler 输出完整调用栈
./build/bin/RelWithDebInfo/mc_tests --gtest_filter='ServerChunkManagerTest.GetChunkSync_MultipleChunks' --gtest_catch_exceptions=0
```

- `--gtest_catch_exceptions=0`：关闭 gtest 的 SEH 捕获，SEH 直接传给 CrashHandler。
- 崩溃时输出 `Reason: ACCESS_VIOLATION - Read/Write access at address 0x...`、寄存器转储、`Stack trace:`（帧序号 + 函数名 + 文件:行号）。
- 仅 Windows 需要 `--gtest_catch_exceptions=0`（Linux/macOS 用信号处理，gtest 默认不捕获 SIGSEGV）。
- 配合 `--gtest_filter` 定位到单个用例，栈最干净。`--gtest_break_on_failure` 会让 gtest 抛 `BREAKPOINT`（被 CrashHandler 当作断点，不是原始崩溃点），调试时不要混用。

## 实现要求

1. 阻塞等待所有子代理完成（禁止子代理在后台运行）。
2. 你时间充足、上下文也充足，先把所有任务代码一口气写完，最后再编译
3. 务必尽可能复用项目中已有的基建！
4. 在任务执行过程中，你可以随时派出子代理来帮你完成一些子任务，比如说探索项目中已有的对应基建、探索项目中需要依赖的某个子系统是否存在、甚至帮你编写代码！
