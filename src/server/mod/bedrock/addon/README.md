# 服务端脚本管理器

服务端侧的脚本系统入口，继承通用 ScriptManager 并集成 MinecraftServer，桥接游戏事件到脚本事件总线。

## 目录结构树

```
src/server/mod/bedrock/addon/
├── bridge/                          # 事件桥接层
│   ├── EventBridge.cpp/hpp          # 游戏事件→脚本事件桥接器
│   └── ServerEventSignals.cpp/hpp   # 服务端事件信号定义（beforeEvents/afterEvents）
├── ServerScriptManager.cpp/hpp      # 服务端脚本管理器
└── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                    ServerScriptManager                       │
│  （顶层管理器，绑定 MinecraftServer 生命周期）                │
└─────────────────────────────────────────────────────────────┘
          │
          ├──→ ScriptManager（通用脚本管理器，来自 common/mod/bedrock/addon）
          │
          └──→ EventBridge（游戏事件桥接）
                      │
                      ├──→ ServerEventBus（订阅游戏事件）
                      └──→ ScriptEventBus（分发到 JS 脚本）
```

**核心流程**：
- `initialize()` → 创建引擎、注册模块工厂、初始化事件总线
- `loadPlugins()` → 扫描行为包目录、加载脚本插件
- `tick()` → 调度回调 → pending jobs → afterEvents 刷新
- `shutdown()` → 关闭事件桥接、关闭脚本系统

## 上下游外部依赖关系

### 本目录依赖
- `src/common/mod/bedrock/addon/` — ScriptManager、ScriptEventBus、事件信号基础设施
- `src/server/event/` — ServerEventBus、服务端事件类型（BlockBreakEvent 等）
- `src/server/application/` — MinecraftServer（广播消息、玩家列表、tick 计数）

### 被以下模块依赖
- `src/server/application/MinecraftServer` — 在 initializeCoreManagers() 创建实例

## 容易踩的坑

1. **beforeEvents vs afterEvents**：beforeEvents 同步执行可取消事件（通过 `e.cancel()`），afterEvents 在 tick 结束时延迟批量分发，不可取消
2. **事件桥接生命周期**：EventBridge 必须在 shutdown() 时取消所有 ServerEventBus 订阅，否则会访问已销毁的对象
3. **setServer() 时序**：必须在 initialize() 前后调用 setServer()，否则 ScriptWorldAccessor 的回调未绑定
