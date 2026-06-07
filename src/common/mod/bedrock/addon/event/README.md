# 脚本事件桥接

将游戏事件桥接到脚本系统，支持BeforeEvent（同步可取消）和AfterEvent（延迟批量）两种模式。

## 目录结构

```
event/
├── ScriptEventBus.hpp/cpp       # 脚本事件总线，持有before/after信号，驱动事件分发
├── BeforeEventSignal.hpp/cpp    # BeforeEvent信号，同步触发，可取消游戏操作
└── AfterEventSignal.hpp/cpp     # AfterEvent信号，延迟批量处理，不可取消
```

## 内部模块关系

```
ScriptEventBus
    ├── m_beforeEvents → BeforeEventSignal（同步可取消）
    └── m_afterEvents → AfterEventSignal（延迟批量）
```

- `ScriptEventBus` 持有并管理两个信号对象
- `BeforeEventSignal` 在游戏逻辑执行前同步触发所有订阅者
- `AfterEventSignal` 将事件入队，在tick结束时批量分发

## 上下游外部依赖关系

### 被以下模块依赖
- `lifecycle/ScriptManager` — 持有ScriptEventBus实例
- `modules/ScriptEventBinding` — 绑定JS事件订阅接口
- `modules/MinecraftModuleFactory` — 创建world.beforeEvents/afterEvents
- `server/mod/bedrock/addon/bridge/EventBridge` — 将ServerEventBus事件桥接到ScriptEventBus

### 依赖以下模块
- 无（仅依赖标准库和项目基础类型）

## 容易踩的坑

1. **BeforeEvent返回值语义**：`BeforeEventSignal::fire()`始终返回false，取消状态由调用者通过事件数据本身的`cancel()`方法检查，不通过返回值判断。

2. **BeforeEvent回调必全执行**：所有beforeEvent处理器都会被调用，即使事件已被取消。这与基岩版行为一致，脚本需要在处理开始时检查取消状态。

3. **初始化检查**：ScriptEventBus必须先调用`initialize()`才能工作，未初始化时所有操作静默跳过（不报错）。

4. **线程安全**：两个Signal类内部使用mutex保护，但ScriptEventBus的`tick()`需要在正确的线程调用。

5. **订阅列表迭代安全**：在回调中修改订阅列表（如unsubscribe）是安全的，实现会复制订阅列表再迭代。

6. **事件模式选择**：
   - BeforeEvent：需要阻止/修改原始操作时使用（如取消伤害、修改掉落）
   - AfterEvent：仅需监听/响应时使用（如日志、统计、触发连锁效果）
