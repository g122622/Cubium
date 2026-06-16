# 资源模块

资源模块负责资源包发现、优先级管理、资源读取和多语言翻译。所有资源模块类统一使用 `mc::resource` 命名空间，并在 `mc` 命名空间中提供 `using` 别名以保持向后兼容。`PackRepository`（原 `ResourcePackList`）已支持并发读写，并被客户端主线程与音频线程共享使用。

## 目录结构

```text
src/common/resource/
├── ResourceLocation.hpp/cpp        # 资源定位符（namespace:path）解析与路径转换
├── PackType.hpp                    # 资源包类型枚举（ClientResources→assets/，ServerData→data/）
├── pack/                           # 资源包接口与实现
│   ├── IResourcePack.hpp/cpp       # 资源包抽象接口，支持 PackType 参数
│   ├── FolderResourcePack.hpp/cpp  # 文件夹资源包，从目录读取资源
│   ├── ZipResourcePack.hpp/cpp     # ZIP 资源包，内部缓存已加锁
│   ├── InMemoryResourcePack.hpp/cpp # 内存资源包，适合原版默认资源
│   └── PackMetadata.hpp/cpp        # pack.mcmeta 解析
├── repository/                     # 资源包管理
│   ├── PackListBase.hpp/cpp        # 资源包列表基类，提取公共逻辑
│   ├── PackRepository.hpp/cpp      # 客户端资源包仓库（原 ResourcePackList），默认 PackType::ClientResources
│   └── DataPackRepository.hpp      # 服务端数据包仓库（原 DataPackList），默认 PackType::ServerData
├── VanillaResources.hpp/cpp        # 原版模型/方块状态等基础资源
├── LanguageManager.hpp/cpp         # 多语言翻译管理器，从资源包加载语言文件，支持占位符替换
└── metadata/                       # 资源元数据
    └── AnimationMetadata.hpp/cpp   # 动画纹理元数据（.mcmeta）
```

## 命名空间

所有资源模块类位于 `mc::resource` 命名空间。在 `mc` 命名空间中提供了 `using` 别名，因此外部代码可以使用 `mc::PackRepository` 或 `mc::resource::PackRepository` 两种写法。

主要别名：
- `mc::IResourcePack` → `mc::resource::IResourcePack`
- `mc::ResourcePackPtr` → `mc::resource::ResourcePackPtr`
- `mc::PackRepository` → `mc::resource::PackRepository`
- `mc::DataPackRepository` → `mc::resource::DataPackRepository`
- `mc::PackListBase` → `mc::resource::PackListBase`
- `mc::ResourceLocation` → `mc::resource::ResourceLocation`
- `mc::LanguageManager` → `mc::resource::LanguageManager`
- `mc::PackType` → `mc::resource::PackType`

## 内部模块关系

- `IResourcePack` 是抽象接口，`FolderResourcePack`、`ZipResourcePack`、`InMemoryResourcePack` 为其具体实现。
- `PackListBase` 是资源包列表基类，提取了 `PackRepository` 和 `DataPackRepository` 的公共逻辑（优先级管理、并发查询、变更通知等）。
- `PackRepository` 继承 `PackListBase`，管理多个客户端资源包实例，默认使用 `PackType::ClientResources`，支持 `loadFromSettings`/`saveToSettings`。
- `DataPackRepository` 继承 `PackListBase`，限定 `PackType::ServerData`，服务战利品表、配方等数据加载。
- `LanguageManager` 从 `PackRepository` 加载语言文件，为 `TranslationTextComponent` 提供翻译服务。

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `common/core/Result.hpp` - 结果类型
- `common/core/settings/ResourcePackListOption.hpp` - 资源包设置
- `common/util/assert/AssertAll.hpp` - 断言
- `nlohmann-json` - JSON 解析
- `libarchive` - ZIP 解压
- `stb_image` - 图像加载
- `spdlog` - 日志

**下游依赖（依赖本模块）：**
- `ClientApplication` - 启动期收集、加载并监听资源包变化
- `AudioService` / `SoundHandler` - 读取 `sounds.json`
- `ResourceManager` - 构建纹理图集与模型缓存
- `TranslationTextComponent` - 翻译文本组件
- 服务端数据加载器（`LootTableLoader`、`RecipeLoader`、`BiomeTagLoader` 等）

## 容易踩的坑

- **不能长期保存 `PackRepository` 内部元素地址**：查询接口返回的是拷贝（`PackInfo` 按值返回）。
- **`containsPack()` 只做存在性判断**：不要拿它代替实际加载。
- **`addPack()` 不是纯内存操作**：会在锁外创建和初始化资源包，再做二次插入校验。
- **`ZipResourcePack` 缓存已加锁**：但资源包本身仍应通过 `PackRepository` 统一访问。
- **资源路径统一使用 `/`**：Windows 路径分隔符会在内部规范化。
- **`LanguageManager::instance()` 是全局单例**：需要手动调用 `loadLanguage()` 加载语言文件。
- **`TranslationTextComponent` 默认使用 `LanguageManager::instance()`**：可通过 `setLanguageManager()` 覆盖。
- **语言文件占位符**：支持 `%s`（顺序参数）、`%1$s`/`%2$s`（位置参数）、`%%`（转义百分号）。
- **`IResourcePack` 的路径工具方法**：`normalizePath`、`makeTypedPath`、`matchesExtension` 是静态保护方法，供子类复用。
- **`InMemoryResourcePack` 统一接口**：使用 `addResource(PackType, path, content)` 方法，`addClientResource`/`addServerDataResource` 为便捷方法。
