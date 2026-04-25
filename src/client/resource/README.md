# client/resource 模块

客户端资源加载模块，负责从资源包加载和管理方块模型、方块状态、纹理图集等渲染所需资源。

## 目录结构

```
src/client/resource/
├── ResourceManager.hpp/cpp      # 资源管理器（核心入口）
├── BlockModelLoader.hpp/cpp     # 方块模型加载器
├── BlockStateLoader.hpp/cpp     # 方块状态加载器
├── BlockModelCache.hpp/cpp      # 方块模型缓存
├── ItemModelLoader.hpp/cpp      # 物品模型加载器
├── ItemModelCache.hpp/cpp       # 物品模型缓存
├── TextureAtlasBuilder.hpp/cpp  # 纹理图集构建器
├── ItemTextureAtlas.hpp/cpp     # 物品纹理图集
├── DestroyStageTextures.hpp/cpp # 破坏阶段纹理
└── EntityTextureLoader.hpp/cpp  # 实体纹理加载器
```

## 文件详解

### ResourceManager.hpp/cpp

**职责**：资源管理器是整个模块的核心入口，统一管理资源包、模型、方块状态和纹理。它协调其他加载器的工作流程。

**主要类型**：

```cpp
// 方块外观信息（渲染所需的所有数据）
struct BlockAppearance {
    std::vector<ModelElement> elements;           // 模型元素列表
    std::map<String, TextureRegion> faceTextures; // 方向 -> 纹理区域映射
    i32 xRotation = 0;                            // X轴旋转角度
    i32 yRotation = 0;                            // Y轴旋转角度
    bool uvLock = false;                          // UV锁定
};

// 解码后的纹理数据
struct DecodedTexture {
    std::vector<u8> pixels; // RGBA8 像素数据
    u32 width = 0;
    u32 height = 0;
};
```

**主要功能**：

- `addResourcePack()` - 添加资源包
- `loadAllResources()` - 加载所有资源（方块状态、模型）
- `buildTextureAtlas()` - 构建纹理图集
- `getBlockAppearance()` - 获取方块外观
- `getTextureRegion()` - 获取纹理区域
- `loadTextureRGBA()` - 加载并解码纹理
- `reload()` - 重新加载所有资源

**关键实现细节**：

1. 资源包按添加顺序优先级处理，后添加的优先级更高
2. 使用 `TextureMapper` 兼容层处理 MC 1.12/1.13+ 的纹理路径差异
3. 构建 `BlockAppearance` 需要先构建纹理图集（依赖纹理区域数据）

---

### BlockModelLoader.hpp/cpp

**职责**：解析和加载方块模型 JSON 文件，支持模型继承（parent）和纹理变量解析。

**主要类型**：

```cpp
// 模型面UV数据
struct ModelFaceUV {
    f32 u0 = 0.0f, v0 = 0.0f, u1 = 16.0f, v1 = 16.0f;
    i32 rotation = 0; // 0, 90, 180, 270
};

// 模型面数据
struct ModelFace {
    String texture;                     // "#all" 或纹理路径
    Direction cullFace = Direction::None;
    i32 tintIndex = -1;
    ModelFaceUV uv;
};

// 模型元素（对应JSON中的elements数组）
struct ModelElement {
    glm::vec3 from{0.0f, 0.0f, 0.0f};   // 起始坐标 (0-16)
    glm::vec3 to{16.0f, 16.0f, 16.0f};  // 结束坐标 (0-16)
    std::map<Direction, ModelFace> faces;
    ModelRotation rotation;
    bool shade = true;
};

// 未烘焙的方块模型
struct UnbakedBlockModel {
    ResourceLocation parentLocation;    // 父模型
    std::vector<ModelElement> elements;
    std::map<String, String> textures;  // 纹理变量 -> 路径
    bool ambientOcclusion = true;
};

// 已烘焙的方块模型（所有纹理路径已解析）
struct BakedBlockModel {
    std::vector<ModelElement> elements;
    std::map<String, ResourceLocation> textures;
    bool ambientOcclusion = true;

    ResourceLocation resolveTexture(StringView textureRef) const;
};
```

