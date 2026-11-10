# 本领域共享的基础类型与定义

## 目录结构树

```text
base/
├── BannedIpListTypes.hpp  # 默认构造函数
├── BannedPlayerListTypes.hpp  # 默认构造函数
├── OpListManagerTypes.hpp  # 在 OP 列表等级之上叠加单机主机作弊提升。
└── WhitelistManagerTypes.hpp  # 默认构造函数
```

## 内部模块关系

AccessManager 通过私有实现持有名单和权限列表；base 只暴露条目及权限值类型，admission 与 permission 保存各自的数据和持久化逻辑。

## 上下游外部依赖关系

登录准入、白名单/封禁/OP 命令与服务器权限解析使用门面；内部名单依赖 JSON、文件路径和 Result 错误处理。

## 容易踩的坑

- 外部使用 AccessManager 与 base 值类型，名单实现只供本领域使用。
- 持久化和过期条目处理仍由名单实现负责，门面不持有第二份数据。
