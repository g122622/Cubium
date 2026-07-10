# 内置组件

Kagero 提供了一套丰富的内置 Widget 组件，用于构建用户界面。

## Widget 基类

所有组件都继承自 `Widget` 基类：

```cpp
#include "kagero/widget/Widget.hpp"

namespace mc::client::ui::kagero::widget {

class Widget {
public:
    // 构造函数
    Widget() = default;
    explicit Widget(std::string id);

    // 生命周期
    virtual void init();
    virtual void tick(f32 dt);
    virtual void paint(PaintContext& ctx);  // 绘制方法
    virtual void onResize(i32 width, i32 height);

    // 事件处理
    virtual bool onClick(i32 mouseX, i32 mouseY, i32 button);
    virtual bool onRelease(i32 mouseX, i32 mouseY, i32 button);
    virtual bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY);
    virtual bool onScroll(i32 mouseX, i32 mouseY, f64 delta);
    virtual bool onKey(i32 key, i32 scanCode, i32 action, i32 mods);
    virtual bool onChar(u32 codePoint);
    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onFocusGained();
    virtual void onFocusLost();

    // 布局属性
    void setPosition(i32 x, i32 y);
    void setSize(i32 width, i32 height);
    void setBounds(const Rect& rect);
    void setAnchor(Anchor anchor);
    void setMargin(const Margin& margin);
    void setPadding(const Padding& padding);

    // 状态查询
    bool isVisible() const;
    bool isActive() const;
    bool isHovered() const;
    bool isFocused() const;
    bool isDisabled() const;
    bool contains(i32 x, i32 y) const;
    bool isMouseOver(i32 mouseX, i32 mouseY) const;

    // 状态设置
    void setVisible(bool visible);
    void setActive(bool active);
    void setFocused(bool focused);
    void setHovered(bool hovered);

    // 属性访问
    const std::string& id() const;
    const Rect& bounds() const;
    i32 x() const;
    i32 y() const;
    i32 width() const;
    i32 height() const;
    Anchor anchor() const;
    const Margin& margin() const;
    const Padding& padding() const;
    f32 alpha() const;
    void setAlpha(f32 alpha);

    // 层级
    void setZIndex(i32 z);
    i32 zIndex() const;
    void setParent(IWidgetContainer* parent);
    IWidgetContainer* parent() const;
};

} // namespace mc::client::ui::kagero::widget
```

## 基础属性

### Rect 矩形

```cpp
Rect rect(10, 20, 200, 100);  // x, y, width, height

rect.right();     // x + width
rect.bottom();    // y + height
rect.centerX();   // x + width/2
rect.centerY();   // y + height/2

rect.contains(50, 50);      // 检查点是否在矩形内
rect.intersects(other);     // 检查两矩形是否相交
rect.intersection(other);   // 获取交集
rect.isValid();             // 检查是否有效（宽高 > 0）
```

### Margin 边距

```cpp
Margin m1(10);                      // 四边都是 10
Margin m2(10, 20);                  // 水平 10，垂直 20
Margin m3(10, 20, 30, 40);          // 左、上、右、下

m1.horizontal();  // left + right
m1.vertical();    // top + bottom
```

### Padding 内边距

```cpp
Padding p1(10);                     // 四边都是 10
Padding p2(10, 20);                 // 水平 10，垂直 20
Padding p3(10, 20, 30, 40);         // 左、上、右、下

p1.horizontal();  // left + right
p1.vertical();    // top + bottom
```

### Anchor 锚点

```cpp
enum class Anchor : u8 {
    TopLeft,        // 左上角
    TopCenter,      // 顶部居中
    TopRight,       // 右上角
    CenterLeft,     // 左侧居中
    Center,         // 正中心
    CenterRight,    // 右侧居中
    BottomLeft,     // 左下角
    BottomCenter,   // 底部居中
    BottomRight     // 右下角
};
```

## 文本组件 (TextWidget)