**主要功能**：

- `loadFromResourcePack()` - 从资源包扫描模型文件
- `loadModel()` - 加载单个模型
- `bakeModel()` - 烘焙模型（解析父模型链和纹理变量）

**模型加载流程**：

1. 解析模型 JSON 文件
2. 递归加载父模型链
3. 从根到叶合并纹理变量和元素
4. 解析 `#variable` 形式的纹理引用

**关键实现细节**：

- 模型坐标系统：0-16 像素单位（MC 标准）
- UV 坐标自动计算：当未指定 UV 时，根据面的方向和元素尺寸计算
- 支持模型继承链，子模型可覆盖父模型的纹理和元素

---

### BlockStateLoader.hpp/cpp

**职责**：解析方块状态 JSON 文件（`blockstates/*.json`），管理方块状态到模型的映射。

**主要类型**：

```cpp
// 方块状态变体
struct BlockStateVariant {
    ResourceLocation model;  // 模型位置
    i32 x = 0;               // X轴旋转 (0, 90, 180, 270)
    i32 y = 0;               // Y轴旋转
    bool uvLock = false;     // UV锁定
    i32 weight = 1;          // 权重（随机选择）
};

// 变体列表
struct VariantList {
    std::vector<BlockStateVariant> variants;
    const BlockStateVariant& select() const;      // 随机选择
    const BlockStateVariant& select(u64 seed) const;
};

// 方块状态定义
class BlockStateDefinition {
    // 从JSON解析
    static Result<BlockStateDefinition> parse(StringView jsonContent);

    // 获取变体：stateStr格式为 "axis=y,facing=north" 或 "normal"
    const VariantList* getVariants(StringView stateStr) const;
};
```

**主要功能**：

- `loadFromResourcePack()` - 从资源包加载所有方块状态
- `getBlockState()` - 获取方块状态定义
- `getVariant()` - 根据属性获取模型变体

**方块状态 JSON 格式**：

```json
{
  "variants": {
    "axis=y": { "model": "minecraft:block/oak_log" },
    "axis=x": { "model": "minecraft:block/oak_log_horizontal", "x": 90 },
    "axis=z": { "model": "minecraft:block/oak_log_horizontal", "x": 90, "y": 90 }
  }
}
```

---

### BlockModelCache.hpp/cpp

**职责**：连接 BlockRegistry 和 ResourceManager，缓存 BlockState -> BlockAppearance 映射。参考 MC 1.16.5 的 BlockModelShapes 类。

**主要功能**：

- `initialize()` - 初始化缓存，遍历所有方块状态构建外观映射
- `rebuild()` - 重建缓存（资源包变更后调用）
- `getBlockAppearance()` - 根据方块状态获取外观（多重重载）

**缓存策略**：

- 使用 `unordered_map<u32, const BlockAppearance*>` 按状态ID缓存
- `getBlockAppearance(const BlockState*)` 会优先通过 `stateId` 命中缓存，区块网格生成阶段不会重复解析属性字符串
- `toModelKey()` 只在构建缓存或必要的回退路径中使用
- 未找到外观时返回 `m_missingAppearance`（紫黑方块）
- 缺失模型外观：6面使用第一个纹理图集位置的UV坐标

**使用示例**：

```cpp
ResourceManager rm;
rm.addResourcePack(pack);
rm.loadAllResources();
rm.buildTextureAtlas();

BlockModelCache cache;
cache.initialize(rm);

// 获取方块外观
const BlockState* state = block->defaultState();
const BlockAppearance* appearance = cache.getBlockAppearance(state);
```

---

### TextureAtlasBuilder.hpp/cpp

**职责**：将多个纹理打包到一个大图集中，减少纹理切换，提高渲染效率。

**主要类型**：

