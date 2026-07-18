# client/resource 模块

客户端资源加载模块，负责从资源包加载和管理方块模型、方块状态、纹理图集等渲染所需资源。

## 目录结构

```
src/client/resource/
├── ResourceManager.hpp/cpp      # 资源管理器核心入口，协调各加载器；含 getAltTexturePath() 路径变体兼容方法
├── BlockModelLoader.hpp/cpp     # 方块模型 JSON 解析，支持继承和纹理变量
├── BlockStateLoader.hpp/cpp     # 方块状态 JSON 解析，管理状态到模型映射
├── BlockModelCache.hpp/cpp      # 连接 BlockRegistry 和 ResourceManager 的缓存层
├── ItemModelLoader.hpp/cpp      # 物品模型加载器
├── ItemModelCache.hpp/cpp       # 物品模型缓存
├── TextureAtlasBuilder.hpp/cpp  # 纹理图集构建器，Skyline 算法打包
├── ItemTextureAtlas.hpp/cpp     # 物品纹理图集管理
├── DestroyStageTextures.hpp/cpp # 破坏阶段纹理（10个阶段）
└── EntityTextureLoader.hpp/cpp  # 实体纹理加载器（含 Misc 类别实体如经验球的特殊加载路径）
```

## 内部模块关系

```
ResourceManager（核心入口，协调所有加载器）
    ├── BlockStateLoader（blockstates/*.json 解析）
    ├── BlockModelLoader（models/block/*.json 解析）
    ├── TextureAtlasBuilder（纹理打包）
    └── BlockModelCache（状态→外观缓存）
            ├── ItemTextureAtlas（物品纹理）
            ├── DestroyStageTextures（破坏纹理）
            └── EntityTextureLoader（实体纹理）
```

**数据流**：
1. `ResourceManager` 添加资源包 → 调用 `loadAllResources()` 加载方块状态和模型
2. `buildTextureAtlas()` 将分散纹理打包成大图集，生成 UV 映射
3. `BlockModelCache.initialize()` 遍历 BlockRegistry 构建状态ID→外观映射
4. 区块渲染时通过 `BlockModelCache.getBlockAppearance(state)` O(1) 获取外观

## 物品模型预加载

`ItemModelCache::initialize()` 会在构造 `ItemModelLoader` 后立即调用
`ItemModelLoader::loadAllModels()` 全量预加载所有资源包中
`assets/<namespace>/models/item/` 目录下的 `.json` 文件并烘焙，将结果填充到
`m_unbakedModels` / `m_bakedModels` 缓存，供后续 `getModel` / `getItemModel`
直接命中。

**设计意图**：将物品模型加载从运行时延迟加载升级为预加载 + 延迟加载兜底，
消除玩家进入游戏后的运行时卡顿。

**容错行为**：
- 单个模型烘焙失败不中断整体流程，仅记录 `spdlog::warn` 警告。
- `loadAllModels()` 整体不返回错误（部分文件缺失属正常情况）。
- 即使 `loadAllModels()` 整体失败（理论上不会发生，因其总是返回 `ok()`），
  `ItemModelCache::initialize()` 也仅记录 `spdlog::warn` 后继续，
  后续 `getItemModel()` 仍可走 `bakeModel()` 延迟加载兜底路径。
- 跨包去重：同一 `ResourceLocation` 只烘焙一次，高优先级包（`m_resourcePacks[0]`）
  的内容通过 `_readModelFromResourcePacks` 的顺序读取自然生效。

## 上下游依赖关系

### 上游依赖（本模块依赖的）

**内部依赖**：
- `common/core/Types.hpp` - 基础类型
- `common/core/Result.hpp` - 错误处理
- `common/resource/` - 资源包接口、ResourceLocation
- `common/world/block/Block.hpp` - 方块注册表
- `common/item/` - 物品注册表
- `client/renderer/MeshTypes.hpp` - TextureRegion 定义

**外部依赖**：
- `nlohmann/json` - JSON 解析
- `stb_image.h` - PNG 解码
- `spdlog` - 日志
- `glm` - 数学库
- `Vulkan` - GPU 资源（ItemTextureAtlas）

### 下游依赖（依赖本模块的）

