# 皮肤缓存管理器 (Skin Cache Manager)

皮肤文件的磁盘缓存管理，包含缓存索引、元数据持久化和默认皮肤提供。

## 目录结构

```
manager/
├── SkinCache.hpp            # 皮肤缓存类（磁盘文件 + 内存索引 + metadata.json 持久化）
├── SkinCache.cpp            # 缓存读写、过期清理、元数据序列化
├── SkinManager.hpp          # 皮肤管理器（主入口，协调缓存、加载器、默认皮肤）
├── SkinManager.cpp          # 加载策略（缓存→异步下载→默认）
├── DefaultSkinProvider.hpp  # 默认皮肤提供者（18种默认皮肤：9 slim + 9 wide）
└── DefaultSkinProvider.cpp  # 基于 UUID 哈希选择默认皮肤
```

## 内部模块关系

```
SkinManager
├── SkinCache ──── metadata.json 持久化 + 磁盘文件缓存
├── DefaultSkinProvider ──── 18种内置默认皮肤（通过 UUID 哈希选择）
├── FileSkinLoader ──── 本地文件/资源包加载（可注入 UniversalWorkerPool 异步加载）
└── HttpSkinLoader ──── HTTP 远程下载（可注入 UniversalWorkerPool 异步加载）
```

## 默认皮肤系统

MC 1.21.1 有 18 种默认皮肤，通过 UUID 哈希选择：

```
选择算法: index = Math.floorMod(UUID.hashCode(), 18)

索引 0-8:  slim (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
索引 9-17: wide (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
```

- `DefaultSkinProvider::getDefaultSkin(uuid)` — 返回 UUID 对应的默认皮肤 ResourceLocation
- `DefaultSkinProvider::getDefaultSkinType(uuid)` — 返回 UUID 对应的皮肤模型类型（Slim/Wide）
- `DefaultSkinProvider::getCanonicalDefaultSkinLocation()` — 返回规范默认皮肤（slim/steve，索引 6）
- `DefaultSkinProvider::isDefaultSkin(location)` — 检查是否为 18 种默认皮肤之一

纹理路径格式：`minecraft:textures/entity/player/{slim|wide}/{name}.png`

## 上下游外部依赖关系

### 上游依赖

- `common/skin/core` - SkinTextures、GameProfile、SkinTypes（含 DefaultSkinVariant）
- `common/skin/loader` - ISkinLoader、SkinLoadResult、FileSkinLoader、HttpSkinLoader
- `common/skin/parser` - SkinMetadataParser
- `common/resource` - ResourceLocation、IResourcePack（DefaultSkinProvider 加载默认皮肤 PNG 纹理）
- `common/util/thread` - UniversalWorkerPool（异步皮肤加载，可选注入）
- `stb_image` - PNG 解码（STB_IMAGE_IMPLEMENTATION 已在 TextureAtlasBuilder.cpp 中定义）
- `nlohmann-json` - 元数据 JSON 序列化
- `spdlog` - 日志

### 下游依赖

- `client/skin/ClientSkinManager` - 继承 SkinManager，添加 GPU 纹理管理

## 容易踩的坑

### 缓存键哈希

缓存键使用 SHA-1 哈希（通过 FileSkinLoader/HttpSkinLoader 的 `_calculateHash` 生成），而非 Mojang 服务器使用的 SHA-256。目录结构为两级：`skins/<hash前2字符>/<完整hash>`。

### 线程安全

SkinCache 的所有公共方法通过 `m_entriesMutex` 保证线程安全。元数据读写仅在初始化/关闭时进行，不持有条目互斥锁。

### 异步加载与线程池注入

`SkinManager` 持有 `FileSkinLoader` 和 `HttpSkinLoader` 实例，并通过 `setWorkerPool()` 将线程池传递给两个加载器。必须在 `initialize()` 之前调用 `setWorkerPool()`。未注入线程池时，`loadAsync` 降级为同步执行。`shutdown()` 会先关闭两个加载器（等待所有在途异步任务完成），再关闭缓存。

### 皮肤加载流程

`_loadFromTextures` 的加载策略：缓存命中 → 异步下载（HttpSkinLoader）→ 默认皮肤。异步下载完成后，皮肤保存到缓存并更新 `PlayerSkinInfo` 的状态。下载失败时回退到默认皮肤。

### DefaultSkinProvider 加载真实皮肤

`_loadBuiltinSkins()` 通过注入的资源包列表从资源包读取 18 种默认皮肤 PNG 纹理
（路径 `textures/entity/player/{slim|wide}/{name}.png`），使用 `stb_image` 解码为 64×64
RGBA 像素数据存入 `m_skinData`。

**生命周期约束**：必须在 `initialize()` 调用之前通过 `setResourcePacks()` 注入资源包列表，
否则 `_loadBuiltinSkins()` 会因无资源包可用而回退到零像素占位数据。注入链路为：
`ClientApplicationSession` → `ClientSkinManager::setResourcePacks()` →
`SkinManager::setResourcePacks()` → `DefaultSkinProvider::setResourcePacks()` /
`FileSkinLoader::setResourcePacks()`。

**查找策略**：按资源包优先级反向遍历（后添加的优先，`rbegin→rend`），与 `ResourceManager`
纹理加载惯例一致。必须注入完整列表而非仅首个 pack——内置 vanilla `InMemoryResourcePack`
只注册了模型/方块状态 JSON，player 皮肤 PNG 实际位于磁盘资源包，需遍历所有 pack 才能命中。

**容错策略**：单个变体加载失败仅记录警告并跳过，对应 `m_skinData[i]` 保持零像素占位
数据，整体 `initialize()` 不返回错误，保持与原 fallback 语义一致的容错性。
旧版 64×32 皮肤会被自动转换为 64×64（下半部分填零）。
