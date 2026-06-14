# 皮肤加载器 (Skin Loader)

皮肤数据加载接口与实现，负责从本地文件系统、资源包或远程 URL 加载皮肤 PNG 数据。

## 目录结构

```
loader/
├── SkinLoader.hpp           # ISkinLoader 接口和 SkinLoadResult 定义
├── SkinLoader.cpp           # ISkinLoader 实现
├── FileSkinLoader.hpp       # 本地文件/资源包加载器
├── FileSkinLoader.cpp       # 包含 PNG 解码（stb_image）、尺寸验证（64x64/64x32）、旧版转换和 PNG 编码（stb_image_write）
├── HttpSkinLoader.hpp       # HTTP 远程加载器（textures.minecraft.net）
└── HttpSkinLoader.cpp       # HTTP 加载器框架（下载功能待实现）
```

## 内部模块关系

```
ISkinLoader（接口）
├── FileSkinLoader ── 依赖 stb_image / stb_image_write / Sha1
└── HttpSkinLoader ── 依赖 Sha1（_httpGet 待实现）
```

## 上下游外部依赖关系

### 上游依赖

- `common/skin/core` - SkinLoadResult、SkinTextures
- `common/resource` - IResourcePack、ResourceLocation
- `common/util/crypto` - Sha1（缓存键哈希）
- `stb_image` - PNG 解码
- `stb_image_write` - PNG 编码
- `spdlog` - 日志

### 下游依赖

- `common/skin/manager/SkinManager` - 协调加载器与缓存
- `client/skin/ClientSkinManager` - GPU 纹理上传

## 容易踩的坑

### SkinLoadResult::pngData 语义

`pngData` 字段包含 **PNG 编码的字节数据**（而非原始 RGBA 像素数据）。FileSkinLoader 在加载时会先解码 PNG 验证尺寸，必要时将 64x32 旧版皮肤转换为 64x64，然后重新编码为 PNG。下游消费者（如 ClientSkinManager::_uploadSkinToAtlas）应使用 stb_image 再次解码。

### HTTP 加载器未实现

`HttpSkinLoader::_httpGet` 当前返回 `ErrorCode::Unsupported`，完整的 HTTP 下载功能（使用 curl 或 asio）待实现。`_validateAndConvertSkin` 也需要在 HTTP 下载实现后补充与 FileSkinLoader 一致的验证逻辑。

### 哈希算法

皮肤缓存键使用 SHA-1 哈希（`util::crypto::Sha1`），生成 40 字符十六进制字符串。Mojang 纹理 URL 中的哈希为 SHA-256 格式（64 字符），本地缓存使用 SHA-1 以保持与目录结构的兼容。
