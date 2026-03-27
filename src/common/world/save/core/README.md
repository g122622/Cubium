# 核心组件 (Core)

提供存档系统的核心基础设施。

## 文件说明

| 文件 | 职责 |
|------|------|
| `SaveFormat.hpp/cpp` | 存档格式管理器，负责检测和加载世界存档 |
| `LevelSave.hpp/cpp` | 单个世界的存档操作，管理世界目录结构 |
| `SessionLock.hpp/cpp` | 会话锁，防止多进程同时访问同一存档 |
| `DataVersion.hpp/cpp` | 数据版本常量，用于存档版本兼容性 |

## 模块关系

```
SaveFormat
    └── LevelSave (一对多)
            ├── SessionLock
            └── RegionFileCache
```

## 使用流程

1. `SaveFormat::createLevelSave()` 创建或打开世界存档
2. `LevelSave` 提供世界目录路径访问
3. `SessionLock` 确保单进程访问

## 会话锁机制

会话锁使用文件锁实现：
- Windows: `LockFileEx` / `UnlockFileEx`
- Linux/macOS: `flock` / `fcntl`

当另一个进程持有锁时，打开存档会失败并返回错误。
