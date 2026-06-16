# 资源包管理 (repository/)

此目录包含资源包列表/仓库的管理类，负责资源包的发现、加载、优先级排序和变更通知。

## 文件说明

| 文件 | 说明 |
|------|------|
| `PackListBase.hpp/cpp` | 资源包列表基类，提取了客户端和服务端的公共逻辑。包括资源包扫描、添加/移除、启用/禁用、优先级管理、并发资源查询和变更通知。使用 `std::shared_mutex` 实现读写锁。 |
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
- 读操作（`hasResource`、`readResource`、`listResources` 等）使用 `shared_lock`
- 写操作（`addPack`、`removePack`、`setEnabled` 等）使用 `unique_lock`
- 查询接口返回 `PackInfo` 按值拷贝，避免暴露内部容器地址

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
