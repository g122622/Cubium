# 粒子数据 (Particle Data)

## 概述

粒子数据类用于携带粒子参数，支持网络序列化和命令行解析。参考 MC 1.16.5 IParticleData 接口。

## 目录结构

```
data/
├── ParticleData.hpp           # 粒子数据基类
├── BasicParticleData.hpp/cpp  # 无参数粒子数据
├── BlockParticleData.hpp/cpp  # 方块粒子数据
├── ItemParticleData.hpp/cpp   # 物品粒子数据
├── RedstoneParticleData.hpp/cpp # 红石粒子数据
└── README.md
```

## 类设计

### ParticleData（基类）

```cpp
class ParticleData {
public:
    virtual ~ParticleData() = default;

    [[nodiscard]] virtual ParticleTypeId getType() const = 0;
    [[nodiscard]] virtual String getTypeName() const = 0;
    [[nodiscard]] virtual String getParameters() const { return ""; }
    [[nodiscard]] virtual std::unique_ptr<ParticleData> clone() const = 0;
};
```

### 数据类型对应关系

| 粒子类型 | 数据类 | 参数 |
|----------|--------|------|
| Flame, Smoke 等 | BasicParticleData | 无 |
| Block, Breaking | BlockParticleData | BlockState |
| Item | ItemParticleData | ItemStack |
| Redstone | RedstoneParticleData | RGB 颜色 |
| DustColorTransition | DustColorTransitionData | 起始色 + 终止色 |

## 使用示例

```cpp
// 创建基础粒子数据
auto flameData = std::make_unique<BasicParticleData>(ParticleTypeId::Flame);

// 创建方块粒子数据
BlockState state = BlockRegistry::get(BlockId::Stone).getDefaultState();
auto blockData = std::make_unique<BlockParticleData>(ParticleTypeId::Block, state);

// 创建物品粒子数据
ItemStack stack(ItemId::Diamond, 1);
auto itemData = std::make_unique<ItemParticleData>(ParticleTypeId::Item, stack);

// 创建红石粒子数据（自定义颜色）
auto redstoneData = std::make_unique<RedstoneParticleData>(glm::vec3(1.0f, 0.5f, 0.0f));

// 通过注册表创建粒子
auto particle = ParticleRegistry::instance().createParticle(
    flameData->getType(),
    position,
    velocity
);
```

## 网络序列化

粒子数据用于网络包传输，支持从服务端发送粒子效果到客户端：

```cpp
// 服务端发送粒子效果
void sendParticleEffect(PacketSerializer& serializer, const ParticleData& data) {
    serializer.writeVarInt(static_cast<i32>(data.getType()));
    data.serialize(serializer);
}

// 客户端接收粒子效果
std::unique_ptr<ParticleData> receiveParticleEffect(PacketDeserializer& deserializer) {
    ParticleTypeId type = static_cast<ParticleTypeId>(deserializer.readVarInt());
    return ParticleDataFactory::createFromType(type, deserializer);
}
```

## 命令行解析

粒子数据用于 `/particle` 命令的参数解析：

```
/particle minecraft:block minecraft:stone ~ ~ ~ 0.5 0.5 0.5 10
/particle minecraft:redstone 1.0 0.0 0.0 ~ ~ ~ 0.5 0.5 0.5 10
```

## 参考

- Minecraft Java 1.16.5 `net.minecraft.particles.IParticleData`
- Minecraft Java 1.16.5 `net.minecraft.particles.BlockParticleData`
- Minecraft Java 1.16.5 `net.minecraft.particles.ItemParticleData`
- Minecraft Java 1.16.5 `net.minecraft.particles.RedstoneParticleData`
