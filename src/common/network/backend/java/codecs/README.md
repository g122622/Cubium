# Java 包编解码

## 目录结构

```text
codecs/
├── JavaCodecBase.hpp              # 编解码器公共辅助设施
├── JavaCodecs.hpp                 # 握手、状态和登录阶段
├── JavaConfigurationCodecs.hpp    # 配置阶段
├── JavaPlayCodecs.hpp            # Play 基础包
├── JavaPlayCodecsExtended.hpp    # Play 扩展包
├── ItemEnchantmentsCodec.hpp     # 附魔组件
├── MobEffectInstanceCodec.hpp    # 状态效果实例
└── PotionContentsCodec.hpp       # 药水内容组件
```

## 内部模块关系

阶段 codec 复用 JavaCodecBase、ByteBuf 原语及组件 codec，将 IR 字段转换为线格式。

## 上下游外部依赖关系

依赖公共 IR、注册表、NBT 与缓冲区；JavaProtocolTables 选取 codec 并登记包方向和 ID。

## 容易踩的坑

- 应查阅 1.21.11 的字段顺序、长度上限和可选值格式，不能仅用同一 codec 的往返证明兼容。
- 容器 ID 使用 VarInt；物品组件补丁以自身计数终止，不能额外添加长度前缀。
- 外部输入的枚举、数量和字符串长度必须在解码边界校验。
