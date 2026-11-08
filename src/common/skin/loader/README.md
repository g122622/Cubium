# 皮肤加载器 (Skin Loader)

皮肤数据加载接口与实现，负责从本地文件系统、资源包或远程 URL 加载皮肤 PNG 数据。

## 目录结构

```
loader/
├── SkinLoader.hpp           # ISkinLoader 接口和 SkinLoadResult 定义
├── SkinLoader.cpp           # ISkinLoader 实现
├── SkinImageValidator.hpp   # 皮肤 PNG 校验与规范化（两个 loader 共用）
├── SkinImageValidator.cpp   # PNG 解码、尺寸校验、旧版转换与重新编码
├── FileSkinLoader.hpp       # 本地文件/资源包加载器（支持异步加载，可注入 UniversalWorkerPool）
├── FileSkinLoader.cpp       # 本地来源的读取与异步调度
├── HttpSkinLoader.hpp       # HTTP 远程加载器（textures.minecraft.net，支持异步加载）
└── HttpSkinLoader.cpp       # asio + OpenSSL 的真实 HTTPS GET 与 HTTP 响应解析
```

## 内部模块关系

```
ISkinLoader（接口）
├── FileSkinLoader ── 依赖 IResourcePack / UniversalWorkerPool（可选注入）
└── HttpSkinLoader ── 依赖 asio / OpenSSL / UniversalWorkerPool（可选注入）

两者都依赖 SkinImageValidator 完成校验与规范化，依赖 Sha1 计算缓存键。
```

## 上下游外部依赖关系

### 上游依赖

- `common/skin/core` - SkinLoadResult、SkinTextures
- `common/resource` - IResourcePack、ResourceLocation
- `common/util/crypto` - Sha1（缓存键哈希）
- `common/util/thread` - UniversalWorkerPool、ITask、FunctionTask（异步加载）
- `asio` / `OpenSSL` - HTTPS 传输（仅 HttpSkinLoader）
- `stb_image` / `stb_image_write` - PNG 编解码（仅 SkinImageValidator）
- `spdlog` - 日志

### 下游依赖

- `common/skin/manager/SkinManager` - 协调加载器与缓存
- `client/skin/ClientSkinManager` - GPU 纹理上传

## 容易踩的坑

### SkinLoadResult::pngData 语义

`pngData` 字段包含 **PNG 编码的字节数据**（而非原始 RGBA 像素数据）。加载时会先解码 PNG 验证尺寸，必要时把 64x32 旧版皮肤转换为 64x64，然后重新编码为 PNG。下游消费者（如 `ClientSkinManager::_uploadSkinToAtlas`）应使用 stb_image 再次解码。

### 校验逻辑必须单一来源

`SkinImageValidator` 是两个 loader 的唯一校验实现。**不要**在任一 loader 里另写一份尺寸/转换逻辑：两个来源必须产出完全一致的字节，否则同一张皮肤经由不同来源加载会得到不同的缓存哈希，缓存会分裂成两份。

### HTTP 下载

`HttpSkinLoader::_httpGet` 用 asio 同步 connect/read，HTTPS 由 OpenSSL 提供，并设置 SNI 与证书主机名校验（`host_name_verification`）。响应体同时支持 `Content-Length` 与 `chunked` 两种长度表达。每次请求使用独立的 `io_context`，因此并发 `loadAsync` 之间不共享任何状态。

注意：`socket` 在 TLS 分支会被 move 进 `ssl::stream`，非 TLS 分支才直接使用它——修改这段代码时不要引入对已 move 对象的引用。

### 异步加载与线程池注入

`loadAsync` 通过注入的 `UniversalWorkerPool` 实现真正的异步加载：
- 必须在 `initialize()` 之前调用 `setWorkerPool()` 注入线程池
- 线程池由调用方拥有，生命周期必须长于加载器（或在 `shutdown()` 后释放）
- 未注入线程池时，`loadAsync` 降级为同步执行后立即回调
- `shutdown()` 会取消所有在途任务并等待回调完成（通过 `m_pendingCount` 计数 + 条件变量）
- `cancel(url)` 设置对应任务的取消信号，`cancelAll()` 取消所有在途任务
- 回调在 worker 线程触发，调用方需自行处理线程安全

### 哈希算法

皮肤缓存键使用 SHA-1 哈希（`util::crypto::Sha1`），生成 40 字符十六进制字符串。Mojang 纹理 URL 中的哈希是 SHA-256 格式（64 字符），本地缓存使用 SHA-1 以保持与目录结构的兼容。