# Chat 模块

## 目录结构

```
src/client/chat/
├── ChatHistory.hpp    # 聊天消息数据结构和历史管理器接口
└── ChatHistory.cpp    # 聊天历史管理器实现
```

## 内部模块关系

模块仅包含一个核心类 `ChatHistory`，职责清晰：

- `ChatMessage` 结构体：封装聊天消息（富文本内容、类型、时间戳、永久标记）
- `ChatHistory` 类：管理消息存储、过期、可见性过滤、输入历史导航

## 上下游依赖关系

### 本模块依赖

| 依赖项 | 用途 |
|--------|------|
| `common/core/Types.hpp` | 基础类型（u8, f32 等） |
| `common/util/text/ITextComponent.hpp` | 富文本组件接口 |
| `common/util/text/StringTextComponent.hpp` | 纯文本组件实现 |
| `<chrono>` | 时间戳（steady_clock） |
| `<deque>` | 消息队列存储 |
| `<vector>` | 输入历史存储 |

### 被依赖

| 模块 | 使用方式 |
|------|----------|
| UI 层 `ChatWidget` | 组合成员，管理聊天界面数据和输入历史 |

## 容易踩的坑

### 1. 消息时间戳必须使用 steady_clock

`system_clock` 会受系统时间调整影响，导致淡出计算错误。代码已正确使用 `std::chrono::steady_clock`。

### 2. 发送命令后必须重置导航状态

发送新命令后应调用 `resetInputNavigation()`，否则上下箭头导航行为异常。

### 3. getVisibleMessages 返回副本

返回 `std::vector<ChatMessage>` 而非引用，避免迭代器失效问题。由于 `ChatMessage` 包含 `unique_ptr`，返回时需要深拷贝。

### 4. 重复输入会被过滤

`addToInputHistory()` 会检查并过滤与最后一条相同的输入。如需保留所有重复，需修改此逻辑。

### 5. 永久消息可能挤占显示位置

大量永久消息会占用 `MAX_VISIBLE`（10条）配额，导致普通消息无法显示。建议限制永久消息数量或在 UI 层分开显示。

### 6. ChatMessage 使用 unique_ptr<ITextComponent>

消息内容不是简单的 `std::string`，而是 `std::unique_ptr<text::ITextComponent>`，支持富文本。获取纯文本需调用 `getPlainText()`，获取带格式代码的文本需调用 `getFormattedText()`。

### 7. ChatMessageType 枚举

消息类型包括 `Chat`、`System`、`Actionbar`、`GameInfo`，而非简单的颜色值。`addSystemMessage()` 会自动应用灰色样式。