- `client/renderer/world/ChunkMeshBuilder.cpp` - 区块网格生成时获取 BlockAppearance
- `client/renderer/item/ItemRenderer.cpp` - 物品渲染时获取物品纹理
- `client/renderer/entity/EntityRenderer.cpp` - 实体渲染时获取实体纹理
- `client/game/ClientWorld.cpp` - 世界加载时初始化资源管理器

## 模型合并语义（leaf-wins）

MC Java 版使用 `findTop*` 模式：沿叶子到根方向查找第一个显式定义了该属性的模型。
`bakeModel` 采用内联的 leaf-to-root 查找策略实现此语义。

### 显式设置标记（has* 标志）

`UnbakedBlockModel` 和 `UnbakedItemModel` 中的 `hasElements`、`hasAmbientOcclusion`、
（仅 Item）`hasOverrides` 布尔字段用于区分"JSON 显式定义了该字段"与"JSON 中未出现该字段（使用默认值）"。
只有 `has* == true` 的模型层才会参与 leaf-wins 查找，确保正确实现 MC Java 的合并语义。

### 各属性合并规则

| 属性 | 合并策略 | 说明 |
|------|----------|------|
| 纹理（textures） | merge（子覆盖父同名键） | 从根到叶逐层合并，后处理的层覆盖先处理层的同名键 |
| 元素（elements） | leaf-wins | 从叶子到根查找第一个 `hasElements == true` 的模型 |
| 环境光遮蔽（ambientOcclusion） | leaf-wins | 从叶子到根查找第一个 `hasAmbientOcclusion == true` 的模型，默认 true |
| 显示变换（display） | 按上下文独立 leaf-wins | 每个 ItemDisplayContext 独立从根到叶覆盖 |
| 覆盖条件（overrides） | leaf-wins | 从叶子到根查找第一个 `hasOverrides == true` 的模型 |

### BakedItemModel 新增字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `ambientOcclusion` | `bool` | 从继承链合并后的环境光遮蔽设置，默认 true |

## BlockModelLoader 共享工具方法

BlockModelLoader 提供了一组 public static 方法，供 ItemModelLoader 等其他加载器复用，
避免模型元素解析和父子模型合并逻辑的重复实现。

| 方法 | 签名 | 用途 | 调用方 |
|------|------|------|--------|
| `parseElement` | `static Result<ModelElement> parseElement(const nlohmann::json&)` | 从 JSON 对象解析单个模型元素（from/to/rotation/shade/faces），并自动计算省略 UV 的面 | BlockModelLoader、ItemModelLoader |
| `parseFace` | `static Result<ModelFace> parseFace(const nlohmann::json&, Direction)` | 从 JSON 对象解析模型面（texture/cullface/tintindex/uv/rotation） | BlockModelLoader（parseElement 内部调用） |
| `parseUV` | `static ModelFaceUV parseUV(const nlohmann::json&)` | 从 JSON 数组解析 UV 坐标 `[u0, v0, u1, v1]` | BlockModelLoader（parseFace 内部调用） |
| `parseRotation` | `static ModelRotation parseRotation(const nlohmann::json&)` | 从 JSON 对象解析旋转信息，支持两种格式：传统 axis/angle/rescale 和 MC 1.21.11 新增的 EulerXYZ (x/y/z/rescale) | BlockModelLoader（parseElement 内部调用） |
| `computeDefaultUVs` | `static void computeDefaultUVs(ModelElement&)` | 为省略 UV 的面根据 from/to 坐标自动计算默认 UV，MC JSON 允许省略 UV | BlockModelLoader（parseElement 内部调用） |
| `mergeParent` | `static void mergeParent(UnbakedBlockModel& accumulated, const UnbakedBlockModel& currentLayer)` | 合并父子模型属性（纹理 merge、元素 leaf-wins、AO leaf-wins），用于 root-to-leaf 逐层累积 | BlockModelLoader（公共API）、测试代码 |
| `resolveTextureReferences` | `static void resolveTextureReferences(std::map<std::string, ResourceLocation>&, i32 maxIterations=10)` | 递归解析 `#variable` 形式的纹理引用链（如 `down=#all, all=block/stone`） | BlockModelLoader、ItemModelLoader |

**使用示例**：

