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

## 测试覆盖说明

- **布局算法（已覆盖）**：`tests/client/ui/screen/tooltip/BundleTooltipRendererTest.cpp` 提供 43 项单元测试，覆盖 `slotCount`、`gridSizeY`、`itemGridHeight`、`progressBarFill`、`amountOfHiddenItems`、`tooltipHeight`、`positionTooltip` 以及所有布局/颜色常量，与 MC 1.21.11 `ClientBundleTooltip` 算法逐一对齐。
- **render 方法（未单元测试）**：`render` 依赖 `GuiRenderer` 和 `ItemRenderer`，两者均为具体类（非虚接口），项目未引入 `IGuiRenderer` / `IItemRenderer` 抽象层，无法用 gmock 桩替换。强行为 render 方法写单元测试需要先重构整个客户端渲染栈的接口抽象，超出本 TODO 范围。

  **TODO**：待客户端渲染栈引入 `IGuiRenderer` / `IItemRenderer` 虚接口（或采用 fixture 截屏比对方案）后，补齐 render 方法的单元测试，验证：
  1. 空收纳袋渲染路径（空描述文本 + 空进度条）
  2. 非空收纳袋渲染路径（网格 + 进度条 + 数量文本）
  3. 满收纳袋 "Full" 文本
  4. >12 项时 "+N" 溢出指示
  5. 选中项高亮（背景 + 前景两层）
  6. 边框颜色参数（默认紫 vs 创造蓝）

  当前 render 方法的正确性由布局算法单元测试 + 人工视觉验证保证。

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

- **ClientWorld 不继承 IWorld**：客户端渲染时 `player->world()` 返回 nullptr（`ClientWorld` 不继承 `IWorld`，客户端 `Player` 构造时也未传入 world）。`Item::addInformation` 的 `world` 参数为 `IWorld*`（可空），对应 MC 1.21.11 `Item.TooltipContext.of(null)` 返回的 EMPTY 上下文。调用方（`AbstractContainerScreen`、`CreativeScreen`）始终调用 `addInformation`，由各 `Item` 子类自行判断 `world != nullptr` 后再访问世界数据（如 `FilledMapItem` 在 world 为空时跳过缩放级别提示）。其他子类（`SmithingTemplateItem`、`PotterySherdItem`、`BannerPatternItem`、`BannerItem`）不依赖 world，world 为空时仍正常添加翻译文本。
- **网格填充顺序**：MC 从右下角向左上角填充（最新插入的物品在最右下），与正常阅读顺序相反。`BundleTooltipRenderer::render` 严格按 MC 算法 1-based 索引迭代。
- **numberOfItemsToShow vs slotCount**：`slotCount = min(12, size)` 控制 *网格槽位数*；`numberOfItemsToShow` 控制 *实际显示的物品数*（>12 项时返回 8，留一格给 "+N"）。两者不要混淆。
- **进度条填充宽度**：使用整数权重 `weight * 94 / MAX_WEIGHT`，避免浮点误差。MC 的 `Mth.mulAndTruncate(Fraction, 94)` 等价于 `(numerator * 94) / denominator`。
- **边框颜色差异**：默认容器屏幕用紫色边框 `0x505000FF`，创造屏幕用蓝色边框 `0xFF4DA3FF`。`BundleTooltipRenderer::render` 接受 `borderColor` 参数以适配两种场景。

## TODO：升级到纹理化渲染

当前 `BundleTooltipRenderer::render` 使用 `GuiRenderer` 的纯色矩形（`fillRect`/`drawRect`）渲染所有视觉元素，与 `AbstractContainerScreen::renderItemTooltip` 的现有风格一致。MC 1.21.11 原版 `ClientBundleTooltip` 使用以下 6 个 sprite 纹理：

| MC sprite | 用途 | 当前替代 |
|-----------|------|----------|
| `SLOT_HIGHLIGHT_BACK_SPRITE` | 选中项背景高亮 | `SELECTED_BACK_COLOR = 0x80FFFFFF` |
| `SLOT_HIGHLIGHT_FRONT_SPRITE` | 选中项前景高亮边框 | `SELECTED_FRONT_COLOR = 0x60000000` |
| `BUNDLE_OVERLAY_SPRITE` | 网格槽位背景纹理 | `SLOT_BACKGROUND_COLOR = 0x40FFFFFF` |
| `EMPTY_BUNDLE_OVERLAY_SPRITE` | 空收纳袋背景纹理 | 纯色背景 + "Bundle is empty" 文本 |
| `PROGRESS_BAR_EMPTY_SPRITE` | 进度条空背景纹理 | `PROGRESSBAR_BG_COLOR = 0x40000000` |
| `PROGRESS_BAR_FULL_SPRITE` | 进度条满填充纹理 | `PROGRESSBAR_FILL_COLOR` / `PROGRESSBAR_FULL_COLOR` |

**TODO**：待 `GuiTextureManager` 支持加载上述 sprite 纹理后，将 `BundleTooltipRenderer::render` 升级为纹理化渲染，以完全复刻 MC 原版视觉。升级时需保持当前纯色方案作为纹理不可用时的回退（参考 `FurnaceScreen` 的纹理回退模式）。