显示文本的组件：

```cpp
#include "kagero/widget/TextWidget.hpp"

using namespace mc::client::ui::kagero::widget;

// 创建文本组件
auto text = std::make_unique<TextWidget>("title", 10, 10, 200, 30);
text->setText("Hello World");
text->setColor(Colors::WHITE);
text->setShadow(true);
text->setShadowColor(Colors::MC_DARK_GRAY);

// 对齐方式
text->setAlignment(TextAlignment::Center);  // Left, Center, Right

// 自动换行
text->setWordWrap(true);
text->setMaxLines(3);
text->setLineHeight(12);

// 文本缩放
text->setScale(1.5f);

// 获取文本尺寸
f32 width = text->getTextWidth();
f32 height = text->getTextHeight();
i32 lines = text->getLineCount();
```

## 按钮组件 (ButtonWidget)

可点击的按钮组件：

```cpp
#include "kagero/widget/ButtonWidget.hpp"

// 创建按钮
auto button = std::make_unique<ButtonWidget>(
    "btn_submit",      // ID
    100, 100,          // 位置
    200, 40,           // 尺寸
    "Submit",          // 文本
    [](ButtonWidget& btn) {
        // 点击回调
        std::cout << "Button clicked: " << btn.id() << std::endl;
    }
);

// 设置样式
ButtonWidget::Style style;
style.normalColor = Colors::fromARGB(255, 60, 60, 60);
style.hoverColor = Colors::fromARGB(255, 80, 80, 80);
style.disabledColor = Colors::fromARGB(255, 40, 40, 40);
style.textColor = Colors::WHITE;
style.borderColor = Colors::fromARGB(255, 100, 100, 100);
style.cornerRadius = 4;
button->setStyle(style);

// 设置提示回调
button->setTooltip("保存");  // 单行提示
// 或多行提示：
button->setTooltip(Tooltip::create("保存", "将当前进度保存到存档"));
// 延迟显示（毫秒）
button->setTooltipDelay(500);

// 查询状态
i32 state = button->getRenderState();  // 0=禁用, 1=正常, 2=悬停
u32 bgColor = button->getBackgroundColor();
u32 textColor = button->getTextColor();
```

### 图片按钮 (ImageButtonWidget)

使用自定义图片的按钮：

```cpp
auto imgButton = std::make_unique<ImageButtonWidget>(
    "btn_icon",
    100, 100, 40, 40,    // x, y, width, height
    0, 0, 40,             // u, v, hoveredU
    "textures/gui/buttons.png"  // texture path
);

// 设置纹理坐标
imgButton->setTextureCoords(0, 0, 40);
```

## 复选框组件 (CheckboxWidget)

可选中/取消选中的复选框：

```cpp
#include "kagero/widget/CheckboxWidget.hpp"

auto checkbox = std::make_unique<CheckboxWidget>(
    "chk_option",
    10, 10,
    "Enable Feature"
);

// 设置状态
checkbox->setChecked(true);
checkbox->toggle();  // 切换状态

// 获取状态
bool checked = checkbox->isChecked();

// 设置回调
checkbox->setOnChanged([](bool checked) {
    std::cout << "Checkbox: " << (checked ? "checked" : "unchecked") << std::endl;
});

// 自定义颜色
checkbox->setTextColor(Colors::WHITE);
checkbox->setCheckColor(Colors::MC_GREEN);
```

## 滑块组件 (SliderWidget)

可拖动调整值的滑块：

