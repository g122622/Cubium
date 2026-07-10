# 皮肤加载器 (Skin Loader)

皮肤数据加载接口与实现，负责从本地文件系统、资源包或远程 URL 加载皮肤 PNG 数据。

## 目录结构

```
loader/
├── SkinLoader.hpp           # ISkinLoader 接口和 SkinLoadResult 定义
├── SkinLoader.cpp           # ISkinLoader 实现
├── FileSkinLoader.hpp       # 本地文件/资源包加载器（支持异步加载，可注入 ServerWorkerPool）
├── FileSkinLoader.cpp       # 包含 PNG 解码（stb_image）、尺寸验证（64x64/64x32）、旧版转换和 PNG 编码（stb_image_write）
├── HttpSkinLoader.hpp       # HTTP 远程加载器（textures.minecraft.net，支持异步加载）
└── HttpSkinLoader.cpp       # HTTP 加载器框架（下载功能待实现）
```

## 内部模块关系

```
ISkinLoader（接口）
├── FileSkinLoader ── 依赖 stb_image / stb_image_write / Sha1 / ServerWorkerPool（可选注入）
└── HttpSkinLoader ── 依赖 Sha1 / ServerWorkerPool（可选注入，_httpGet 待实现）
```

## 上下游外部依赖关系

### 上游依赖

- `common/skin/core` - SkinLoadResult、SkinTextures
- `common/resource` - IResourcePack、ResourceLocation
- `common/util/crypto` - Sha1（缓存键哈希）
- `common/util/thread` - ServerWorkerPool、ITask、FunctionTask（异步加载）
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

### 异步加载与线程池注入

`loadAsync` 通过注入的 `ServerWorkerPool` 实现真正的异步加载：
- 必须在 `initialize()` 之前调用 `setWorkerPool()` 注入线程池
- 线程池由调用方拥有，生命周期必须长于加载器（或在 `shutdown()` 后释放）
- 未注入线程池时，`loadAsync` 降级为同步执行后立即回调
- `shutdown()` 会取消所有在途任务并等待回调完成（通过 `m_pendingCount` 计数 + 条件变量）
- `cancel(url)` 设置对应任务的取消信号，`cancelAll()` 取消所有在途任务
- 回调在 worker 线程触发，调用方需自行处理线程安全

### 哈希算法

皮肤缓存键使用 SHA-1 哈希（`util::crypto::Sha1`），生成 40 字符十六进制字符串。Mojang 纹理 URL 中的哈希为 SHA-256 格式（64 字符），本地缓存使用 SHA-1 以保持与目录结构的兼容。
