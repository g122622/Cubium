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
├── TranslationTextComponent.hpp/cpp # 翻译键组件
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
std::string plain = text->getUnformattedText();  // "Hello World!"

// 获取带格式文本
std::string formatted = text->getFormattedText(); // "§c§lHello World!"
```

### 复合文本组件

通过 `append()` 方法组合多个文本组件，子组件会继承父组件样式：

```cpp
// 创建根组件（队伍颜色应用到根组件）
auto root = std::make_unique<mc::text::StringTextComponent>("");
mc::text::Style rootStyle;
rootStyle.setColor(mc::text::TextFormatting::Gold);  // 队伍颜色
root->setStyle(rootStyle);

// 追加前缀（可有自己的样式）
auto prefix = std::make_unique<mc::text::StringTextComponent>("[VIP] ");
mc::text::Style prefixStyle;
prefixStyle.setColor(mc::text::TextFormatting::Green);
prefixStyle.setBold(true);
prefix->setStyle(prefixStyle);
root->append(std::move(prefix));

// 追加名称
root->append(std::make_unique<mc::text::StringTextComponent>("Steve"));

// 追加后缀
root->append(std::make_unique<mc::text::StringTextComponent>(" ★"));

// 结果: "[VIP] Steve ★"
// - 根组件为金色，子组件继承
// - 前缀覆盖为绿色粗体
// - 名称和后缀继承金色
```

**使用场景**：
- `ScorePlayerTeam::formatName()` - 队伍名称格式化
- 聊天消息组合 - 多段不同样式文本
- 物品描述 - 多行富文本

### TranslationTextComponent

翻译键文本组件，支持多语言：

```cpp
// 简单翻译
auto text = std::make_unique<mc::text::TranslationTextComponent>("chat.type.text");

// 带参数翻译
auto text = std::make_unique<mc::text::TranslationTextComponent>("chat.type.announcement");
text->addParam(std::make_unique<mc::text::StringTextComponent>("Server"));
text->addParam(std::make_unique<mc::text::StringTextComponent>("Hello!"));
// 如果语言文件中 "chat.type.announcement" = "[%s] %s"
// 则 getUnformattedText() 返回 "[Server] Hello!"
```

**支持的占位符**：
- `%s` - 顺序参数，按出现顺序替换
- `%1$s`, `%2$s` - 位置参数，按索引指定位置
- `%%` - 转义的百分号，输出 `%`

**集成 LanguageManager**：

```cpp
// 在客户端初始化时设置语言管理器
mc::LanguageManager& langManager = mc::LanguageManager::instance();
langManager.loadLanguage(packList, "zh_cn");
mc::text::TranslationTextComponent::setLanguageManager(&langManager);

// 之后 TranslationTextComponent 会自动翻译
auto text = std::make_unique<mc::text::TranslationTextComponent>("item.minecraft.diamond");
std::cout << text->getUnformattedText();  // 输出: "钻石"
```

### TextParser

解析 § 代码格式文本：

```cpp
auto text = mc::text::TextParser::parse("§cHello §lWorld!");
std::string legacy = mc::text::TextParser::toLegacyFormat(*text);
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

**翻译组件 JSON 格式**：

```json
{
  "translate": "chat.type.announcement",
  "with": [
    {"text": "Server"},
    {"text": "Hello!"}
  ],
  "color": "yellow"
}
```

## 使用场景

1. **告示牌文本** - SignEntity 使用 ITextComponent 存储 4 行文本
2. **实体名称** - Entity 自定义名称使用 ITextComponent
3. **聊天消息** - ChatMessage 使用 ITextComponent 存储富文本
4. **物品描述** - ItemStack Lore 使用 ITextComponent 列表
5. **队伍名称格式化** - ScorePlayerTeam::formatName() 使用样式继承

## 依赖关系

```
TextStyle.hpp
    └── TextEvents.hpp
ITextComponent.hpp
    ├── TextStyle.hpp
    ├── StringTextComponent.hpp
    └── TranslationTextComponent.hpp
        └── LanguageManager (可选)
TextParser.hpp/cpp
    └── ITextComponent.hpp
```

## 与多语言系统的集成

翻译系统由 `LanguageManager` 类提供：

1. **加载语言文件**：从资源包加载 `assets/<namespace>/lang/<lang_code>.json`
2. **翻译查询**：根据翻译键获取翻译文本
3. **占位符替换**：支持 `%s` 和 `%1$s` 格式的参数替换
4. **全局实例**：`LanguageManager::instance()` 提供全局访问

```cpp
// 加载语言
mc::LanguageManager& lang = mc::LanguageManager::instance();
lang.loadLanguage(packList, "zh_cn");

// 简单翻译
std::string text = lang.get("item.minecraft.diamond");  // "钻石"

// 带参数翻译
std::string chat = lang.get("chat.type.text", {"Player", "Hello"});
// 如果 "chat.type.text" = "<%s> %s"，输出 "<Player> Hello"

// 检查键是否存在
if (lang.has("some.key")) { ... }

// 获取可用语言列表
auto languages = mc::LanguageManager::getAvailableLanguages(packList);
```

## 参考

- net.minecraft.util.text.ITextComponent
- net.minecraft.util.text.StringTextComponent
- net.minecraft.util.text.TranslationTextComponent
- net.minecraft.util.text.Style
- net.minecraft.util.text.event.ClickEvent
- net.minecraft.util.text.event.HoverEvent
- net.minecraft.client.resources.LanguageManager
- net.minecraft.util.text.LanguageMap