```cpp
#include "kagero/widget/SliderWidget.hpp"

auto slider = std::make_unique<SliderWidget>(
    "sldr_volume",
    10, 10, 200, 20,    // x, y, width, height
    0.0, 100.0, 50.0    // min, max, value
);

// 设置值
slider->setValue(75.0);
slider->setFromRatio(0.5);  // 设置为 50%

// 获取值
f64 value = slider->value();
f64 ratio = slider->getRatio();  // 0.0 - 1.0

// 设置范围和步长
slider->setRange(0.0, 100.0);
slider->setStepSize(5.0);  // 步进 5

// 设置显示文本
slider->setDisplayText("Volume: {}");
slider->setFormatCallback([](f64 value) -> std::string {
    return "Volume: " + std::to_string(static_cast<i32>(value)) + "%";
});

// 设置回调
slider->setOnValueChanged([](f64 value) {
    std::cout << "Value: " << value << std::endl;
});

// 检查状态
bool dragging = slider->isDragging();
```

### 整数滑块 (IntSliderWidget)

```cpp
auto intSlider = std::make_unique<IntSliderWidget>(
    "sldr_count",
    10, 10, 200, 20,
    0, 100, 50
);

intSlider->setIntValue(75);
i32 value = intSlider->intValue();
```

## 文本输入框 (TextFieldWidget)

支持文本输入、选择、复制粘贴的输入框：

```cpp
#include "kagero/widget/TextFieldWidget.hpp"

auto textField = std::make_unique<TextFieldWidget>(
    "txt_name",
    10, 10, 200, 24
);

// 设置文本
textField->setText("Hello");
textField->setPlaceholder("Enter your name...");

// 获取文本
const std::string& text = textField->text();

// 设置最大长度
textField->setMaxLength(32);

// 设置文本变化回调
textField->setTextChangedCallback([](const std::string& text) {
    std::cout << "Text changed: " << text << std::endl;
});

// 设置验证器
textField->setValidator([](const std::string& text) {
    // 只允许字母和数字
    for (char c : text) {
        if (!std::isalnum(c)) return false;
    }
    return true;
});

// 光标操作
textField->setCursorPosition(5);
textField->setCursorPositionStart();
textField->setCursorPositionEnd();
i32 pos = textField->cursorPosition();

// 选择操作
textField->selectAll();
textField->clearSelection();
bool hasSel = textField->hasSelection();
std::string sel = textField->getSelectedText();

// 状态
textField->setEnabled(true);
textField->setCanLoseFocus(true);
textField->setDrawBackground(true);
```

## 容器组件 (ContainerWidget)

容纳其他组件的容器：

```cpp
#include "kagero/widget/ContainerWidget.hpp"
#include "kagero/widget/IWidgetContainer.hpp"

auto container = std::make_unique<ContainerWidget>("main");
container->setBounds(Rect(0, 0, 800, 600));

// 添加子组件
container->addChild(std::make_unique<TextWidget>("title", 10, 10, 200, 30));
container->addChild(std::make_unique<ButtonWidget>("btn", 10, 50, 100, 30, "Click"));

// 访问子组件
Widget* child = container->findChild("title");
size_t count = container->childCount();

// 移除子组件
container->removeChild("title");
container->clearChildren();
```

### Flex 布局

ContainerWidget 支持 Flex 弹性布局，自动排列子元素：

```cpp
// 设置 Flex 列布局
container->setLayoutType(ContainerLayoutType::Flex);
container->setFlexConfig(layout::centerColumnFlexConfig());

// 自定义 Flex 配置
layout::FlexConfig config;
config.direction = layout::Direction::Column;
config.justifyContent = layout::JustifyContent::Center;
config.alignItems = layout::Align::Center;
config.gap = 10;
container->setFlexConfig(config);
```

**FlexConfig 字段**：

| 字段 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| `direction` | `Direction` | `Row` | 主轴方向 |
| `justifyContent` | `JustifyContent` | `Start` | 主轴对齐 |
| `alignItems` | `Align` | `Stretch` | 交叉轴对齐 |
| `wrap` | `Wrap` | `NoWrap` | 换行方式 |
| `gap` | `i32` | `0` | 子元素间距 |

**Direction 枚举**：`Row`、`RowReverse`、`Column`、`ColumnReverse`

