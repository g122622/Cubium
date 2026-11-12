# 构建模块

## 目录结构

```text
cmake/
├── BuildOptions.cmake          # 项目开关与 project() 前的 vcpkg 功能选择
├── BuildScheduling.cmake       # 按核心和内存限制编译并发，串行链接
├── CompilerCache.cmake         # Windows、Linux、macOS 的 ccache launcher
├── CompilerWarnings.cmake      # 项目警告策略
├── CopyRuntimeDlls.cmake       # Windows 运行库部署
├── IntegratedPacks.cmake       # 行为包的完整输入、生成产物和独立构建目标
├── Sanitizers.cmake            # 随开关启停的 C/C++ 编译与链接插桩
└── ServerCommandSources.cmake  # 服务端共享库消费的命令源清单
```

## 内部模块关系

根 CMake 在 `project()` 前加载构建开关，以便 vcpkg 只安装当前目标需要的功能。
语言初始化后加载警告、并发、缓存与插桩模块，再创建第三方库和项目目标。
`mc_server_shared` 编译命令与服务端公共实现，服务端、测试和 benchmark 复用其对象。
测试目标通过 `mc_integrated_packs` 获取 JS 与共享结构，生产服务端不依赖测试资源编译。

## 上下游外部依赖关系

入口为根 `CMakeLists.txt`、`CMakePresets.json` 和各目标的 CMake 清单；CI 消费生成的
编译数据库、CTest 元数据与可执行文件。行为包依赖 Node.js/npm，缓存依赖 ccache。

## 容易踩的坑

- vcpkg 功能必须在 `project()` 前选定；`server/tests/benchmarks/client` 分别控制可选依赖。
- sanitizer 参数由模块添加，不能再写入预设的通用 `CMAKE_*_FLAGS`。
- 多配置测试预设必须指定 `configuration`；开发、sanitizer 和 CI Release 使用独立目录。
- `MC_COMPILE_JOBS` 为空时按逻辑核数减二及每任务 1536 MiB 预算取较小值，最低为一；
  分布式构建必须显式传入远端并发，不能套用协调节点的本机限制。
- ccache 的必要 PCH 参数随 launcher 传入；目录、容量和远端前缀继续使用 ccache 的正常配置。
- 行为包输入清单排除生成的 `src/utils/` 和共享结构副本；JS 与副本均声明为产物，缺失会重建。
- 修改共享实现时只登记到共享库，不要在 benchmark、服务端和测试中再次登记同一源文件。
- Windows clang++ 消费 vcpkg 的 Abseil 时，根 CMake 将 MSVC 的原始链接参数改由 LINKER 转发；不要把 `-ignore:4221` 直接传给编译器驱动。
- `MC_ENABLE_NO_EXCEPTIONS` 默认 ON，只有根 `CMakeLists.txt` 末尾的「target 拓扑」段落消费它；
  开关本身必须在 `BuildOptions.cmake` 定义，否则 `project()` 前的逻辑与 CI 传参拿不到默认值。
  该开关改变 PCH 的异常模式，测试目标因此不能 `REUSE_FROM mc_common`，见 `tests/unit/CMakeLists.txt`
  的 `mc_configure_unit_target()`。
