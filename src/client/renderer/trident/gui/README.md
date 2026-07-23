# GUI 模块

Trident 渲染引擎的 GUI 子系统，负责容器屏幕纹理管理、精灵图集、文字渲染等。

## 目录结构

```text
gui/
├── GuiRenderer.hpp/cpp          # GUI 渲染器（矩形、纹理矩形、文字绘制）
├── GuiTextureManager.hpp/cpp    # GUI 容器纹理管理器（多容器纹理架构）
├── GuiTextureAtlas.hpp/cpp      # GUI 纹理图集
├── GuiTextureLoader.hpp/cpp     # GUI 纹理加载器
├── GuiSprite.hpp                # GUI 精灵定义
├── GuiSpriteAtlas.hpp/cpp       # GUI 精灵图集
├── GuiSpriteManager.hpp/cpp     # GUI 精灵管理
├── GuiSpriteParser.hpp/cpp      # GUI 精灵解析
├── GuiSpriteRegistry.hpp/cpp    # GUI 精灵注册表
└── GuiAtlasRegistry.hpp         # 多图集注册表
```

## 核心组件

### GuiTextureManager — 容器纹理管理器

统一管理所有 GUI 容器纹理的加载、缓存和渲染。采用**多容器纹理架构**，每种容器类型拥有独立的 Vulkan 资源和图集槽位。

#### 多容器纹理架构

每种容器类型（背包、工作台、熔炉）拥有独立的 `ContainerTextureEntry`：

```cpp
struct ContainerTextureEntry {
    VkImage image;              // Vulkan 图像
    VkDeviceMemory imageMemory; // 图像内存
    VkImageView imageView;      // 图像视图
    VkSampler sampler;          // 采样器
    u32 width = 256;            // 纹理宽度
    u32 height = 256;           // 纹理高度
    u8 atlasSlot = 255;         // 图集槽位（255 = 未注册）
    bool loaded = false;        // 加载状态
};
```

加载流程：
1. `initialize()` — 初始化 Vulkan 设备引用
2. `loadInventoryTexture()` — 加载背包纹理（`minecraft:textures/gui/container/inventory.png`）
3. `loadCraftingTableTexture()` — 加载工作台纹理（`minecraft:textures/gui/container/crafting_table.png`），失败时回退到背包纹理
4. `loadFurnaceTexture()` — 加载熔炉纹理（`minecraft:textures/gui/container/furnace.png`），失败时使用程序化生成纹理
5. `registerToRenderer()` — 将所有已加载纹理注册到 GuiRenderer 的多图集槽位

纹理资源不存在时，使用程序化生成的默认纹理（`_createDefaultContainerTexture` / `_createDefaultFurnaceTexture`），确保无资源包时 GUI 也能正常显示。

#### ContainerTex UV 坐标常量

所有 MC 容器纹理基于 256×256 像素纹理，GUI 可见区域为左上角 176×166 像素。`ContainerTex` 命名空间定义了精确的 UV 坐标：

| 常量 | 像素坐标 | 说明 |
|------|----------|------|
| `INVENTORY_BG_*` | (0,0)-(176,166) | 背包背景 |
| `CRAFTING_TABLE_BG_*` | (0,0)-(176,166) | 工作台背景 |
| `FURNACE_BG_*` | (0,0)-(176,166) | 熔炉背景 |
| `FURNACE_LIT_*` | (176,0)-(190,14) | 熔炉火焰指示器（14×14） |
| `FURNACE_ARROW_*` | (176,14)-(200,30) | 熔炉进度箭头（24×16） |

熔炉屏幕坐标常量（相对于 GUI 左上角）：
- `FURNACE_LIT_SCREEN_X/Y = (56, 36)` — 火焰指示器位置
- `FURNACE_ARROW_SCREEN_X/Y = (79, 34)` — 进度箭头位置

#### GuiColors 颜色常量

用于程序化默认纹理生成和纯色后备绘制：

| 常量 | 值 | 说明 |
|------|----|------|
| `CONTAINER_BG` | 0xFFC6C6C6 | 浅灰背景 |
| `CONTAINER_BORDER` | 0xFF555555 | 深灰边框 |
| `SLOT_BG` | 0xFF8B8B8B | 槽位背景 |
| `SLOT_BORDER` | 0xFF373737 | 槽位边框 |
| `DEFAULT_BG` | 0xFF404040 | 默认背景 |
| `FURNACE_FIRE_FILL` | 0xFFFFAA00 | 火焰填充（橙色） |
| `FURNACE_ARROW_FILL` | 0xFFC6C6C6 | 箭头填充（浅灰） |

#### 熔炉动画绘制

`drawFurnaceLitProgress()` 和 `drawFurnaceBurnProgress()` 实现了与 MC Java 一致的动画逻辑：

- **火焰指示器**：从底部向上填充，可见高度 = `ceil(litProgress × 13.0) + 1`（范围 1~14 像素）
- **进度箭头**：从左向右填充，可见宽度 = `ceil(burnProgress × 24.0)`（范围 0~24 像素）

### GuiRenderer — GUI 渲染器

提供基础绘制原语：`fillRect`、`drawRect`、`drawTexturedRect`、`drawText` 等。支持多图集槽位绑定，不同容器纹理通过槽位 ID 区分。

### 精灵系统

精灵系统（`GuiSprite*` / `GuiSpriteAtlas` / `GuiSpriteRegistry`）处理 MC 的精灵动画（如水流动画、岩浆动画），与容器纹理系统独立运作。

## 上下游依赖关系

### 上游依赖

| 模块 | 用途 |
|------|------|
| `common/resource` | 资源管理器（纹理文件加载） |
| `client/renderer/trident/core` | Vulkan 设备、命令池、队列 |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `client/ui/screen` | 容器屏幕（FurnaceScreen、CartographyScreen 等） |
| `client/application/ClientApplicationBootstrap` | 初始化时加载所有容器纹理 |

## 容易踩的坑

- **每种容器类型有独立图集槽位**：调用 `drawFurnaceBackground` 前必须已调用 `loadFurnaceTexture()` 并 `registerToRenderer()`，否则纹理不会显示。
- **回退机制**：`loadCraftingTableTexture()` 失败时回退到共享背包纹理的 imageView/sampler；`loadFurnaceTexture()` 失败时使用程序化生成纹理，而非共享背包纹理。
- **UV 坐标基于 256×256**：所有 `ContainerTex` UV 常量以 256×256 纹理尺寸为基准，修改纹理尺寸时需同步更新。
- **程序化纹理质量**：默认生成的纹理是简化纯色矩形，仅用于无资源包时的后备显示。