```cpp
// 在 ItemModelLoader 中复用元素解析（替代原先的内联实现）
auto result = BlockModelLoader::parseElement(elemJson);
if (result.success()) {
    model.elements.push_back(result.value());
}

// 在 bakeModel 中复用合并和纹理解析逻辑
BlockModelLoader::mergeParent(merged, *modelLayer);
BlockModelLoader::resolveTextureReferences(baked.textures);
```

## 容易踩的坑

### 1. 资源包加载顺序

后添加的资源包优先级更高，可能导致纹理被意外覆盖。按优先级从低到高添加：vanilla → mod → user。

### 2. 纹理路径兼容性

MC 1.12 使用 `textures/blocks/`，MC 1.13+ 使用 `textures/block/`。ResourceManager 通过 `getAltTexturePath()` 集中管理路径变体转换，在以下三个层面自动处理兼容性：

1. **图集构建阶段**（`buildTextureAtlas()`）：当原始路径在资源包中找不到纹理时，自动尝试变体路径加载；构建完成后，将路径变体别名注册到 `m_textureRegions`，确保后续直接查找即可命中。
2. **纹理查找阶段**（`_findTextureRegion()` / `getTextureRegion()`）：查找纹理区域时，原始路径未命中则自动尝试对应的变体路径。
3. **集中化工具方法**（`getAltTexturePath()`）：公共静态方法，支持以下路径变体互转：
   - `textures/block/` ↔ `textures/blocks/`（MC 1.13+ 单数 ↔ MC 1.12 复数）
   - `textures/item/` ↔ `textures/items/`（MC 1.13+ 单数 ↔ MC 1.12 复数）
   - `textures/entity/<name>/<name>` ↔ `textures/entity/<name>`（MC 1.13+ 子目录 ↔ MC 1.12 扁平格式）

   所有模块（`DestroyStageTextures`、`ItemTextureAtlas`、`EntityTextureLoader`、`EntityTextureAtlas`）均复用此方法，消除了独立的路径变体回退逻辑。

### 3. 模型烘焙依赖顺序

`BlockAppearance` 需要 `TextureRegion`，必须先调用 `buildTextureAtlas()` 再调用 `computeBlockAppearances()`。ResourceManager 内部已正确处理此顺序。

### 4. 方块状态属性字符串格式

属性字符串必须按字母顺序排列才能匹配。`BlockStateDefinition::_normalizeStateKey()` 在解析时自动排序，`ResourceManager::getBlockAppearance` 的回退匹配也按排序后的属性键比较。

### 5. 纹理图集尺寸限制

默认最大 4096×4096，大量纹理可能无法放入。使用 `TextureAtlasBuilder::setMaxSize()` 调整。

### 6. ItemTextureAtlas GPU 上传时机

`upload()` 必须在 Vulkan 设备就绪后调用，且需要有效的命令池和图形队列。确保在 `create()` 后调用。

### 7. 破坏纹理格式转换

原版破坏纹理是灰度图，需要转换为着色器期望格式。DestroyStageTextures 自动处理：`crackIntensity = 255 - luminance`。

### 8. 模型继承循环

模型父子关系可能存在循环引用。BlockModelLoader::bakeModel() 限制最大迭代次数为10次防止无限递归。

### 9. 方块物品纹理回退

方块物品优先从 `textures/item/` 加载，找不到时回退到 `textures/block/`。ItemTextureLoader 自动处理此逻辑。

### 10. 动画纹理

动画纹理（如水、岩浆）需要 `.mcmeta` 元数据文件。TextureAtlasBuilder 会自动检测并解析动画信息。

### 11. 缓存失效

资源包变更后需调用 `BlockModelCache::rebuild()` 重建缓存，否则外观数据会过时。

### 12. Misc 类别实体纹理加载

`EntityClassification::Misc` 的实体（如经验球、投掷物）默认不加载纹理（`needsTexture(Misc)` 返回 `false`）。需要纹理的 Misc 实体必须加入 `SPECIAL_TEXTURE_PATHS` 映射，并在 `_loadMiscEntityTextures()` 中额外加载。经验球纹理 `experience_orb` 是 64×64 精灵图集，包含 4列×3行共 11 个 16×16 图标。
