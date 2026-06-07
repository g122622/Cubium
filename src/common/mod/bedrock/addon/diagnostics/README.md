# 诊断与调试

提供脚本系统的错误收集和结构化日志。

## 目录结构树

```
diagnostics/
├── ScriptDiagnostics.hpp    # 脚本诊断收集器（错误/警告/信息收集与报告生成）
├── ScriptDiagnostics.cpp    # 实现文件
├── ScriptSentryLogger.hpp   # Sentry风格结构化日志器（使用[BedrockSentry]前缀）
└── ScriptSentryLogger.cpp   # 实现文件
```

## 内部模块关系

两个组件相互独立，无依赖关系：

- `ScriptDiagnostics`：面向插件系统，收集诊断条目并生成汇总报告
- `ScriptSentryLogger`：面向日志系统，输出结构化事件日志

## 上下游外部依赖关系

### 本目录依赖

- `common/core/Types.hpp` — 基本类型定义（i32, u8等）
- `spdlog` — 日志输出

### 被以下模块依赖

- `lifecycle/` — ScriptManager、Logger等生命周期组件（计划集成）
- 上级addon模块的其他组件可通过这两个类获取脚本诊断信息

## 容易踩的坑

1. **尚未集成到lifecycle/层**：这两个组件目前是独立实现，尚未被ScriptManager等生命周期组件调用集成