**JustifyContent 枚举**：`Start`、`Center`、`End`、`SpaceBetween`、`SpaceAround`、`SpaceEvenly`

**Align 枚举**：`Start`、`Center`、`End`、`Stretch`、`Baseline`

**便捷函数**：

```cpp
// 水平居中
auto config = layout::centerRowFlexConfig();

// 垂直居中（常用）
auto config = layout::centerColumnFlexConfig();

// 两端对齐
auto config = layout::spaceBetweenFlexConfig();
```

在模板中使用 Flex 布局：

```xml
<container layout="flex-column" gap="10" align-items="center" justify-content="center">
    <button text="OK" size="100,20"/>
    <button text="Cancel" size="100,20"/>
</container>
```

**`<screen>` 标签默认使用垂直居中 Flex 布局**，无需手动指定 `layout` 属性。

### Grid 布局

ContainerWidget 同时支持 Grid 网格布局，按列/行均匀排布子元素：

```cpp
// 设置 Grid 布局
container->setLayoutType(ContainerLayoutType::Grid);

// 自定义 Grid 配置
layout::GridConfig config;
config.columns = 9;        // 列数（≥1，默认 1）
config.rows = 3;           // 行数（0=自动推算，默认 0）
config.columnGap = 2;      // 列间距（≥0）
config.rowGap = 2;         // 行间距（≥0）
container->setGridConfig(config);
```

**GridConfig 字段**：

| 字段 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| `columns` | `i32` | `1` | 列数，<1 时被钳制为 1 |
| `rows` | `i32` | `0` | 行数，0 表示根据子项数与列数自动推算 |
| `columnGap` | `i32` | `0` | 列间距（横向间隙），<0 时被钳制为 0 |
| `rowGap` | `i32` | `0` | 行间距（纵向间隙），<0 时被钳制为 0 |
| `autoPlacement` | `bool` | `true` | 是否启用自动放置（按行优先填充单元格） |

**自动推算行数**：当 `rows=0` 时，GridLayout 会按 `ceil(childCount / columns)` 推算实际行数，保证所有子项都能被布局。

**单元格尺寸**：每个单元格的宽度为 `(contentWidth - (columns-1) * columnGap) / columns`，高度同理。

在模板中使用 Grid 布局：

```xml
<grid id="inventoryGrid" cols="9" rows="3" gap="2" pos="10,80" size="178,70">
    <slot id="slot_0" index="0" size="18,18"/>
    <slot id="slot_1" index="1" size="18,18"/>
    <!-- ... -->
</grid>
```

模板属性语义：

| 属性 | 说明 |
|------|------|
| `cols` | 列数，非数字或缺失时为默认值 1；<1 时被钳制为 1 |
| `rows` | 行数，非数字或缺失时为默认值 0（自动推算） |
| `gap` | 同时设置 `columnGap` 与 `rowGap`；<0 时被钳制为 0 |

> 注意：当前 `<grid>` 标签的 `gap` 属性是 `columnGap` 与 `rowGap` 的便捷写法，等价于同时设置两者为相同值。如需让两者不同，请在 C++ 侧通过 `setGridConfig` 设置。

## 图片组件 (ImageWidget)

在 UI 上显示一张来自 GUI 精灵图集的纹理图片。设计参考 MC Java 版 `net.minecraft.client.gui.components.ImageWidget`：

- **非交互组件**：`isActive()` 始终为 `false`，不参与焦点导航，不响应点击
- **尺寸语义**：支持显式尺寸与 `auto` 自适应（按纹理原始像素尺寸）
- **着色**：支持 ARGB 着色（`tint`），默认 `0xFFFFFFFF`（无着色）
- **UV 子区域**：支持自定义 UV，用于从大图集中截取局部精灵
- **回退渲染**：当未关联图集或精灵不存在时，回退为半透明品红矩形，便于开发期识别资源缺失

