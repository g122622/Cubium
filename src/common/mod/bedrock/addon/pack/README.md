# 行为包系统

解析和管理基岩版行为包。

## 目录结构

```
pack/
├── AddonManifest.hpp/cpp       # 清单解析（manifest.json format_version 2）
├── AddonModule.hpp/cpp         # 模块声明（script/data/resources类型）
├── AddonDependency.hpp/cpp     # 依赖声明
├── BehaviorPack.hpp/cpp        # 行为包装器（manifest + 资源读取）
├── BehaviorPackList.hpp/cpp    # 行为包列表管理（扫描、优先级排序、依赖解析）
├── PackDependencyResolver.hpp/cpp  # UUID依赖解析器
├── PackVersion.hpp/cpp         # 版本号（[major,minor,patch]格式，含兼容性检查）
└── README.md
```

## 内部模块关系

```
PackVersion ←── AddonModule ←── AddonManifest ←── BehaviorPack ←── BehaviorPackList
    ↑               ↑               ↑                   ↑                │
    └───────────────┴───────────────┴───────────────────┴────────────────┘
                                    AddonDependency          PackDependencyResolver
```

- `PackVersion`：基础类型，被`AddonModule`、`AddonDependency`、`AddonManifest`使用
- `AddonManifest`：依赖`AddonModule`、`AddonDependency`、`PackVersion`
- `BehaviorPack`：持有`AddonManifest`，提供资源读取接口
- `BehaviorPackList`：管理多个`BehaviorPack`，通过`PackDependencyResolver`解析依赖

## 外部依赖关系

### 上游依赖（本目录依赖）
- `common/core/Types.hpp` — 基础类型定义
- `common/core/Result.hpp` — 结果类型

### 下游依赖（依赖本目录）
- `plugin/ScriptPluginSource.hpp` — 从`BehaviorPack`加载脚本
- `plugin/ScriptPluginManager.hpp` — 使用`BehaviorPackList`管理包
- `lifecycle/ScriptManager.hpp` — 初始化脚本系统时扫描行为包
- `server/mod/bedrock/addon/ServerScriptManager.hpp` — 服务端脚本管理器

## 容易踩的坑

1. **版本兼容性语义**：`PackVersion::isCompatibleWith()`要求主版本号相同，次版本号和补丁号必须大于等于要求版本。这与语义化版本不同，是基岩版的特定规则。

2. **BehaviorPack不可拷贝**：`BehaviorPack`禁止拷贝（持有资源路径），只能移动，使用时需注意所有权转移。

3. **BehaviorPackList线程安全**：使用`std::shared_mutex`保护，读操作用共享锁，写操作用独占锁。高并发场景需注意锁竞争。

4. **依赖解析时机**：`resolveDependencies()`需要在所有包加载完成后调用，否则会报告缺失依赖。
