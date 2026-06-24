# 皮肤缓存管理器 (Skin Cache Manager)

皮肤文件的磁盘缓存管理，包含缓存索引、元数据持久化和默认皮肤提供。

## 目录结构

```
manager/
├── SkinCache.hpp            # 皮肤缓存类（磁盘文件 + 内存索引 + metadata.json 持久化）
├── SkinCache.cpp            # 缓存读写、过期清理、元数据序列化
├── SkinManager.hpp          # 皮肤管理器（主入口，协调缓存、加载、默认皮肤）
├── SkinManager.cpp          # 加载策略（缓存→下载→默认）
├── DefaultSkinProvider.hpp  # 默认皮肤提供者（18种默认皮肤：9 slim + 9 wide）
└── DefaultSkinProvider.cpp  # 基于 UUID 哈希选择默认皮肤
```

## 内部模块关系

```
SkinManager
├── SkinCache ──── metadata.json 持久化 + 磁盘文件缓存
├── DefaultSkinProvider ──── 18种内置默认皮肤（通过 UUID 哈希选择）
└── ISkinLoader（通过 loadSkin 间接使用）
    ├── FileSkinLoader
    └── HttpSkinLoader
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
- `common/skin/loader` - ISkinLoader、SkinLoadResult
- `common/skin/parser` - SkinMetadataParser
- `common/resource` - ResourceLocation
- `nlohmann-json` - 元数据 JSON 序列化
- `spdlog` - 日志

### 下游依赖

- `client/skin/ClientSkinManager` - 继承 SkinManager，添加 GPU 纹理管理

## 容易踩的坑

### 缓存键哈希

缓存键使用 SHA-1 哈希（通过 FileSkinLoader/HttpSkinLoader 的 `_calculateHash` 生成），而非 Mojang 服务器使用的 SHA-256。目录结构为两级：`skins/<hash前2字符>/<完整hash>`。

### 线程安全

SkinCache 的所有公共方法通过 `m_entriesMutex` 保证线程安全。元数据读写仅在初始化/关闭时进行，不持有条目互斥锁。

### DefaultSkinProvider 尚未加载真实皮肤

`_loadBuiltinSkins()` 当前填充零像素数据，需要后续从资源包加载 18 种默认皮肤纹理。纹理文件已存在于资源包路径 `textures/entity/player/{slim|wide}/{name}.png`。