```cpp
#include "kagero/widget/ImageWidget.hpp"

// 基本用法：指定精灵来源与显式尺寸
auto image = std::make_unique<ImageWidget>("logo");
image->setSpriteSource(atlas, "title_logo");
image->setBounds(Rect(10, 10, 200, 60));
container->addChild(std::move(image));

// 自动尺寸：按纹理原始像素尺寸绘制
auto icon = std::make_unique<ImageWidget>("icon");
icon->setSpriteSource(atlas, "heart_full");
icon->setAutoSize(true);
icon->setBounds(Rect(10, 10, 0, 0)); // auto 模式下宽高由纹理决定

// 着色
image->setTint(0x80FF0000); // 半透明红色叠加

// 自定义 UV（归一化坐标）
image->setUV(0.25f, 0.5f, 0.75f, 1.0f);
image->clearUV(); // 恢复整张纹理
```

**核心 API**：

| 方法 | 说明 |
|------|------|
| `setSpriteSource(atlas, spriteId)` | 绑定精灵图集与精灵ID |
| `setSpriteId(spriteId)` | 设置精灵ID（保持图集不变） |
| `setAtlas(atlas)` | 设置图集指针（外部拥有） |
| `setTint(argb)` / `tint()` | 设置/获取着色颜色 |
| `setAutoWidth(b)` / `autoWidth()` | 自动宽度（按纹理宽度） |
| `setAutoHeight(b)` / `autoHeight()` | 自动高度（按纹理高度） |
| `setAutoSize(b)` | 同时设置自动宽高 |
| `setUV(u0,v0,u1,v1)` / `clearUV()` | 自定义 UV 子区域 |
| `hasCustomUV()` | 是否设置了自定义 UV |

在模板中使用：

```xml
<image id="logo" src="title_logo" size="200,60" tint="#FFFFFFFF"/>
<image id="icon" src="heart_full" size="auto,auto"/>
```

模板属性：

| 属性 | 说明 |
|------|------|
| `src` | 精灵ID，对应 `setSpriteId`。图集需在外部通过 `setSpriteSource`/`setAtlas` 注入。 |
| `tint` | 着色颜色（ARGB），支持 `#RRGGBB`、`#RRGGBBAA`、`rgb(...)`、`rgba(...)` 与颜色名称 |
| `size` | 尺寸，支持 `auto` 关键字（如 `auto,auto`、`auto,100`、`100,auto`）。`auto` 分量按纹理原始像素尺寸绘制 |

> 注意：模板工厂仅设置精灵ID与着色，**图集指针需由业务层在创建后注入**（通过 `setSpriteSource` 或 `setAtlas`），因为 `BuiltinWidgets` 工厂无法访问运行期 `GuiSpriteAtlas` 实例。

## 滚动容器 (ScrollableWidget)

支持滚动的内容容器：

```cpp
#include "kagero/widget/ScrollableWidget.hpp"

auto scrollable = std::make_unique<ScrollableWidget>(
    "scroll_list",
    10, 10, 300, 400    // x, y, width, height
);

// 设置内容尺寸
scrollable->setContentSize(300, 1000);

// 设置滚动位置
scrollable->setScrollX(0);
scrollable->setScrollY(100);

// 获取滚动位置
i32 scrollX = scrollable->scrollX();
i32 scrollY = scrollable->scrollY();

// 滚动到指定位置
scrollable->scrollTo(0, 200);

// 滚动到可见区域
scrollable->scrollIntoView(child);
```

## 列表组件 (ListWidget)

显示列表项的组件：

```cpp
#include "kagero/widget/ListWidget.hpp"

auto list = std::make_unique<ListWidget>("list");
list->setBounds(Rect(10, 10, 200, 300));

// 添加列表项
list->addItem("Item 1");
list->addItem("Item 2");
list->addItem("Item 3");

// 选择
list->setSelectedIndex(0);
i32 selected = list->selectedIndex();

// 多选
list->setMultiSelect(true);
list->setSelectedIndices({0, 2});

// 回调
list->setOnSelectionChanged([](i32 oldIndex, i32 newIndex) {
    std::cout << "Selected: " << newIndex << std::endl;
});
```

