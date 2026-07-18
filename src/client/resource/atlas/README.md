# client/resource/atlas 模块

原版 `atlases` 资产的数据驱动加载层，对齐 MC 1.21.11 `SpriteSourceList` / `SpriteSources` / `SpriteLoader`。从资源包 `assets/<ns>/atlases/<id>.json` 加载图集定义，驱动 `TextureAtlasBuilder` 完成纹理收集与打包，替代历史散落在各图集类中的硬编码收集逻辑。

## 目录结构

```
src/client/resource/atlas/
├── AtlasSource.hpp/cpp          # 抽象基类 + SpriteSourceOutput(sprite 级后覆盖先/removeAll) + SpriteSourceOutput 实现
├── SpriteContents.hpp            # 解码后精灵内容 {pixels, w, h, optional<AnimationMetadata>}
├── SpriteLoader.hpp/cpp         # fromTextureResource/fromPredecoded + resolve(packs)→SpriteContents(读PNG+mcmeta)
├── IdentifierPattern.hpp/cpp    # filter source 的 namespace/path 可选正则，matches(ResourceLocation)
├── Sources.hpp/cpp              # 5 个具体 source: Single/Directory/Filter/Unstitch/PalettedPermutations
├── AtlasSourceParser.hpp/cpp    # parseSource(json) type dispatch + parseAtlasJson(json)→vector<source>
├── AtlasConfigLoader.hpp/cpp    # load(packs, atlasId)：多包 sources 拼接(低→高优先级 addAll)
├── MissingNo.hpp/cpp            # 紫黑棋盘格生成(每个图集末尾兜底 sprite)
├── TexturePathVariant.hpp/cpp   # getAltTexturePath 迁移(textures/block↔blocks 等兼容，仅查询层回退)
├── AtlasHandle.hpp/cpp          # 【阶段2】单图集 Vulkan 资源(VkImage/View/Sampler)+AtlasBuildResult+uploadRegion
└── AtlasManager.hpp/cpp         # 【阶段2】核心：管 N 个具名图集生命周期/加载/GPU上传/查询/动画tick
```

> 阶段1（已完成）：AtlasSource/SpriteContents/SpriteLoader/IdentifierPattern/Sources/AtlasSourceParser/AtlasConfigLoader/MissingNo/TexturePathVariant。
> 阶段2 待建：AtlasHandle、AtlasManager。

## 内部模块关系

```
AtlasManager（核心，编排每个具名图集的完整加载链）
    ├── AtlasConfigLoader（读 atlas JSON，多包 sources 拼接）
    │       └── AtlasSourceParser（type dispatch 到 5 种 source）
    │               └── Sources（Single/Directory/Filter/Unstitch/Paletted，各自 run 一个 SpriteSourceOutput）
    │                       └── SpriteLoader（懒解码：single/directory 读 PNG+mcmeta；unstitch 切片；paletted 调色板映射）
    ├── TextureAtlasBuilder（复用，Skyline 打包，每个图集一个实例）
    ├── AtlasHandle（单图集 Vulkan 资源 + GPU 上传 + 动画帧更新）
    ├── TextureAtlasTicker（复用，每个图集一个，动画 tick）
    ├── MissingNo（每个图集末尾兜底 sprite）
    └── TexturePathVariant（查询层路径变体回退）
```

**数据流**（对齐原版 SpriteLoader.loadAndStitch）：
1. `AtlasConfigLoader::load(packs, atlasId)` 遍历包（低→高优先级），把每个包的 `atlases/<id>.json` 的 `sources` 数组 addAll 拼接
2. 依次 `source->run(pack, output)` 执行语义，`SpriteSourceOutput` 处理 sprite 级后覆盖先 + removeAll
3. `output.build()` 产出唯一 sprite→SpriteLoader 列表，末尾追加 missingno 兜底
4. 每个 SpriteLoader `resolve(pack)` 解码像素（含 mcmeta 动画）
5. 喂给 `TextureAtlasBuilder::addTextureFrame` 打包，`build()` 得 AtlasBuildResult
6. `AtlasHandle` 上传 GPU，`TextureAtlasTicker` 注册动画

## 原版 5 种 source type 语义

| type | 类 | 字段 | 语义 |
|------|----|------|------|
| `minecraft:single` | SingleFileSource | `resource`(必填)、`sprite`(可选=resource) | 加载单个纹理，sprite 名可独立于资源名 |
| `minecraft:directory` | DirectoryListerSource | `source`(必填,如"block")、`prefix`(必填,如"block/") | 枚举 `textures/<source>/**/*.png`，sprite 名=prefix+相对路径 |
| `minecraft:filter` | FilterSource | `pattern`(含可选 namespace/path 正则) | 从已累积 sprite 集合 removeAll 匹配项，只影响之前的 source |
| `minecraft:unstitch` | UnstitcherSource | `resource`、`regions`[{sprite,x,y,width,height}]、`divisor_x/y`(默认1.0) | 大图切片，像素=floor(region.x*imgW/divisorX) |
| `minecraft:paletted_permutations` | PalettedPermutationsSource | `textures`、`palette_key`、`permutations`、`separator`(默认"_") | textures[i]×permutations[name] 生成衍生 sprite，调色板映射上色 |

