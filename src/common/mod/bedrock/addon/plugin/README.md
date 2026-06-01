# 插件管理

管理脚插件的发现、加载、权限和生命周期。

- `ScriptPlugin` — 单个插件实例，管理状态机（Unloaded→Loading→Loaded→Running→Error）
- `ScriptPluginManager` — 插件发现、加载、卸载管理器
- `ScriptPluginSource` — 插件源接口，从BehaviorPack加载脚本
- `ScriptPackPermissions` — 包权限（位掩码实现，解析manifest capabilities）
- `ScriptPackConfiguration` — 包配置（允许/排除模块列表）
- `PluginExecutionGroup` — 执行分组枚举（PrePackLoad, ServerStart, ClientLevel）
