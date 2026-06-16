# 资源包接口与实现 (pack/)

此目录包含资源包的抽象接口和具体实现。

## 文件说明

| 文件 | 说明 |
|------|------|
| `IResourcePack.hpp/cpp` | 资源包抽象接口。所有资源访问方法需显式传入 `PackType` 参数。提供静态保护方法 `normalizePath`、`makeTypedPath`、`matchesExtension` 供子类复用。 |
| `FolderResourcePack.hpp/cpp` | 文件夹资源包，从磁盘目录读取资源。路径规范化使用 `weakly_canonical` + 大小写规范化。 |
| `ZipResourcePack.hpp/cpp` | ZIP 资源包，从 ZIP 文件读取资源。内部缓存已加锁（线程安全）。 |
| `InMemoryResourcePack.hpp/cpp` | 内存资源包，用于原版默认资源。通过 `addResource(PackType, path, content)` 添加资源。 |
| `PackMetadata.hpp/cpp` | `pack.mcmeta` 文件解析，提取包格式版本和描述信息。 |

## 继承关系

```
IResourcePack (抽象接口)
├── FolderResourcePack (文件夹资源包)
├── ZipResourcePack (ZIP 资源包)
└── InMemoryResourcePack (内存资源包)
```

## 路径规范化

所有实现类共享 `IResourcePack` 提供的路径工具方法：
- `normalizePath(path)` — 反斜杠转正斜杠，移除前导斜杠
- `makeTypedPath(type, path)` — 构造带 PackType 目录前缀的完整路径
- `matchesExtension(path, extension)` — 统一扩展名匹配（去除前导点号后比较）