```cpp
// 纹理图集构建结果
struct AtlasBuildResult {
    std::vector<u8> pixels;                           // RGBA8像素数据
    u32 width = 0;                                    // 图集宽度
    u32 height = 0;                                   // 图集高度
    std::map<ResourceLocation, TextureRegion> regions; // 纹理位置映射
};

// 纹理图集构建器
class TextureAtlasBuilder {
    void setMaxSize(u32 width, u32 height);           // 设置最大尺寸
    void setPadding(u32 padding);                      // 设置边距
    Result<void> addTexture(IResourcePack& pack, const ResourceLocation& loc);
    void addTexture(const ResourceLocation& loc, const std::vector<u8>& pixels, u32 w, u32 h);
    Result<AtlasBuildResult> build();                 // 构建图集
};
```

**打包算法**：

使用 Skyline 算法进行矩形打包：

1. 按面积从大到小排序纹理
2. 维护天际线（已放置纹理的顶部轮廓）
3. 在最低可放置位置放置纹理
4. 动态扩展图集尺寸（2的幂次，最大4096）

**关键实现细节**：

- 自动计算最小可行尺寸（从64x64开始，按2的幂次扩展）
- 支持 PNG 解码（使用 stb_image）
- 避免重复添加相同纹理

---

### ItemTextureAtlas.hpp/cpp

**职责**：管理非方块物品的纹理图集（工具、食物、材料等）。方块物品使用方块纹理图集。

**主要功能**：

- `create()` - 创建 Vulkan 图像、视图、采样器
- `loadFromResourcePacks()` - 加载物品纹理
- `upload()` - 上传到GPU
- `getItemTexture()` - 获取纹理区域

**纹理路径搜索顺序**：

1. `textures/item/<item>.png` (MC 1.13+)
2. `textures/items/<item>.png` (MC 1.12)
3. 对于方块物品，回退到 `textures/block/<block>.png`

**关键实现细节**：

- 支持 Vulkan 纹理上传（使用暂存缓冲区）
- 自动缩放大尺寸纹理（最大64x64）
- 使用 TextureMapper 兼容 MC 1.12/1.13+ 路径

---

### DestroyStageTextures.hpp/cpp

**职责**：管理10个破坏阶段纹理（destroy_stage_0 ~ destroy_stage_9），用于方块挖掘效果。

**主要常量**：

```cpp
static constexpr size_t STAGE_COUNT = 10;    // 10个阶段
static constexpr u32 TEXTURE_SIZE = 16;       // 16x16像素
```

**主要功能**：

- `initialize()` - 初始化，从资源包加载或程序生成
- `getTextureData()` - 获取指定阶段纹理数据
- `getTextureUV()` - 获取纹理UV坐标
- `getAtlasData()` - 获取合并图集数据

**图集布局**：2行5列，每个纹理16x16像素

**纹理格式转换**：

- 原版纹理：灰度图，白色=裂纹可见
- 着色器期望：RGB=(0,0,0)，Alpha=裂纹强度
- 转换公式：`crackIntensity = 255 - luminance`

**程序生成纹理**：

当资源包中没有纹理时，使用确定性随机算法生成裂纹图案：

- 水平裂纹（阶段2+）
- 垂直裂纹（阶段3+）
- 对角裂纹（阶段6+）
- 随机裂纹点
- 边缘破损

---

### EntityTextureLoader.hpp/cpp

**职责**：从资源包加载实体纹理并构建纹理图集。自动从 EntityRegistry 获取需要纹理的实体列表。

**自动发现机制**：

1. 遍历 `EntityRegistry::getAllTypes()` 获取所有注册实体
2. 根据 `EntityClassification` 过滤需要纹理的实体类型：
   - `Creature` - 动物（猪、牛、羊等）
   - `WaterCreature` - 水生生物（鱿鱼、海豚等）
   - `WaterAmbient` - 水生环境生物（鱼类）
   - `Ambient` - 环境生物（蝙蝠）
   - `Monster` - 怪物（僵尸、骷髅等）
3. 根据实体名称推断纹理路径

**纹理路径搜索顺序**：

1. 特殊路径映射表（如 `player` -> `entity/steve.png`, `entity/alex.png` 等）
2. MC 1.13+ 格式：`textures/entity/<name>/<name>.png`
3. MC 1.12 格式：`textures/entity/<name>.png`