## 上下游依赖关系

### 上游依赖

- `common/core/Result.hpp`、`common/resource/ResourceLocation.hpp`、`common/resource/pack/IResourcePack.hpp`
- `common/resource/metadata/AnimationMetadata.hpp`（mcmeta 解析）
- `client/resource/TextureAtlasBuilder.hpp`、`client/renderer/MeshTypes.hpp`（TextureRegion/AtlasBuildResult）
- `client/renderer/trident/core/texture/TextureAtlasTicker.hpp`
- 外部：nlohmann/json、stb_image、spdlog、Vulkan（仅 AtlasHandle/AtlasManager）

### 下游依赖

- `AtlasManager` 被 `TridentEngine`、`ClientApplication`、各 Renderer（ChunkMesher/ItemRenderer/EntityRenderer/ParticleManager/GuiRenderer/BreakProgressRenderer）消费

## 容易踩的坑

### 1. sprite 名约定与文件路径映射（不含 textures/ 前缀）

原版 sprite 名是 `block/stone` 而非 `textures/block/stone`。图集内部 sprite 名严格用原版约定（directory source 产出 `block/stone`，single source 的 `resource`/`sprite` 也是 `block/stone`）。方块模型 textures 字段值 `minecraft:block/stone` 在原版约定下即 sprite 名，无需转换。

**sprite 名 → 物理文件路径**（对齐原版 `FileToIdConverter("textures",".png")`）：
sprite `minecraft:block/stone` → 文件 `assets/minecraft/textures/block/stone.png`。
在 Cubium 的 `IResourcePack` API 里，`readResource`/`hasResource` 的路径参数需带 namespace：
`"<ns>/textures/<path>.png"`（如 `minecraft/textures/block/stone.png`），`makeTypedPath`/`FolderResourcePack` 再补 `assets/` 前缀。

**`listResources` 目录参数需带 namespace**（对齐 `BlockStateLoader` 范式）：
`listResources(ClientResources, "<ns>/textures/<source>", "png")`，返回路径形如 `<ns>/textures/<source>/<sub>.png`。
故 `directory` source 必须 `getResourceNamespaces` 逐命名空间枚举，不能传裸 `textures/<source>`。

`textures/` 前缀仅在 `TexturePathVariant` 查询层回退时使用，不污染图集键。

### 2. 多包覆盖必须在 SpriteSourceOutput.build() 阶段处理

原版多包 sources 是 addAll 拼接（不覆盖），但同一 sprite 名最终像素由"后执行覆盖先执行"决定。若直接把所有 source 产出的 sprite 喂 TextureAtlasBuilder，其 `m_addedLocations` 去重会丢弃高优先级包的同名 sprite。**必须**在 `SpriteSourceOutput::build()` 阶段先处理覆盖（后 add 替换 loader），只喂最终胜出 loader 给 builder。

### 3. filter 只移除已累积的

`minecraft:filter` 的 removeAll 只影响它之前 source 已 add 的 sprite，不阻止后续 source 重新 add 同名 sprite。

### 4. unstitch/paletted 产出的 sprite 无 mcmeta

unstitch（切片）和 paletted_permutations（调色板映射）产出的 sprite 像素是直接生成的，无对应 `.mcmeta` 动画元数据，`SpriteContents.animation` 为空。

### 5. paletted_permutations 调色板等长校验

`palette_key` 与每个 permutation 调色板像素数必须相等，否则报错跳过该 permutation。映射按 RGB 查表替换，保留输入 alpha；palette_key 中找不到输入像素 RGB 时保留原样。

### 6. missingno 兜底

每个图集末尾追加 `minecraft:missingno` sprite（紫黑棋盘格）。查询 miss 时返回 missingno 的 region 而非 nullptr（对齐原版），消费方需适配。

### 7. per-atlas 采样模式

blocks/items 用 NEAREST，particles/gui 用 LINEAR。`AtlasHandle` 创建 sampler 时按 atlasId 查配置表。

### 8. AtlasHandle GPU 生命周期

reload 时先等 GPU 完成旧帧再 destroy 旧 Handle，否则可能 in-flight 访问已释放资源。

### 9. `resource::` 命名空间歧义（本模块专用坑）

本模块在 `namespace mc::client::resource::atlas` 内。裸写 `resource::PackType` / `resource::metadata::AnimationMetadata` 时，名字查找先命中 `mc::client::resource`（当前命名空间的父），而非 `mc::resource`，编译失败。**必须**写全 `mc::resource::PackType`、`mc::resource::metadata::AnimationMetadata`。同理 `mc::resource::packTypeDirectoryName`。
