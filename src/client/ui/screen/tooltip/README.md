# Tooltip 渲染模块

本目录存放容器屏幕中专用 tooltip 渲染器，对应 MC 1.21.11 的 `net.minecraft.client.gui.screens.inventory.tooltip` 包。

## 目录结构

```text
src/client/ui/screen/tooltip/
├── README.md                            # 本文档
├── BundleTooltipRenderer.hpp            # 收纳袋 tooltip 渲染器（声明）
├── BundleTooltipRenderer.cpp            # 布局算法实现（无渲染依赖，可单独链接到测试）
└── BundleTooltipRendererRender.cpp       # 渲染主入口实现（依赖 GuiRenderer + ItemRenderer）
```

## 文件拆分原因

`BundleTooltipRenderer.cpp` 仅包含布局算法（slotCount、gridSizeY、progressBarFill 等），
不依赖 `ItemRenderer`，因此可以单独链接到 `mc_tests` 用于单元测试。

`BundleTooltipRendererRender.cpp` 包含 `render` 方法实现，调用 `ItemRenderer::renderItem`，
依赖完整的客户端渲染栈，仅在 `mc_client` 中构建。

## 模块职责

普通物品 tooltip 由 `AbstractContainerScreen::renderItemTooltip` / `CreativeScreen::_renderItemTooltip` 直接渲染文本行；
某些物品（如收纳袋）需要更复杂的图形 tooltip（图标网格、进度条），由本目录的渲染器负责。

- **BundleTooltipRenderer**：对应 MC `ClientBundleTooltip`，渲染收纳袋内容物网格 + 满度进度条 + 溢出指示

## 内部模块关系

```
┌──────────────────────────────────┐
│ AbstractContainerScreen          │
│   renderItemTooltip(stack, ...)  │
└──────────────┬───────────────────┘
               │
               ├─ BundleItem::isBundleItem(stack)?
               │     │
               │     ▼
               │   ┌────────────────────────────────┐
               │   │ BundleTooltipRenderer::render  │
               │   │   - 物品网格（4列×N行 24×24px） │
               │   │   - 进度条（96×13px）           │
               │   │   - +N 溢出指示                 │
               │   └────────────────────────────────┘
               │
               └─ 否：渲染普通文本 tooltip
                     │
                     ▼
                   调用 Item::addInformation(stack, world, lines, false)
                   以附加物品自定义文本（锻造模板、旗帜图案等）
```

## 外部依赖

| 模块 | 用途 |
|------|------|
| `client/renderer/trident/gui/GuiRenderer` | 矩形 / 文本绘制原语 |
| `client/renderer/trident/item/ItemRenderer` | 物品图标渲染 |
| `common/item/items/special/bundle/BundleContents` | 收纳袋内容物数据 |
| `common/item/items/special/bundle/BundleItem` | isBundleItem 判定（由调用方调用） |

## 容易踩的坑

- **ClientWorld 不继承 IWorld**：客户端渲染时无法直接通过 `player->world()` 拿到 `IWorld*`。`AbstractContainerScreen::renderItemTooltip` 调用 `Item::addInformation` 时，若 `m_menu->getPlayerInventory()->getPlayer()->world()` 为空，必须跳过 addInformation 调用（基类实现本身是空操作，子类实现也不依赖 world）。
- **网格填充顺序**：MC 从右下角向左上角填充（最新插入的物品在最右下），与正常阅读顺序相反。`BundleTooltipRenderer::render` 严格按 MC 算法 1-based 索引迭代。
- **numberOfItemsToShow vs slotCount**：`slotCount = min(12, size)` 控制 *网格槽位数*；`numberOfItemsToShow` 控制 *实际显示的物品数*（>12 项时返回 8，留一格给 "+N"）。两者不要混淆。
- **进度条填充宽度**：使用整数权重 `weight * 94 / MAX_WEIGHT`，避免浮点误差。MC 的 `Mth.mulAndTruncate(Fraction, 94)` 等价于 `(numerator * 94) / denominator`。
- **边框颜色差异**：默认容器屏幕用紫色边框 `0x505000FF`，创造屏幕用蓝色边框 `0xFF4DA3FF`。`BundleTooltipRenderer::render` 接受 `borderColor` 参数以适配两种场景。