### 数据绑定与刷新

ListWidget 支持从 `tpl::binder::Value` 数组构建列表项，常用于模板 `bind:items` 绑定：

```cpp
// 设置 ItemFactory 自定义从 Value 创建列表项的方式
list->setItemFactory([](const tpl::binder::Value& data, size_t index) {
    return std::make_unique<TextListItem>(data["label"].toString());
});

// 从 Value 数组设置列表项（同时缓存数据源）
tpl::binder::Value array = ...;  // Array 类型
list->setItemsFromValue(array);

// 之后若替换了 ItemFactory，可使用缓存的数据源重建列表项，
// 无需重新提供 Value 数组
list->setItemFactory(newFactory);
list->refreshItems();
```

**数据源缓存语义：**
- `setItemsFromValue(array)` 会将 `array` 缓存到内部 `m_dataSource`，后续可通过 `refreshItems()` 复用
- `refreshItems()` 使用缓存的数据源重建列表项，并尽可能保留当前选中状态（单选索引重新校验范围，多选列表过滤越界索引）
- 两个方法均触发 `m_onItemsChanged` 回调以保持观察者一致性
- `hasCachedDataSource()` 可查询是否存在可重建的数据源
- 若从未调用过 `setItemsFromValue`，`refreshItems()` 为空操作（仅更新内容高度）

> **注意：** 模板绑定路径（`bind:items`）由 `TemplateScreen::tick()` → `updateBindings()` 每帧自动刷新，无需手动调用 `refreshItems()`。该方法主要用于直接 C++ 调用场景（如替换 ItemFactory 后强制重建）。

## 物品槽组件 (SlotWidget)

显示物品槽的组件：

```cpp
#include "kagero/widget/SlotWidget.hpp"

auto slot = std::make_unique<SlotWidget>("slot_0", 10, 10);
slot->setSize(32, 32);

// 设置物品
slot->setItem(itemStack);

// 获取物品
const ItemStack* item = slot->item();

// 设置槽索引
slot->setSlotIndex(0);

// 回调
slot->setOnSlotClick([](i32 slotIndex, i32 button, bool shiftHeld) {
    std::cout << "Slot " << slotIndex << " clicked" << std::endl;
});
```

在模板中使用 SlotWidget：

```xml
<slot id="slot_0" index="5" size="18,18"/>
```

模板属性：

| 属性 | 说明 |
|------|------|
| `index` | 槽索引，对应 `slot->slotIndex()`。非数字或缺失时为默认值 -1（表示未绑定到具体槽位）；解析失败时也回退到 -1。`index=0` 是合法值。 |

> 模板中显式指定 `index` 后，槽位点击回调收到的 `slotIndex` 即为该值，便于和上游容器（如 Inventory）的槽位下标对齐。

## 3D 视口组件 (Viewport3DWidget)

显示 3D 内容的视口：

```cpp
#include "kagero/widget/Viewport3DWidget.hpp"

auto viewport = std::make_unique<Viewport3DWidget>(
    "viewport_3d",
    10, 10, 400, 300
);

// 设置相机
viewport->setCameraPosition(glm::vec3(0, 10, 20));
viewport->setCameraRotation(-30.0f, 0.0f);  // pitch, yaw

// 设置渲染场景
viewport->setScene(scene);

// 设置背景颜色
viewport->setBackgroundColor(Colors::fromARGB(255, 100, 149, 237));
```

## PaintContext

组件绘制的抽象层：

