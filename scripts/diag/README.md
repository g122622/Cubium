# scripts/diag

## 目录结构

```text
scripts/diag/
├── orphan_sources.ts       # 游离源文件统计：找出未被任何 CMake target 收集、也未被聚合单元包含的 .cpp/.c（支持 --report-json/--report-md 供 CI 消费）
├── syntax-check/
│   └── syntax_check.sh     # 单文件语法检查：复用 compile_commands.json 的编译参数，加 -fsyntax-only 不产出 .o
└── windows-memory/         # Windows 服务端内存归因（UMDH / VMMap / 逐页驻留）
```

`orphan_sources.ts` 的判定分三层：先把仓库内全部 `CMakeLists.txt` / `*.cmake` 按语义求值，得到
`add_library` / `add_executable` / `target_sources` 收集到的源清单；再把 `#include "Xxx.cpp"` 的
“聚合单元成员”排除在外；最后用 `build/compile_commands.json`（若已配置）交叉核对，凡是真正进入编译器的
文件一律不算游离。

## 内部关系与外部依赖

`orphan_sources.ts` 只依赖 Node.js 内置模块，不依赖构建产物；`compile_commands.json` 只是可选的交叉核对
数据源。`windows-memory/` 的内容由 `ci/` 与人工排查内存问题时调用。

## 容易踩的坑

- **不要只靠 `grep CMakeLists.txt` 找游离**：本仓库的源清单大量经变量、`foreach`、`list(GET)`、
  自定义函数（如 `mc_add_diff_probe` / `mc_add_fuzz_target`）间接登记，grep 到文件名也未必真被编译。
- 相对源路径按**当前脚本目录**（`CMAKE_CURRENT_SOURCE_DIR`）解析，不是按仓库根；`tests/unit/client/`
  下的脚本写成 `client/renderer/X.cpp` 实际指向 `tests/unit/client/client/renderer/X.cpp`，属于路径写错，
  脚本会如实报告为游离。
- 被聚合单元 `#include "Xxx.cpp"` 的成员**不能**再补进 CMakeLists，否则同一批符号编译两次。
  它们的正确状态是 “Included by aggregate units”，不是游离。
- 输出末尾出现 “Parsing warnings” 时结论可能不完整：那说明脚本遇到了它不理解的变量或通配，
  应先看告警再判断，不要直接采信游离列表。
- `--all` 输出里每个文件都带状态（compiled / declared / aggregated / orphan），排查“某文件为什么没被编”
  时先看它被标成哪种状态。
- 未配置构建目录时脚本退化为纯静态判定，`file(GLOB)` 之外的黑盒收集手段可能漏判；跑一次
  `./scripts/configure.sh` 生成 `build/compile_commands.json` 即可启用交叉核对。
- `add_subdirectory` 与函数调用是**作用域边界**，`include()` 不是：兄弟子目录之间看不到对方的
  `set()`。静态求值若共用同一帧，会凭空“补全”本该未定义的变量，从而漏报真问题。
- `include()` 没有 include_guard 语义，同一脚本被多个子目录分别 include 是正常用法；按路径
  **全局**去重会让第二次起的 include 静默失效。去重只能沿当前调用链。
- `if()` 不引入作用域，但块体解析必须按**嵌套深度**切分 `elseif`/`else`，否则内层分支会被
  当成外层分支，块内命令被重复执行或漏执行。
- 退出码 1 同时表示「有游离」与「有解析告警」。CI 里必须显式 `exit 0`，否则会中断 job。