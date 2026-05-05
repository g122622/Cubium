# 马类实体模块

包含所有马类实体的实现。

## 目录结构

```
horse/
├── AbstractHorseEntity.hpp/cpp  # 马类基类
├── HorseEntity.hpp/cpp          # 马
├── DonkeyEntity.hpp/cpp         # 驴
├── MuleEntity.hpp/cpp           # 骡
├── SkeletonHorseEntity.hpp/cpp  # 骷髅马
├── ZombieHorseEntity.hpp/cpp    # 僵尸马
├── LlamaEntity.hpp/cpp          # 羊驼
└── README.md                    # 本文件
```

## 继承层次

```
AnimalEntity
└── AbstractHorseEntity (IRideable, IJumpingMount)
    ├── HorseEntity      # 马 (35种变体)
    ├── DonkeyEntity     # 驴 (背包15格)
    ├── MuleEntity       # 骡 (不育，背包15格)
    ├── SkeletonHorseEntity  # 骷髅马 (亡灵，无需驯服)
    ├── ZombieHorseEntity    # 僵尸马 (亡灵，无需驯服)
    └── LlamaEntity      # 羊驼 (商队，吐口水)
```

## 实体特性

### HorseEntity (马)
| 特性 | 说明 |
|------|------|
| 驯服方式 | 骑乘驯服 |
| 装备栏 | 鞍 + 马铠 |
| 变体 | 7种颜色 × 5种花纹 = 35种 |
| 繁殖物品 | 金苹果、金胡萝卜 |
| 属性 | 随机速度、跳跃、生命值 |

### DonkeyEntity (驴)
| 特性 | 说明 |
|------|------|
| 驯服方式 | 骑乘驯服 |
| 装备栏 | 鞍 + 箱子(15格) |
| 繁殖 | 与马繁殖产生骡 |
| 跳跃 | 较低 |

### MuleEntity (骡)
| 特性 | 说明 |
|------|------|
| 驯服方式 | 骑乘驯服 |
| 装备栏 | 鞍 + 箱子(15格) |
| 繁殖 | 不育 |
| 来源 | 马+驴杂交 |

### SkeletonHorseEntity (骷髅马)
| 特性 | 说明 |
|------|------|
| 驯服 | 无需驯服 |
| 生成 | 雷暴天气陷阱 |
| 免疫 | 溺水、中毒 |
| 燃烧 | 阳光下燃烧 |

### ZombieHorseEntity (僵尸马)
| 特性 | 说明 |
|------|------|
| 驯服 | 无需驯服 |
| 生成 | 命令/刷怪蛋 |
| 免疫 | 溺水、中毒 |
| 燃烧 | 不燃烧 |

### LlamaEntity (羊驼)
| 特性 | 说明 |
|------|------|
| 驯服 | 骑乘驯服 |
| 装备栏 | 地毯 + 箱子(3-15格) |
| 骑乘 | 可骑乘但不可控制 |
| 攻击 | 吐口水攻击 |
| 商队 | 跟随前方羊驼 |
| 变体 | 4种颜色 |

## 接口实现

| 实体 | IRideable | IJumpingMount | IEquipable |
|------|-----------|---------------|------------|
| AbstractHorseEntity | ✅ | ✅ | ✅ |
| HorseEntity | 继承 | 继承 | 继承 |
| DonkeyEntity | 继承 | 继承 | 继承 |
| MuleEntity | 继承 | 继承 | 继承 |
| SkeletonHorseEntity | 继承 | 继承 | 继承 |
| ZombieHorseEntity | 继承 | 继承 | 继承 |
| LlamaEntity | 继承 | 继承 | 继承 |

### IEquipable 接口

`IEquipable` 接口提供装备槽管理功能：

```cpp
class IEquipable {
public:
    virtual i32 getEquipmentSlotCount() const = 0;
    virtual ItemStack getEquipment(i32 slot) const = 0;
    virtual void setEquipment(i32 slot, const ItemStack& item) = 0;
    virtual bool canEquip(const ItemStack& item, i32 slot) const = 0;
};
```

马类实现：
- 槽位 0: 鞍
- 槽位 1: 马铠

### 数据同步

马类实体使用 `DataParameter` 同步状态：
- `STATUS_PARAM`: 鞍、驯服、繁殖、进食、扬蹄、张嘴状态
- `OWNER_UUID_PARAM`: 主人 UUID

## 使用示例

```cpp
// 创建马
auto horse = std::make_unique<HorseEntity>(LegacyEntityType::Unknown, 0);
horse->randomizeAppearance();  // 随机外观
horse->setTame(true);
horse->setSaddle(true);

// 骑乘
horse->onPlayerStartRiding(player);

// 跳跃
horse->startJumping();
// 蓄力...
horse->stopJumping();

// 加速（胡萝卜钓竿）
horse->boost();
```

## 参考

- MC 1.16.5 AbstractHorseEntity
- MC 1.16.5 HorseEntity
- MC 1.16.5 DonkeyEntity
- MC 1.16.5 MuleEntity
- MC 1.16.5 SkeletonHorseEntity
- MC 1.16.5 ZombieHorseEntity
- MC 1.16.5 LlamaEntity
