# 插件管理

管理脚插件的发现、加载、权限和生命周期。

## 目录结构树

```
src/common/mod/bedrock/addon/plugin/
├── PluginExecutionGroup.hpp/cpp   # 执行分组枚举（PrePackLoad/ServerStart/ClientLevel）
├── ScriptPackConfiguration.hpp/cpp # 包配置（模块允许/排除列表、权限持有）
├── ScriptPackPermissions.hpp/cpp  # 包权限管理（位掩码实现，解析manifest capabilities）
├── ScriptPlugin.hpp/cpp           # 插件实例，管理状态机（Unloaded→Loading→Loaded→Running→Error）
├── ScriptPluginManager.hpp/cpp    # 插件发现、加载、卸载管理器（协调依赖和生命周期）
├── ScriptPluginSource.hpp/cpp     # 从BehaviorPack加载脚本源码（实现IDependencyLoader）
└── README.md
```

## 内部模块关系

```
ScriptPluginManager（核心协调者）
    │
    ├── 持有 ──→ ScriptPlugin（多个插件实例）
    │               │
    │               ├── 持有 ──→ IScriptContext（脚本上下文，来自core/）
    │               ├── 持有 ──→ ScriptPackConfiguration
    │               │               │
    │               │               └── 持有 ──→ ScriptPackPermissions
    │               │
    │               └── 加载时使用 ──→ ScriptPluginSource
    │
    └── 持有 ──→ ScriptPluginSource（每个BehaviorPack一个）
```

## 上下游外部依赖关系

### 本目录依赖
- `core/IScriptEngine.hpp` — 脚本引擎抽象接口，创建IScriptContext
- `core/IScriptContext.hpp` — 脚本上下文，执行模块代码
- `core/Result.hpp` — 结果类型
- `pack/BehaviorPack.hpp` — 行为包，提供脚本文件读取
- `pack/AddonManifest.hpp` — manifest解析，获取脚本模块入口和能力声明

### 被以下模块依赖
- `lifecycle/ScriptManager` — 脚本生命周期管理器
- `src/server/mod/bedrock/addon/ServerScriptManager` — 服务端脚本管理器集成

## 容易踩的坑

1. **状态机转换限制**：ScriptPlugin状态转换有严格限制，只能Unloaded→Loading→Loaded→Running，不能跳跃。start()只能在Loaded状态调用，stop()只能在Running状态调用。

2. **模块加载分工**：ScriptPluginSource.loadScript()对`@minecraft/*`模块返回nullopt，这些原生模块由引擎内部处理。只有用户脚本（相对路径）才从BehaviorPack文件系统加载。

3. **执行分组顺序**：ScriptPluginManager按固定顺序加载插件：PrePackLoad → ServerStart → ClientLevel。卸载时按逆序：ClientLevel → ServerStart → PrePackLoad。不要假设插件会同时启动。

4. **权限来源**：ScriptPackPermissions从manifest的capabilities字段解析，目前只支持`script_eval`能力。新增能力需要同步更新解析逻辑。
