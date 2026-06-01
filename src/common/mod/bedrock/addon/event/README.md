# 脚本事件桥接

将游戏事件桥接到脚本系统，支持BeforeEvent（同步可取消）和AfterEvent（延迟批量）两种模式。

- `ScriptEventBus` — 脚本事件总线，扩展ServerEventBus支持脚本事件
- `BeforeEventSignal` — BeforeEvent信号，同步触发，可取消游戏操作
- `AfterEventSignal` — AfterEvent信号，延迟批量处理，不可取消

## 事件模式

- **BeforeEvent**：在游戏逻辑执行前同步触发，所有订阅者都会执行，cancel状态会被传播
- **AfterEvent**：在tick结束后批量处理，不可取消，用于监听和响应