**主要功能**：

- `loadAllEntityTextures()` - 从资源包列表加载所有实体纹理（推荐）
- `loadDefaultTextures()` - 单资源包加载（向后兼容）
- `needsTexture()` - 判断实体分类是否需要纹理
- `getTexturePaths()` - 获取实体的纹理路径列表

**附加纹理**：

某些实体需要多个纹理文件（如羊需要 `sheep.png` 和 `sheep_fur.png`），通过 `ADDITIONAL_TEXTURES` 映射表处理。

---

## 模块关系图

```
┌─────────────────────────────────────────────────────────────┐
│                     ResourceManager                          │
│  (核心入口：协调所有加载器)                                    │
└─────────────────────────────────────────────────────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│ BlockStateLoader│  │ BlockModelLoader│  │TextureAtlasBuilder│
│ (blockstates/)  │  │ (models/block/) │  │ (纹理打包)        │
└─────────────────┘  └─────────────────┘  └─────────────────┘
          │                    │                    │
          └────────────────────┼────────────────────┘
                               ▼
                    ┌─────────────────────┐
                    │   BlockModelCache   │
                    │ (状态->外观缓存)     │
                    └─────────────────────┘
                               │
          ┌────────────────────┼────────────────────┐
          ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│ ItemTextureAtlas│  │DestroyStageTextures│ EntityTextureLoader│
│ (物品纹理)       │  │ (破坏纹理)        │ (实体纹理)          │
└─────────────────┘  └─────────────────┘  └─────────────────┘
```

---

## 模块整体职责

**核心职责**：

1. **资源包管理**：支持多个资源包叠加，后添加的优先级更高
2. **模型加载**：解析方块模型 JSON，支持继承和纹理变量
3. **方块状态**：解析方块状态 JSON，管理状态到模型映射
4. **纹理图集**：打包多个纹理到大图集，优化渲染性能
5. **缓存管理**：缓存方块外观，避免重复计算
6. **兼容性处理**：支持 MC 1.12 和 MC 1.13+ 资源包格式

---

## 输入和输出

### 输入

