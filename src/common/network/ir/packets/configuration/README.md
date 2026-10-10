# Configuration 包定义

## 目录结构

```text
configuration/
└── ConfigurationPackets.hpp  # 注册表、标签、客户端偏好与阶段协商结构
```

## 内部模块关系

配置包聚合到 ConfigurationPacket；Play 客户端偏好包组合相同的 ClientInformation 负载。

## 上下游外部依赖关系

依赖公共基础类型和 BedrockMeta；Java codec 编解码，握手状态机及服务器应用层消费。

## 容易踩的坑

- viewDistance 是有符号字节；各枚举和语言长度在 codec 边界校验。
- 配置阶段和 Play 阶段的客户端设置必须写入同一连接状态，不能只更新握手缓存。
- 重新配置后需将新偏好应用到已有玩家，不能重复创建玩家实体。
