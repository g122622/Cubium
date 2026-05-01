# 文本系统 (Text System)

富文本系统，支持 Minecraft 风格的样式代码和 ITextComponent。

## 目录结构

```
text/
├── TextStyle.hpp/cpp       # 文本样式和格式化类型
├── TextEvents.hpp          # 点击和悬停事件
├── ITextComponent.hpp/cpp  # 文本组件接口
├── ITextComponentFwd.hpp   # 前向声明（用于避免循环依赖）
├── StringTextComponent.hpp # 纯文本组件
├── TranslationTextComponent.hpp # 翻译键组件
└── TextParser.hpp/cpp      # § 代码解析器
```

## 核心类型

### TextFormatting

文本格式化枚举，对应 Minecraft 的 § 代码：

| 枚举值 | 代码 | 说明 |
|--------|------|------|
| Black | §0 | 黑色 |
| DarkBlue | §1 | 深蓝 |
| DarkGreen | §2 | 深绿 |
| DarkAqua | §3 | 深青 |
| DarkRed | §4 | 深红 |
| DarkPurple | §5 | 深紫 |
| Gold | §6 | 金色 |
| Gray | §7 | 灰色 |
| DarkGray | §8 | 深灰 |
| Blue | §9 | 蓝色 |
| Green | §a | 绿色 |
| Aqua | §b | 青色 |
| Red | §c | 红色 |
| LightPurple | §d | 浅紫 |
| Yellow | §e | 黄色 |
| White | §f | 白色 |
| Obfuscated | §k | 混淆 |
| Bold | §l | 粗体 |
| Strikethrough | §m | 删除线 |
| Underline | §n | 下划线 |
| Italic | §o | 斜体 |
| Reset | §r | 重置 |

### Style

样式类，包含颜色、样式标志和事件：

```cpp
mc::text::Style style;
style.setColor(mc::text::TextFormatting::Red);
style.setBold(true);
style.setClickEvent(mc::text::ClickEvent(mc::text::ClickAction::RunCommand, "/help"));
```

### ITextComponent

文本组件接口，支持嵌套和样式继承：

```cpp
// 创建红色粗体文本
auto text = std::make_unique<mc::text::StringTextComponent>("Hello");
text->setStyle(style);

// 添加子组件
text->append(std::make_unique<mc::text::StringTextComponent>(" World!"));

// 获取纯文本
String plain = text->getUnformattedText();  // "Hello World!"

// 获取带格式文本
String formatted = text->getFormattedText(); // "§c§lHello World!"
```

### TextParser

解析 § 代码格式文本：

```cpp
auto text = mc::text::TextParser::parse("§cHello §lWorld!");
String legacy = mc::text::TextParser::toLegacyFormat(*text);
```

## JSON 序列化

```json
{
  "text": "Hello",
  "color": "red",
  "bold": true,
  "clickEvent": {
    "action": "run_command",
    "value": "/help"
  },
  "extra": [
    {
      "text": " World!",
      "color": "blue"
    }
  ]
}
```

## 使用场景

1. **告示牌文本** - SignEntity 使用 ITextComponent 存储 4 行文本
2. **实体名称** - Entity 自定义名称使用 ITextComponent
3. **聊天消息** - ChatMessage 使用 ITextComponent 存储富文本
4. **物品描述** - ItemStack Lore 使用 ITextComponent 列表

## 依赖关系

```
TextStyle.hpp
    └── TextEvents.hpp
ITextComponent.hpp
    ├── TextStyle.hpp
    ├── StringTextComponent.hpp
    └── TranslationTextComponent.hpp
TextParser.hpp
    └── ITextComponent.hpp
```

## 参考

- net.minecraft.util.text.ITextComponent
- net.minecraft.util.text.StringTextComponent
- net.minecraft.util.text.TranslationTextComponent
- net.minecraft.util.text.Style
- net.minecraft.util.text.event.ClickEvent
- net.minecraft.util.text.event.HoverEvent
