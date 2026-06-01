# 基岩版Addon兼容模组系统

本目录实现了完整兼容基岩版Addon的模组系统，使用户可以直接使用现有的基岩版行为包（Behavior Pack）。

## 架构概览

```
addon/
├── core/          # 脚本引擎核心抽象层（IScriptRuntime/Engine/Context）
├── engine/        # QuickJS引擎实现
├── binding/       # 模块绑定框架（C++→JS桥接）
├── modules/       # @minecraft/server 模块绑定实现
├── plugin/        # 插件管理（发现、加载、生命周期）
├── pack/          # 行为包系统（manifest解析、包管理、依赖解析）
├── event/         # 脚本事件桥接（BeforeEvent/AfterEvent）
├── lifecycle/     # 生命周期管理（ScriptManager、Watchdog、Logger、TickListener）
└── diagnostics/   # 诊断与调试（ScriptDiagnostics、SentryLogger）
```

## 关键设计决策

- **JS引擎抽象**：通过 `IScriptRuntime`/`IScriptEngine`/`IScriptContext` 接口抽象JS引擎，当前使用QuickJS实现，未来可切换到V8
- **模块绑定**：使用声明式API（`IModuleBindingFactory`）注册C++类和函数到JS上下文
- **事件系统**：BeforeEvent同步可取消，AfterEvent延迟批量处理
- **行为包优先级**：行为包内容覆盖DataPack中同路径内容
- **插件生命周期**：Unloaded → Loading → Loaded → Running → Error/Unloading

## 服务端集成

服务端集成代码位于 `src/server/mod/bedrock/addon/ServerScriptManager`，在MinecraftServer中初始化和驱动。

## 当前状态

已实现的模块：
- [x] QuickJS引擎集成与抽象接口
- [x] 行为包系统（manifest解析、包列表、依赖解析）
- [x] 事件系统扩展（cancellation支持）
- [x] 模块绑定框架
- [x] @minecraft/server核心绑定
- [x] 插件生命周期管理
- [x] 服务端集成

未实现（后续迭代）：
- [ ] @minecraft/server-ui
- [ ] @minecraft/server-net
- [ ] 自定义方块/物品组件
- [ ] 调试器支持
- [ ] V8引擎后端
- [ ] 脚本热重载（/reload命令）
- [ ] Watchdog完整实现（超时终止脚本上下文）
