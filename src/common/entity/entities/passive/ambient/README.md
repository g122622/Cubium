# 环境生物模块

环境生物模块负责蝙蝠这类低频率、低干扰的实体实现。它们通常继承自通用的 `MobEntity`，但会使用独立的声音分类和更低的环境声触发频率。

## 目录结构

```text
src/common/entity/entities/passive/ambient/
├── AmbientEntity.hpp  # 环境生物基类，设置环境音分类
├── AmbientEntity.cpp  # 环境生物通用实现
├── BatEntity.hpp      # 蝙蝠实体声明
├── BatEntity.cpp      # 蝙蝠实体实现
└── README.md          # 本文档
```

## 文件介绍

### AmbientEntity.hpp / AmbientEntity.cpp

- 作为环境生物的公共基类。
- 统一把声音分类设置为 `SoundCategory::Ambient`。
- 给子类提供比普通动物更安静、更稀疏的环境声语义。

### BatEntity.hpp / BatEntity.cpp

- 蝙蝠的具体实现。
- 继承环境生物的声音分类和基础移动能力。

## 模块关系

- `AmbientEntity` 继承 `MobEntity`，复用受伤、死亡和环境声播放链路。
- `Entity::playSound(...)` 会把环境声事件转发到 `IWorld::playSound(...)`。
- `ServerWorld` 再把这些事件交给服务器广播层。

## 整体职责

该模块的职责是提供“环境生物”这一类实体的共同行为，重点是声音分类和低频环境音语义，而不是复杂 AI。

## 输入 / 输出

### 输入

- 世界 tick
- 生物空闲时长
- 伤害与死亡事件

### 输出

- 环境声事件
- 受伤/死亡声音事件
- 由服务器广播到客户端的音效数据包

## 依赖项

- `entity/core/MobEntity.hpp`
- `common/sound/SoundCategory.hpp`
- `world/IWorld.hpp`

## 使用方法

```cpp
class TestBat : public AmbientEntity {
public:
    TestBat(EntityId id) : AmbientEntity(LegacyEntityType::Bat, id) {}
};
```

## 容易踩的坑

- 不要把环境生物当作普通怪物来处理声音分类，否则混音通道会被放错。
- 环境声应通过 `playAmbientSound()` 触发，不要在外部绕过 `MobEntity` 的统一入口。

## 测试用例

- [tests/entity/LivingEntityTests.cpp](../../../../../../tests/entity/LivingEntityTests.cpp) 中的环境声测试会验证声音事件是否能从实体传到世界。

## Mermaid 图表

```mermaid
flowchart TD
    Mob[MobEntity] --> Ambient[AmbientEntity]
    Ambient --> Cat[SoundCategory::Ambient]
    Ambient --> Play[playAmbientSound()]
    Play --> World[IWorld::playSound()]
    World --> Server[ServerWorld 广播]

    style Mob fill:#8ecae6,stroke:#1d4ed8,color:#111
    style Ambient fill:#90be6d,stroke:#2f6f3e,color:#111
    style Cat fill:#f4a261,stroke:#b45309,color:#111
    style Play fill:#e9c46a,stroke:#a16207,color:#111
    style World fill:#cdb4db,stroke:#6d28d9,color:#111
    style Server fill:#f28482,stroke:#b91c1c,color:#111
```
