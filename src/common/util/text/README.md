# 文本系统 (Text System)

富文本系统，支持 Minecraft 风格的样式代码和 ITextComponent。

## 目录结构

```
text/
├── Utf8.hpp/cpp             # UTF-8 编码/解码/迭代工具函数
├── TextStyle.hpp/cpp       # 文本样式和格式化类型（颜色、粗体、斜体等）
├── TextEvents.hpp          # 点击和悬停事件定义
├── ITextComponent.hpp/cpp  # 文本组件抽象接口及 BaseTextComponent
├── ITextComponentFwd.hpp   # 前向声明（避免循环依赖）
├── StringTextComponent.hpp # 纯文本组件实现
├── TranslationTextComponent.hpp/cpp  # 翻译键组件（支持多语言）
├── ComponentUtils.hpp      # 文本组件工具函数（wrapInSquareBrackets 等）
└── TextParser.hpp/cpp      # § 代码解析器（传统格式转 ITextComponent）
```

## 内部模块关系

```
TextEvents.hpp (独立)
     ↓
TextStyle.hpp (依赖 TextEvents)
     ↓
ITextComponent.hpp (依赖 TextStyle)
     ↓
┌───────────────────┬────────────────────────┐
│                   │                        │
StringTextComponent.hpp   TranslationTextComponent.hpp
│                        (依赖 LanguageManager)
│
├── ComponentUtils.hpp (依赖 ITextComponent + TranslationTextComponent)
│
TextParser.hpp (依赖 ITextComponent, TextStyle)
```

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（u8, u32 等） |
| `nlohmann-json` | JSON 序列化/反序列化 |
| `LanguageManager` | 翻译键解析（TranslationTextComponent 可选依赖） |

### 下游依赖

被以下模块广泛使用：

- **common/entity/** - Entity 自定义名称（`Entity.hpp`）
- **common/item/** - ItemStack 显示名称和 Lore（`ItemStack.hpp`）
- **common/world/blockentity/** - SignEntity 告示牌文本、BannerEntity 旗帜名称
- **common/world/map/** - MapBanner、MapData、MapDecoration 地图标记文本
- **common/scoreboard/** - ScorePlayerTeam 队伍名称格式化、ScoreObjective 显示名称
- **common/advancement/** - 进度显示名称和描述
- **common/network/packet/** - TitlePacket、MapDataPacket 网络包
- **common/skin/** - SkinPackets 皮肤文本
- **server/command/** - TellRawCommand、TeamCommand、ScoreboardCommand、BossBarCommand 等命令
- **server/bossbar/** - BossInfo 血条显示
- **client/chat/** - ChatHistory 聊天历史
- **client/ui/** - ChatWidget、RichTextWidget、FontRenderer UI 渲染

## 容易踩的坑

### 1. TranslationTextComponent 需要设置 LanguageManager

```cpp
// 必须在启动时设置，否则翻译键无法解析
mc::text::TranslationTextComponent::setLanguageManager(&mc::LanguageManager::instance());
```

如果未设置，`getUnformattedText()` 会返回原始翻译键而非翻译后的文本。

### 2. 样式继承规则

子组件继承父组件样式，但可覆盖：
- 父组件设置红色
- 子组件设置粗体 → 显示为**红色+粗体**
- 子组件设置蓝色 → 显示为**蓝色**（覆盖父组件颜色）

### 3. append 与 appendText 的区别

```cpp
// append - 接受 unique_ptr<ITextComponent>
text->append(std::make_unique<StringTextComponent>("World"));

// appendText - 便捷方法，内部创建 StringTextComponent
text->appendText("World");
```

### 4. TextFormatting 枚举值不是颜色索引

`TextFormatting::Red` 的枚举值是 12，不是颜色表索引。使用 `getFormattingColor()` 获取实际 ARGB 颜色值。

### 5. § 代码解析不保留原始格式

`TextParser::parse()` 解析后调用 `TextParser::toLegacyFormat()` 可能产生不同但等效的 § 代码序列。例如 `"§c§lHello"` 可能输出为 `"§cHello"`（样式合并）。

### 6. JSON 序列化中的 extra 字段

`toJson()` 输出的 `extra` 字段包含所有子组件，反序列化时需要递归处理。`fromJson()` 会自动处理 `extra` 数组。

### 7. deepCopy 与 shallowCopy

- `deepCopy()` - 复制当前组件及所有子组件
- `shallowCopy()` - 仅复制当前组件，不含子组件

根据使用场景选择正确的拷贝方法。

## 参考

- net.minecraft.util.text.ITextComponent
- net.minecraft.util.text.StringTextComponent
- net.minecraft.util.text.TranslationTextComponent
- net.minecraft.util.text.Style
- net.minecraft.util.text.event.ClickEvent
- net.minecraft.util.text.event.HoverEvent
