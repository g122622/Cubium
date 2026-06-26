# 资源包管理 (repository/)

此目录包含资源包列表/仓库的管理类，负责资源包的发现、加载、优先级排序和变更通知。

## 文件说明

| 文件 | 说明 |
|------|------|
| `PackListBase.hpp/cpp` | 资源包列表基类，提取了客户端和服务端的公共逻辑。包括资源包扫描、添加/移除、启用/禁用、优先级管理、并发资源查询和变更通知。使用 `std::shared_mutex` 实现读写锁。提供 `readAllResourceVersions()` 和 `listResourceStacks()` 方法支持多数据包标签合并。 |
| `PackRepository.hpp/cpp` | 客户端资源包仓库（原 `ResourcePackList`），默认使用 `PackType::ClientResources`。额外提供 `loadFromSettings`/`saveToSettings` 方法与 `ResourcePackListOption` 集成。 |
| `DataPackRepository.hpp` | 服务端数据包仓库（原 `DataPackList`），默认使用 `PackType::ServerData`。仅提供无 PackType 参数的便捷方法（自动使用 ServerData）。 |

## 继承关系

```
PackListBase (基类)
├── PackRepository (客户端资源包仓库, PackType::ClientResources)
└── DataPackRepository (服务端数据包仓库, PackType::ServerData)
```

## 线程安全

- `PackListBase` 使用 `std::shared_mutex` 实现读写锁
- 读操作（`hasResource`、`readResource`、`listResources`、`listResourceStacks` 等）使用 `shared_lock`
- 写操作（`addPack`、`removePack`、`setEnabled` 等）使用 `unique_lock`
- 查询接口返回 `PackInfo` 按值拷贝，避免暴露内部容器地址

## 多数据包资源访问

`PackListBase` 提供两种资源访问模式：

| 方法 | 行为 | 适用场景 |
|------|------|---------|
| `readTextResource()` | 只返回最高优先级数据包的内容 | 配方、战利品表、进度等非标签资源（覆盖语义） |
| `listResources()` | 返回去重后的路径列表 | 非标签资源列举 |
| `readAllResourceVersions()` | 返回同一资源在所有数据包中的文本内容 | 标签合并时需要单个资源的多版本 |
| `listResourceStacks()` | 返回每个路径在所有数据包中的内容栈 | 标签加载（MC Java 标签合并语义） |

`listResourceStacks()` 返回 `map<string, vector<ResourceVersion>>`，其中 `ResourceVersion` 包含 `packName` 和 `content`。每个路径对应的内容栈按数据包优先级从高到低排序，与 MC Java 的 `listMatchingResourceStacks()` 行为一致。

标签加载器（如 `FunctionLoader::loadFunctionTags()`、`BiomeTagLoader::loadFromDataPackRepository()`）使用 `listResourceStacks()` 实现多数据包标签合并：逆序遍历同一标签的所有数据包版本（从低优先级到高优先级），默认追加条目，`replace=true` 时清空已有条目后追加。此遍历顺序与 MC Java 的 `TagLoader.load()` 一致。

## PackInfo 结构

```cpp
struct PackInfo {
    std::string path;         // 资源包路径
    ResourcePackPtr pack;     // 资源包实例
    bool enabled = true;      // 是否启用
    i32 priority = 0;         // 优先级（越大越优先）
    bool isZip = false;       // 是否是 ZIP 文件
    bool initialized = false; // 是否已初始化
    std::string error;        // 初始化错误信息
};
```
