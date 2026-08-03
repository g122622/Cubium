# pch - 预编译头文件（PCH）

预编译头文件目录，收录全项目高频且稳定的头文件，由 CMake `target_precompile_headers`
预编译为单个 `.pch` 产物供各翻译单元（TU）复用，消除重复解析开销。

## 目录结构树

```
src/common/pch/
└── pch_common.hpp    # 唯一 PCH 头（标准库高频头 + glm/json/spdlog + Types/Result/AssertAll）
```

## 内部模块关系

`pch_common.hpp` 收录三类内容：

1. **标准库高频头**：`<memory>`/`<vector>`/`<string>`/`<optional>`/`<algorithm>`/
   `<functional>`/`<unordered_map>` 等，被全项目绝大多数 TU 引用。
2. **第三方重头**（按引用 TU 数降序）：
   - `<nlohmann/json.hpp>`（~316 TU，全项目最高引用，模板元编程重库）。
   - `<glm/glm.hpp>`（227 TU，渲染层数学库全家桶；项目无分散子头独立使用，
     全部引完整 glm.hpp，故 PCH 放全家桶收益最大）。
   - `<spdlog/spdlog.h>`（`Result.hpp` 传递引入的重头）。
3. **项目自有基础头**：`common/core/Types.hpp`（被 ~3221 个 TU 引用，最高频）、
   `common/core/Result.hpp`、`common/util/assert/AssertAll.hpp`（断言宏入口）。
4. **警告豁免 pragma**：上述第三方头（spdlog/glm/json）在 `-Wall -Wextra -pedantic`
   下均可能产生警告，而项目 `-Werror` 默认开启。PCH 编译单元继承 target 的全部编译
   选项，警告会直接导致 PCH 创建失败并阻塞整个 target。故在 include 这些头前后用
   `#pragma clang diagnostic push/ignored/pop` 局部豁免（sign-conversion/conversion/
   old-style-cast/shadow/double-promotion/float-conversion 等），作用域仅限本 PCH 头
   展开区间，使用方 TU 的警告行为不受影响。glm 与 spdlog 警告同源，复用同一 pragma 块；
   json 额外补 `-Wfloat-conversion`/`-Wcovered-switch-default` 等数值解析路径项。

## 上下游外部依赖关系

- **谁依赖本目录**：`mc_common`（STATIC 库，PCH 创建于此 target）、`minecraft-server`、
  `minecraft-client`（通过 `target_precompile_headers(... REUSE_FROM mc_common)` 复用
  同一 PCH，避免重复创建）。测试 target 不复用（其用 `-w` 全量禁用警告，且测试源文件
  依赖结构与生产 target 差异较大）。
- **本目录依赖谁**：标准库 + spdlog + `common/core` + `common/util/assert`，无额外依赖。

## 容易踩的坑

1. **PCH 头稳定性**：PCH 头（或其传递依赖头）的任何改动会使 `.pch` 失效，触发**所有**
   依赖 TU 全量重编译。因此本头**严禁放入业务头**（`BlockState`/`ChunkData`/网络包定义
   等高频演进头）。只放基础类型、错误处理、断言、稳定第三方库这类低频改动的头。
2. **`-Werror` 与第三方头**：`cmake/CompilerWarnings.cmake` 对 `GNU|Clang` 无条件开启
   `-Werror`。PCH 编译单元继承该选项，故收录的任何头（含传递依赖）触发警告都会阻塞
   PCH 创建。新增第三方头到 PCH 时，须先实测其在 `-Wall -Wextra -pedantic` 下是否干净；
   不干净则在 pragma 段补对应 `-W...` 豁免。注意豁免须成对（push/pop），勿泄漏到使用方。
   glm/spdlog 警告同源（sign-conversion/conversion/old-style-cast 等），复用同一 pragma 块；
   json 额外需 `-Wfloat-conversion`/`-Wcovered-switch-default` 等数值解析路径项。
3. **不放低频头**：如 `windows.h`（仅 4 文件引用）放 PCH 反增未使用它的 TU 的开销。
   PCH 收益 = 头重量 × 引用 TU 数，低频头收益边际。`nlohmann/json_fwd.hpp`（前向声明版）
   项目 0 引用，放 PCH 收益为零，禁止收入——真正高杠杆是完整 `json.hpp`（316 TU）。
4. **`.gen.cpp` 自动继承**：`mc_common` 内的 `.gen.cpp`（如 `java_block_state_table.gen.cpp`）
   会自动继承本 PCH，已核查其依赖与 PCH 内容一致，安全且有益。
5. **多 config 磁盘占用**：Ninja Multi-Config 下 Debug/RelWithDebInfo/Release 各生成
   独立 `.pch`，磁盘占用增加，功能正确。`MC_ENABLE_TRACING`/`MC_ENABLE_TRACY` 等开关
   变更须 clean rebuild（属正常预期）。加入 `glm/glm.hpp` 与 `nlohmann/json.hpp` 后
   `.pch` 体积显著增大（glm 全家桶 + json 模板元编程均为重库），属预期，单 config 仍可接受。
6. **新增 PCH 内容的流程**：先单独 `#include` 该头在一个空 TU 上以
   `-Wall -Wextra -pedantic -Werror` 编译验证零警告，再收入 PCH，避免 PCH 创建失败
   导致全 target 阻塞。
7. **TraceEvents.hpp 暂未收入的理由**：它条件引入 `<perfetto.h>`/`<tracy/Tracy.hpp>`
   （开启追踪时极重，收益高），但依赖 `TraceCategories.hpp` 的 `PERFETTO_DEFINE_CATEGORIES`
   类别注册表——新增追踪类别会改动该文件，使 PCH 失效触发全量重编译。类别树演进频率高于
   Types/Result 等基础头，性价比低于 glm/json，故暂缓，留作后续单独评估。
