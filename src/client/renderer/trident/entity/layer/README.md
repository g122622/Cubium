# 层渲染器系统

本目录包含实体层渲染器，用于在基础模型上添加额外渲染层。

## 目录结构

```
layer/
├── core/                   # 层渲染器核心
│   ├── LayerRenderer.hpp   # 层渲染器基类
│   └── README.md
├── equipment/              # 装备层
│   ├── ArmorLayer.hpp/cpp  # 盔甲层
│   ├── HeldItemLayer.hpp/cpp # 手持物品层
│   ├── HeadLayer.hpp/cpp   # 头部物品层
│   └── README.md
├── cosmetic/               # 外观层
│   ├── CapeLayer.hpp/cpp   # 斗篷层
│   ├── ElytraLayer.hpp/cpp # 鞘翅层
│   └── README.md
├── entity/                 # 实体特性层
│   ├── SaddleLayer.hpp/cpp # 鞍层
│   ├── SheepWoolLayer.hpp/cpp # 羊毛层
│   ├── VillagerLayer.hpp   # 村民多层纹理层
│   ├── ArrowLayer.hpp/cpp  # 箭矢附着层
│   └── README.md
└── effect/                 # 效果层
    ├── EnergyGlintLayer.hpp/cpp # 附魔光效层
    ├── EyesLayer.hpp/cpp    # 发光眼睛层
    └── README.md
```

## LayerRenderer 基类

层渲染器基类，用于在基础实体模型上添加额外渲染层。

```cpp
template<typename TEntity>
class LayerRenderer {
public:
    virtual ~LayerRenderer() = default;
    
    virtual void render(
        TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) = 0;
    
    virtual bool shouldRender(const TEntity& entity) const { return true; }
};
```

## 层渲染器类型

### 装备层 (equipment/)
- **ArmorLayer**: 盔甲渲染，支持皮革染色
- **HeldItemLayer**: 手持物品渲染，支持手臂动画跟随
  - 通过 `BipedModel::translateHand()` 获取手臂变换矩阵
  - 物品正确跟随手臂动画旋转和平移
  - 参考 MC 1.16.5 `HeldItemLayer.func_229135_a_`
- **HeadLayer**: 头部物品渲染（头盔、南瓜等）

### 外观层 (cosmetic/)
- **CapeLayer**: 斗篷渲染
- **ElytraLayer**: 鞘翅渲染

### 实体特性层 (entity/)
- **SaddleLayer**: 鞍渲染（马、猪等）
- **SheepWoolLayer**: 羊毛渲染（羊），支持染色羊毛和 jeb_ 彩虹羊
- **VillagerLayer**: 村民多层纹理渲染，根据职业和生物群系叠加纹理
- **ArrowLayer**: 箭矢附着渲染
- **HeldBlockLayer**: 方块持有渲染（末影人），使用 `ChunkMesher::getDefaultBlockTintColor()` 获取方块默认着色颜色

### 效果层 (effect/)
- **EnergyGlintLayer**: 附魔光效渲染
- **EyesLayer**: 发光眼睛渲染（蜘蛛、末影人等）

## 渲染顺序

层渲染器按添加顺序依次渲染：
1. 基础模型
2. 装备层（盔甲、手持物品）
3. 外观层（斗篷、鞘翅）
4. 实体特性层（鞍、羊毛）
5. 效果层（附魔光效、发光眼睛）

## 参考

- MC 1.16.5 LayerRenderer
- MC 1.16.5 ArmorLayer
- MC 1.16.5 ElytraLayer
- MC 1.16.5 HeadLayer