| 来源 | 数据 |
|------|------|
| IResourcePack | 资源包抽象接口（文件夹/ZIP/内存） |
| blockstates/*.json | 方块状态定义 |
| models/block/*.json | 方块模型定义 |
| textures/block/*.png | 方块纹理 |
| textures/item/*.png | 物品纹理 |
| textures/entity/*.png | 实体纹理 |

### 输出

| 类型 | 数据 |
|------|------|
| BlockAppearance | 方块渲染所需的所有数据（元素、纹理、旋转） |
| AtlasBuildResult | 纹理图集（像素数据+UV映射） |
| BakedBlockModel | 已烘焙的方块模型 |
| TextureRegion | 纹理在图集中的UV坐标 |

---

## 依赖项

### 内部依赖

- `common/core/Types.hpp` - 基础类型
- `common/core/Result.hpp` - 错误处理
- `common/resource/` - 资源包接口、ResourceLocation
- `common/resource/compat/TextureMapper.hpp` - 纹理路径兼容
- `common/world/block/Block.hpp` - 方块注册表
- `common/item/` - 物品注册表
- `renderer/MeshTypes.hpp` - TextureRegion 定义

### 外部依赖

- `nlohmann/json` - JSON 解析
- `stb_image.h` - PNG 解码
- `spdlog` - 日志
- `glm` - 数学库
- `Vulkan` - GPU 资源（ItemTextureAtlas）

---

## 使用方法

### 基本使用流程

```cpp
// 1. 创建资源管理器
ResourceManager resourceManager;

// 2. 添加资源包
auto pack = std::make_shared<FolderResourcePack>("resourcepacks/default");
resourceManager.addResourcePack(pack);

// 3. 加载所有资源
resourceManager.loadAllResources();

// 4. 构建纹理图集
auto atlasResult = resourceManager.buildTextureAtlas();

// 5. 初始化模型缓存
BlockModelCache modelCache;
modelCache.initialize(resourceManager);

// 6. 获取方块外观
const BlockAppearance* appearance =
    modelCache.getBlockAppearance(blockState);
```

### 物品纹理图集

```cpp
ItemTextureAtlas itemAtlas;
itemAtlas.create(device, physicalDevice, commandPool, graphicsQueue);
itemAtlas.loadFromResourcePacks(resourcePacks);
itemAtlas.upload();

// 获取物品纹理
const TextureRegion* region = itemAtlas.getItemTexture(itemId);
```

### 破坏阶段纹理

```cpp
DestroyStageTextures::instance().initialize(&resourceManager);

// 获取破坏阶段UV
f32 u0, v0, u1, v1;
DestroyStageTextures::instance().getTextureUV(stage, u0, v0, u1, v1);
```

---

## 容易踩的坑

### 1. 资源包加载顺序

**问题**：后添加的资源包优先级更高，这可能导致纹理被意外覆盖。

**解决**：按优先级从低到高添加资源包（vanilla -> mod -> user）。

### 2. 纹理路径兼容性

**问题**：MC 1.12 使用 `textures/blocks/`，MC 1.13+ 使用 `textures/block/`。

**解决**：ResourceManager 使用 `TextureMapper::getPathVariants()` 自动尝试所有路径变体。

### 3. 模型烘焙依赖

**问题**：`BlockAppearance` 需要 `TextureRegion`，必须在 `buildTextureAtlas()` 之后调用 `computeBlockAppearances()`。

**解决**：ResourceManager 内部已正确处理调用顺序。

### 4. 方块状态属性字符串格式

**问题**：属性字符串必须按字母顺序排列，否则无法匹配。

**解决**：BlockStateLoader 使用 `propertiesToStateStr()` 自动排序。

### 5. 纹理图集尺寸限制

**问题**：默认最大 4096x4096，大量纹理可能无法放入。

**解决**：使用 `TextureAtlasBuilder::setMaxSize()` 调整尺寸，或使用图集数组。

### 6. ItemTextureAtlas GPU 上传

**问题**：`upload()` 必须在 Vulkan 设备就绪后调用，且需要正确的命令池和队列。

**解决**：确保在 `create()` 后调用 `upload()`，并传入有效的命令池和图形队列。

### 7. 破坏纹理格式转换

**问题**：原版破坏纹理是灰度图，需要转换为着色器期望的格式。

**解决**：DestroyStageTextures 自动处理转换：`crackIntensity = 255 - luminance`。

### 8. 模型继承循环

**问题**：模型父子关系可能存在循环引用，导致无限递归。

**解决**：BlockModelLoader::bakeModel() 限制最大迭代次数为10次。

---

## 涉及的测试用例

| 测试文件 | 测试内容 |
|---------|---------|
| `tests/client/resource/ItemTextureAtlasTest.cpp` | 物品纹理加载、方块物品回退路径、路径映射 |

**测试用例详情**：

1. **LoadItemTextureWithoutPngSuffixInLocation**
   - 测试物品纹理加载（无 .png 后缀）
   - 验证多种路径格式都能找到纹理

2. **BlockItemCanLoadFromItemTexturePath**
   - 测试方块物品从 `textures/item/` 加载
   - 验证方块物品优先使用物品纹理路径

3. **BlockItemFallsBackToBlockTexturePath**
   - 测试方块物品回退到方块纹理路径
   - 验证 `textures/block/` 路径作为回退

---

## 性能考虑

1. **纹理图集**：将数百个小纹理打包到单个大纹理，减少 draw call
2. **模型缓存**：BlockModelCache 按状态ID缓存，O(1) 查找
3. **按需加载**：BlockModelLoader 不预加载所有模型，按需加载和缓存
4. **Skyline算法**：纹理打包时间复杂度 O(n log n)，适合离线处理

---

## 扩展建议

1. **异步加载**：考虑在后台线程加载和烘焙模型
2. **图集数组**：对于大量纹理，使用 texture2DArray 替代单个大图集
3. **热重载**：实现 F3+T 纹理重载功能
4. **资源包下载**：支持从服务器下载资源包
