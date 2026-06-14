# client/resource 模块

客户端资源加载模块，负责从资源包加载和管理方块模型、方块状态、纹理图集等渲染所需资源。

## 目录结构

```
src/client/resource/
├── ResourceManager.hpp/cpp      # 资源管理器核心入口，协调各加载器
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

## BlockModelLoader 共享工具方法

BlockModelLoader 提供了一组 public static 方法，供 ItemModelLoader 等其他加载器复用，
避免模型元素解析和父子模型合并逻辑的重复实现。

| 方法 | 签名 | 用途 | 调用方 |
|------|------|------|--------|
| `parseElement` | `static Result<ModelElement> parseElement(const nlohmann::json&)` | 从 JSON 对象解析单个模型元素（from/to/rotation/shade/faces），并自动计算省略 UV 的面 | BlockModelLoader、ItemModelLoader |
| `parseFace` | `static Result<ModelFace> parseFace(const nlohmann::json&, Direction)` | 从 JSON 对象解析模型面（texture/cullface/tintindex/uv/rotation） | BlockModelLoader（parseElement 内部调用） |
| `parseUV` | `static ModelFaceUV parseUV(const nlohmann::json&)` | 从 JSON 数组解析 UV 坐标 `[u0, v0, u1, v1]` | BlockModelLoader（parseFace 内部调用） |
| `parseRotation` | `static ModelRotation parseRotation(const nlohmann::json&)` | 从 JSON 对象解析旋转信息（origin/axis/angle/rescale） | BlockModelLoader（parseElement 内部调用） |
| `computeDefaultUVs` | `static void computeDefaultUVs(ModelElement&)` | 为省略 UV 的面根据 from/to 坐标自动计算默认 UV，MC JSON 允许省略 UV | BlockModelLoader（parseElement 内部调用） |
| `mergeParent` | `static void mergeParent(UnbakedBlockModel& accumulated, const UnbakedBlockModel& currentLayer)` | 合并父子模型属性（纹理覆盖、元素 first-defined-wins、AO 继承），用于 root-to-leaf 逐层累积 | BlockModelLoader、ItemModelLoader |
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

MC 1.12 使用 `textures/blocks/`，MC 1.13+ 使用 `textures/block/`。加载器会自动尝试两种路径。

### 3. 模型烘焙依赖顺序

`BlockAppearance` 需要 `TextureRegion`，必须先调用 `buildTextureAtlas()` 再调用 `computeBlockAppearances()`。ResourceManager 内部已正确处理此顺序。

### 4. 方块状态属性字符串格式

属性字符串必须按字母顺序排列才能匹配。BlockStateLoader 使用 `_propertiesToStateStr()` 自动排序。

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
