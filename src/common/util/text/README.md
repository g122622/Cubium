# 文本系统 (Text System)

富文本系统，支持 Minecraft 风格的样式代码和 ITextComponent。

## 目录结构

```
text/
├── Utf8.hpp/cpp             # UTF-8 编码/解码/迭代工具函数
├── TextStyle.hpp/cpp       # 文本样式和格式化类型（颜色、粗体、斜体等）
├── StyleNbt.hpp/cpp        # Style ↔ NBT 复合标签编解码（组件与数字格式共用）
├── TextEvents.hpp          # 点击和悬停事件定义
├── ITextComponent.hpp/cpp  # 文本组件抽象接口及 BaseTextComponent
├── ITextComponentFwd.hpp   # 前向声明（避免循环依赖）
├── StringTextComponent.hpp # 纯文本组件实现
├── TranslationTextComponent.hpp/cpp  # 翻译键组件（支持多语言）
├── contents/               # 原版其余 5 种 contents（与 text/translate 合称 7 种）
│   ├── ScoreTextComponent.hpp    # 记分板分数（{"score":{name,objective}}）
│   ├── SelectorTextComponent.hpp # 实体选择器（{"selector":...,"separator":...}）
│   ├── KeybindTextComponent.hpp  # 按键绑定（{"keybind":"key.jump"}）
│   ├── NbtTextComponent.hpp      # NBT 路径（{"nbt":...,"interpret":...,"block"/"entity"/"storage":...}）
│   └── ObjectTextComponent.hpp   # 内联对象（{"object":{atlas/sprite 或 player}}，子对象原样透传）
├── ComponentFactory.hpp/cpp # 组件工厂（literal / translatable / translatableEscape）
├── ComponentUtils.hpp      # 文本组件工具函数（wrapInSquareBrackets 等）
├── ComponentNbtSerialization.hpp/cpp # ITextComponent ↔ NBT 标签/线格式字节
├── TextParser.hpp/cpp      # § 代码解析器（传统格式转 ITextComponent）
└── numbers/                # 数字格式（记分板分数的 blank/styled/fixed 渲染格式）
    ├── NumberFormat.hpp/cpp      # NumberFormatType 与 NumberFormat 值类型
    ├── NumberFormatCodec.hpp/cpp # 存档 NBT 编码 + Java 线格式编码
    └── README.md
```

## 内部模块关系

```
TextEvents.hpp (独立)
     ↓
TextStyle.hpp (依赖 TextEvents)
     ↓
StyleNbt.hpp (依赖 TextStyle + nbt)          ← 唯一的 Style NBT 实现
     ↓
ITextComponent.hpp (依赖 TextStyle)
     ↓
┌───────────────────┬────────────────────────┐
│                   │                        │
StringTextComponent.hpp   TranslationTextComponent.hpp
│                        (依赖 LanguageManager)
│
├── contents/* (5 种 contents，均依赖 ITextComponent)
├── ComponentFactory.hpp (依赖 String/Translation 组件)
├── ComponentUtils.hpp (依赖 ITextComponent + TranslationTextComponent)
│
ComponentNbtSerialization.hpp (依赖 ITextComponent + StyleNbt + NbtIo)
     ↓
numbers/NumberFormatCodec.hpp (styled 用 StyleNbt、fixed 用 ComponentNbtSerialization)
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
- **common/scoreboard/** - ScorePlayerTeam 队伍名称格式化、ScoreObjective 显示名称与数字格式
- **common/util/text/numbers/** - 数字格式的 styled/fixed 载荷（Style 与 Component 的 NBT 编码）
- **common/advancement/** - 进度显示名称和描述
- **common/network/ir/packets/play/** - Title 系列 IR 包（`SetTitleText`/`SetSubtitleText`/`SetActionBarText`，旧 `TitlePacket` 已删除）
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

### 8. Style 的 NBT 只有一份实现（StyleNbt）

组件序列化与数字格式（`numbers/`）都必须走 `StyleNbt.hpp` 的 `styleToNbt`/`styleFromNbt`，不要在
别处另写一份字段清单——两份清单必然漂移，表现为「组件里的颜色对了、数字格式里的丢了」。

颜色一律用 `Style::getTextColor()`（`TextColor::serialize()`，具名色写名字、自定义 RGB 写 `#RRGGBB`），
**不要**用 `getColor()`：后者只覆盖具名色，自定义 RGB 会被静默丢弃。

`shadow_color` **不是**颜色字符串，而是 **ARGB 打包整数**（`ExtraCodecs.ARGB_COLOR_CODEC` 的主形态是
`Codec.INT`），写成 `"#RRGGBB"` 会让真客户端解码失败；`font` 是资源位置字符串
（1.21.11 的 `FontDescription.CODEC` 只支持 `Resource` 变体）。这两个字段与 `color` 的类型完全不同，
不要顺手按 `color` 的写法处理。

### 9. `Result<std::unique_ptr<T>>::value()` 只能取一次

`componentFromNbtTag` 等返回 `Result<std::unique_ptr<T>>`，`value()` 内部会转移所有权。同一表达式里
调用两次，第二次拿到 nullptr —— 后续解引用即空指针崩溃。必须先把所有权取到局部变量：`auto v = r.value();`

### 10. 7 种 contents 的键名必须与 MAP_CODEC 一致

`score` / `selector` / `keybind` / `nbt` / `object` 的字段名直接决定真 Java 客户端能否解码：
`nbt` 组件的 `interpret` 是 ByteTag（`0x01`），数据源是同层的 `block`/`entity`/`storage`，
不是嵌套 `source`；`object` 子对象（atlas/sprite 或 player profile）**原样透传**，不要拆字段
重写，否则 ResolvableProfile 等结构会丢字段。`ObjectTextComponent` 的字符串化内容是 U+FFFC
占位符。

### 11. 工厂返回基类 `unique_ptr<ITextComponent>`

`text::translatable` / `text::literal` 返回基类指针（便于直接传给 `sendSuccess/sendFailure`），
需要 `getKey()`/`getParams()` 时先 `dynamic_cast<TranslationTextComponent*>`。`sendSuccess`
的 `unique_ptr` 重载会解引用后同步调用，临时对象生命周期安全。

### 12. Component 根标签不一定是 Compound

`ComponentSerialization` 经 `NbtOps` 编码后，可折叠纯文本是 **StringTag(0x08)**，只有带样式/siblings
的才是 CompoundTag(0x0A)。读线格式字节时不能假定根是 `0x0A`；写出请用
`nbt_io::serializeRootTagToBytes`（任意标签），它和 `serializeRootCompoundToBytes` 同一实现。

## 参考

- net.minecraft.util.text.ITextComponent
- net.minecraft.util.text.StringTextComponent
- net.minecraft.util.text.TranslationTextComponent
- net.minecraft.util.text.Style
- net.minecraft.util.text.event.ClickEvent
- net.minecraft.util.text.event.HoverEvent
