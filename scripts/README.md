# 项目脚本

## 目录结构

```text
scripts/
├── ci/                          # nightly 结果汇总、性能对比、崩溃符号化与缓存清理（内含 tests/ 回归）
├── test/                        # GameTest / 差分测试编排与报告
│   ├── run-gametests.ts         # 全量 GameTest + 失败隔离重跑 + JUnit 聚合
│   ├── gametest-process.ts      # 按绝对期限启动/终止服务端进程
│   ├── run_diff.ts              # Cubium 与基岩 BDS 的差对比对
│   ├── setup.ts                 # 差分测试环境一次性准备（行为包产物、junction、server.properties）
│   ├── _bedrock_single.ts       # 单项基岩行为验证工具
│   ├── _fix_structure_origin.mjs # 结构原点修正工具
│   └── _rebuild_creeper_pit.ts  # 测试结构重建工具
├── format/                      # C++ 代码格式化与文本编码校验
│   ├── clang_format_all.ts      # 按 .clang-format 原地格式化 src/ 与 tests/ 的 .cpp/.hpp
│   ├── check_encoding.ts        # 校验全部文本文件为 UTF-8：无 BOM、无 U+FFFD、非 GBK
│   └── normalize_includes.ts    # 把含 ../ 的 include 改写为项目根路径
├── tidy/                        # clang-tidy wrapper
│   ├── tidy.sh                  # Linux：直接使用 compile_commands.json
│   └── tidy.ps1                 # Windows：注入 MSVC / Windows SDK / vcpkg 头文件路径
├── diag/
│   └── syntax-check/
│       └── syntax_check.sh      # 单文件语法检查：复用 compile_commands.json 参数，加 -fsyntax-only 不产出 .o
├── iwyu/                        # clang-include-cleaner 工具
│   ├── iwyu.sh / iwyu.ps1       # 只打印 include 建议，供人工 review
│   ├── fix_includes.ts          # 全仓库自动增删 include（支持 dry-run）
│   └── README.md                # 用法与已知问题
├── baking/                      # 离线烘焙：把原版实测/注册表数据固化为内嵌 C++ 快照或查找表
│   ├── bake_java_block_id_table.ts      # blockName ↔ Java registry id 查找表
│   ├── bake_java_block_state_table.ts   # 方块状态 → Java state id 表
│   ├── bake_java_item_table.ts          # 物品注册表快照
│   ├── gen_entity_dimensions.py         # 实体碰撞箱尺寸
│   ├── gen_item_enchantable.py          # 物品可附魔标签
│   └── gen_item_rarity.py               # 物品稀有度
├── diag/windows-memory/         # Windows 服务端内存归因（UMDH / VMMap / 逐页驻留）
├── diff/                        # 差分测试辅助
│   └── compact_block_map_color.py       # 压缩逐状态方块颜色快照，缩减编译体积
├── trace_analysis/              # Perfetto trace 分析脚本（输出为本地一次性产物，不入库）
├── addLicense.ts                # 为源码批量补充/更新 license header
├── check_includes.ts            # 校验 include 路径大小写与 git 中的实际文件一致
├── countLines.ts                # 统计仓库代码行数（按目录与扩展名分组）
├── build_unimplemented_content_doc.py   # 生成 docs/UNIMPLEMENTED_CONTENT.md（未注册方块/物品清单）
├── build_unimplemented_items_doc.py     # 生成未被数据包引用的物品清单
├── rename-perfetto-to-profiler.py       # 一次性机械改名脚本（perfetto → profiler），已完成
├── license_header.txt           # addLicense.ts 使用的许可证头模板
├── configure.sh                 # 跨平台构建入口（Windows 分支转发 configure.bat）
├── configure.bat                # Windows：注入 VS 开发环境后调用 CMake preset
├── configure.ps1                # PowerShell 版构建入口
└── vsenv.bat                    # 调用 VsDevCmd 并覆盖 VCPKG_ROOT 的环境脚本
```

## 内部关系与外部依赖

`configure.*` 统一经 `vsenv.bat` 注入 Visual Studio 环境后调用 CMake preset，产出 `build/bin/<Config>/`；
`test/` 消费这些构建产物，`ci/` 再读取测试产物与归档 artifacts 生成报告。

外部依赖按脚本区分：Node.js（原生 type stripping，格式化脚本要求 Node 24）、Python 3
（ci 图表依赖 Matplotlib/NumPy 与中文字体）、CMake/Ninja、LLVM 工具链
（clang-format / clang-tidy / clang-include-cleaner）、GitHub CLI、UMDH/VMMap。
CI 使用说明见 `docs/CI.md`。

## 容易踩的坑

- Windows 构建入口只有 `./scripts/configure.sh build`，它转调 `configure.bat` 并经 `vsenv.bat` 注入 VS 环境；
  直接 `cmake --preset` 会缺少 MSVC / Windows SDK 路径。
- Windows 的链接器由 clang 驱动决定：项目只在 Linux 追加 `-fuse-ld=lld`，Windows 实际使用 MSVC `link.exe`；
  `CMakeCache.txt` 中的 `CMAKE_LINKER=lld-link.exe` 不参与 Ninja 链接命令，不要据此判断。
- C++ 格式化脚本要求 Node.js 24，只处理 `.cpp`/`.hpp`，并跳过 `.gen.cpp/.gen.hpp`。
- 全仓库文本统一 UTF-8（无 BOM，`*.ps1` 例外保留 BOM）；提交前可跑
  `node --experimental-strip-types scripts/format/check_encoding.ts` 检查存量 GBK 与 U+FFFD 乱码。
- `iwyu.sh` 只打印建议；批量落地用 `fix_includes.ts`，其中 `.hpp` 的 include 移除默认禁用。
- `baking/` 的产物是数据包中不存在的内嵌快照，会与原版注册表漂移；上游数据变更后必须重新烘焙。
- 测试输出可能很大；CI 解析必须读取完整输入，长度限制只适用于展示结果。