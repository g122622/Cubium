# 资源模块

资源模块负责资源包发现、优先级管理、资源读取、版本兼容、统一资源加载和多语言翻译。当前 `ResourcePackList` 已支持并发读写，并被客户端主线程与音频线程共享使用。

## 目录结构

```text
src/common/resource/
├── ResourceLocation.hpp/cpp        # 资源定位符（namespace:path）
├── PackType.hpp                    # 资源包类型枚举（ClientResources/ServerData）
├── IResourcePack.hpp/cpp           # 资源包接口
├── FolderResourcePack.hpp/cpp      # 文件夹资源包
├── ZipResourcePack.hpp/cpp         # ZIP 资源包
├── InMemoryResourcePack.hpp/cpp    # 内存资源包
├── PackMetadata.hpp/cpp            # pack.mcmeta 解析
├── ResourcePackList.hpp/cpp        # 客户端资源包列表与优先级管理
├── DataPackList.hpp/cpp            # 服务端数据包列表管理
├── VanillaResources.hpp/cpp        # 原版基础资源
├── LanguageManager.hpp/cpp         # 多语言翻译管理器
├── metadata/                       # 资源元数据
│   └── AnimationMetadata.hpp/cpp   # 动画纹理元数据（.mcmeta）
```

## 文件介绍

- `ResourceLocation`：统一的资源定位符，负责 `namespace:path` 解析与路径转换。
- `PackType`：资源包类型枚举，区分 `ClientResources`（映射到 `assets/` 目录）和 `ServerData`（映射到 `data/` 目录）。
- `IResourcePack`：资源包抽象接口，文件夹包、ZIP 包和内存包都实现它。支持 `PackType` 参数的方法。
- `FolderResourcePack`：从目录读取资源。
- `ZipResourcePack`：从 ZIP 读取资源，当前内部缓存已加锁。
- `InMemoryResourcePack`：内置资源包，适合原版默认资源。
- `PackMetadata`：读取 `pack.mcmeta`。
- `ResourcePackList`：客户端资源包列表、启用状态、优先级、并发查询、`containsPack()`、`getPackInfo()` 和变更通知。
- `DataPackList`：服务端数据包列表管理，限定 `PackType::ServerData`，提供战利品表、配方等数据的加载接口。
- `VanillaResources`：原版模型/方块状态等基础资源。
- `LanguageManager`：多语言翻译管理器，从资源包加载语言文件，支持占位符替换。
- `loader/`：统一资源加载流程，面向纹理/模型/方块状态等上层消费。

## 模块关系

- `ClientApplication` 在启动期收集、加载并监听资源包变化。
- `AudioService` / `SoundHandler` 共享同一个 `ResourcePackList` 读取 `sounds.json`。
- `ResourceManager` 消费 `ResourcePackList` 和 `VanillaResources` 构建纹理图集与模型缓存。
- `LanguageManager` 从 `ResourcePackList` 加载语言文件，为 `TranslationTextComponent` 提供翻译服务。
- `ZipResourcePack`、`FolderResourcePack` 是 `ResourcePackList` 的具体数据源。

## 整体职责

1. 发现并管理资源包。
2. 维护资源包优先级和启用状态。
3. 为客户端渲染、音频、模型和 UI 提供统一资源访问。
4. 处理 MC 不同版本资源路径/格式兼容。
5. 保障主线程和音频线程同时读取资源时的安全性。
6. 加载和管理多语言翻译文件。

## 输入 / 输出

- 输入：
  - 资源包目录中的文件夹或 ZIP
  - `pack.mcmeta`
  - 资源路径，例如 `minecraft:textures/block/stone.png`
  - 启动设置中的资源包配置
  - 语言文件，例如 `assets/minecraft/lang/zh_cn.json`
- 输出：
  - 二进制资源数据
  - 文本资源
  - 纹理像素数据
  - 统一模型/方块状态表示
  - 翻译文本

## 依赖项

- 内部依赖：
  - `common/core/Result.hpp`
  - `common/core/settings/ResourcePackListOption.hpp`
  - `common/util/assert/AssertAll.hpp`
  - `common/resource/FolderResourcePack.hpp`
  - `common/resource/ZipResourcePack.hpp`
  - `common/util/text/TranslationTextComponent.hpp`
- 外部依赖：
  - `nlohmann-json`
  - `libarchive`
  - `stb_image`
  - `spdlog`
  - `shared_mutex`（标准库）

## 使用方法

### 资源包管理

```cpp
using namespace mc;

ResourcePackList packList;
packList.scanDirectory(“resourcepacks”);
packList.loadFromSettings(settings.resourcePacks);

auto packs = packList.getEnabledPacks();
auto resource = packList.readResource(“assets/minecraft/textures/block/stone.png”);
```

### 多语言翻译

```cpp
using namespace mc;

// 获取全局语言管理器
LanguageManager& langManager = LanguageManager::instance();

// 从资源包加载语言文件
auto result = langManager.loadLanguage(packList, “zh_cn”);
if (result.success()) {
    // 获取翻译
    std::string text = langManager.get(“item.minecraft.diamond”);
    // 输出: “钻石”

    // 带参数翻译
    std::string chat = langManager.get(“chat.type.text”, {“玩家”, “你好”});
    // 输出: “<玩家> 你好”
}

// 设置 TranslationTextComponent 的语言管理器
text::TranslationTextComponent::setLanguageManager(&langManager);

// 使用翻译组件
auto text = std::make_unique<text::TranslationTextComponent>(“item.minecraft.diamond”);
std::cout << text->getUnformattedText();  // 输出: “钻石”
```

### 语言文件格式

语言文件位于 `assets/<namespace>/lang/<lang_code>.json`，格式如下：

```json
{
    “item.minecraft.diamond”: “Diamond”,
    “chat.type.text”: “<%s> %s”,
    “translation.test.positional”: “First: %1$s, Second: %2$s”
}
```

支持的占位符：
- `%s` - 顺序参数
- `%1$s`, `%2$s` - 位置参数
- `%%` - 转义的百分号

## 容易踩的坑

- 不能长期保存 `ResourcePackList` 内部元素地址，现在查询接口返回的是拷贝。
- `containsPack()` 只做存在性判断，不要拿它代替实际加载。
- `addPack()` 会在锁外创建和初始化资源包，再做二次插入校验，不能把它当成纯内存操作。
- `ZipResourcePack` 的缓存已经加锁，但资源包本身仍然应该通过 `ResourcePackList` 统一访问。
- 资源路径统一使用 `/`，Windows 路径分隔符会在内部规范化。
- `LanguageManager::instance()` 返回全局单例，但需要手动调用 `loadLanguage()` 加载语言文件。
- `TranslationTextComponent` 默认使用 `LanguageManager::instance()`，但可以通过 `setLanguageManager()` 覆盖。

## 测试用例

- `tests/common/resource/ResourcePackListSelfContainedTest.cpp`：自包含资源包读取测试。
- `tests/common/resource/LanguageManagerTest.cpp`：语言管理器和翻译组件测试。
- `tests/client/resource/test_resource_location.cpp`：资源定位符相关测试。