```cpp
#include "kagero/paint/PaintContext.hpp"

void MyWidget::paint(PaintContext& ctx) {
    // 绘制填充矩形
    ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 60, 60, 60));
    
    // 绘制边框
    ctx.drawBorder(bounds(), 1.0f, Colors::WHITE);
    
    // 绘制文本
    ctx.drawText(m_text, x(), y(), m_color);
    ctx.drawTextCentered(m_text, bounds(), m_color);
    
    // 绘制图片
    ctx.drawImage(m_image, bounds());
    
    // 绘制圆角矩形
    ctx.drawRoundedRect(bounds(), 4.0f, m_color);
    
    // 绘制渐变
    ctx.drawGradientRect(bounds(), m_startColor, m_endColor, Direction::Vertical);
    
    // 裁剪
    ctx.pushClip(bounds());
    // ... 绘制内容
    ctx.popClip();
}
```

## 最佳实践

### 1. 组件 ID 命名规范

```cpp
// 使用有意义的 ID
"btn_submit"           // 按钮
"txt_username"         // 文本输入框
"lbl_title"            // 标签文本
"chk_remember"         // 复选框
"sldr_volume"          // 滑块
"lst_items"            // 列表
```

### 2. 组件生命周期

```cpp
class MyScreen {
    std::unique_ptr<Widget> m_root;
    ButtonWidget* m_submitBtn;  // 不拥有
    
    void init() {
        auto container = std::make_unique<ContainerWidget>("root");
        
        auto btn = std::make_unique<ButtonWidget>(...);
        m_submitBtn = btn.get();  // 保存裸指针引用
        
        container->addChild(std::move(btn));
        m_root = std::move(container);
    }
    
    void update() {
        if (m_submitBtn) {
            // 更新按钮状态
        }
    }
};
```

### 3. 事件处理

```cpp
class MyButton : public ButtonWidget {
public:
    using ButtonWidget::ButtonWidget;
    
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        if (!isActive() || !isVisible()) return false;
        
        // 自定义点击处理
        playClickSound();
        
        return ButtonWidget::onClick(mouseX, mouseY, button);
    }
    
protected:
    void playClickSound() {
        // 播放音效
    }
};
```

### 4. 样式管理

```cpp
// 定义全局样式
namespace styles {
    ButtonWidget::Style primaryButton() {
        ButtonWidget::Style style;
        style.normalColor = Colors::fromARGB(255, 60, 120, 200);
        style.hoverColor = Colors::fromARGB(255, 80, 140, 220);
        style.textColor = Colors::WHITE;
        style.cornerRadius = 4;
        return style;
    }
    
    ButtonWidget::Style secondaryButton() {
        ButtonWidget::Style style;
        style.normalColor = Colors::fromARGB(255, 80, 80, 80);
        style.hoverColor = Colors::fromARGB(255, 100, 100, 100);
        style.textColor = Colors::WHITE;
        return style;
    }
}

// 使用样式
auto btn = std::make_unique<ButtonWidget>(...);
btn->setStyle(styles::primaryButton());
```

### 5. 响应式更新

```cpp
class HealthBarWidget : public Widget {
public:
    HealthBarWidget(std::string id, Reactive<i32>& health, Reactive<i32>& maxHealth)
        : Widget(std::move(id))
        , m_health(health)
        , m_maxHealth(maxHealth)
    {
        // 观察状态变化
        m_healthObserver = std::make_unique<AutoObserver<i32>>(
            m_health, [this](const i32&) { markDirty(); }
        );
        m_maxHealthObserver = std::make_unique<AutoObserver<i32>>(
            m_maxHealth, [this](const i32&) { markDirty(); }
        );
    }

    void paint(PaintContext& ctx) override {
        if (!isVisible()) return;

        f32 ratio = static_cast<f32>(m_health.get()) / m_maxHealth.get();
        // 绘制血条...
    }

private:
    Reactive<i32>& m_health;
    Reactive<i32>& m_maxHealth;
    std::unique_ptr<AutoObserver<i32>> m_healthObserver;
    std::unique_ptr<AutoObserver<i32>> m_maxHealthObserver;
};
```
