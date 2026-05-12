# 特殊方块模块 (Special Blocks)

特殊方块模块提供管理、装饰和特殊功能方块的实现。

## 目录结构

```
special/
├── README.md            # 本文档
├── SpecialBlocks.hpp/cpp # 所有特殊方块
```

## 方块类型

### 管理类方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `BarrierBlock` | 屏障（不可见不可破坏） | 无 |
| `StructureVoidBlock` | 结构空位 | 无 |
| `StructureBlock` | 结构方块 | MODE |
| `JigsawBlock` | 拼图方块 | ORIENTATION |

### 命令方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `CommandBlock` | 脉冲命令方块 | FACING, CONDITIONAL, POWERED |
| `RepeatingCommandBlock` | 循环命令方块 | 同上 |
| `ChainCommandBlock` | 连锁命令方块 | 同上 |

### 物理方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `SlimeBlock` | 粘液块（弹跳） | 无 |
| `HoneyBlock` | 蜂蜜块（粘滞） | 无 |

### 功能方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `SpongeBlock` | 海绵（吸水） | 无 |
| `WetSpongeBlock` | 湿润海绵 | 无 |
| `WebBlock` | 蜘蛛网（减速） | 无 |

## 核心机制

### 屏障方块
- 完全不透明但不可见
- 只有创造模式可见轮廓
- 不可破坏（生存模式）

### 命令方块
- 脉冲：单次执行
- 循环：每 tick 执行
- 连锁：前方命令后执行
- 可设置条件执行

### 粘液块
- 实体落在上面弹跳
- 活塞推动时粘住方块
- 弹跳高度可调整

### 蜂蜜块
- 实体在上面减速
- 活塞推动时粘住方块
- 无弹跳效果

### 海绵吸水机制

**SpongeBlock** 实现完整的吸水逻辑，参考 MC 1.16.5 `net.minecraft.block.SpongeBlock`：

#### 吸水算法 (BFS)
1. 从海绵位置开始广度优先搜索
2. 最大搜索深度：**6 格**（可吸收到距离 7 的相邻水方块）
3. 最多吸收数量：**65 个水方块**
4. 超过限制立即停止

#### 可吸收的水类型
| 类型 | 检测方式 | 处理方法 |
|------|----------|----------|
| 水源方块 | `IBucketPickupHandler` | `pickupFluid()` 移除水源 |
| 流动水 | `LiquidBlock` | 设置为空气方块 |
| 海洋植物 | `Material::OCEAN_PLANT` | 设置为空气（掉落物品待实现） |
| 海草 | `Material::SEA_GRASS` | 设置为空气（掉落物品待实现） |

#### 触发时机
- `onBlockAdded()` - 方块放置时
- `neighborChanged()` - 邻居方块更新时

#### 吸水后效果
- 海绵变为湿润海绵 (`WetSpongeBlock`)
- 播放方块破坏效果（事件 2001，数据为水的方块状态 ID）

#### 已知限制
- 海洋植物/海草的物品掉落尚未实现（需要 `Block::spawnDrops` 方法支持）
- 当前实现直接移除方块，不生成掉落物

### 湿润海绵干燥机制

**WetSpongeBlock** 在下界自动干燥：

#### 触发条件
- `onBlockAdded()` 在超热维度（`isUltraWarm() == true`）

#### 干燥效果
- 变为干海绵 (`SpongeBlock`)
- 播放蒸汽效果（事件 2009）
- 播放火焰熄灭音效

### 蜘蛛网
- 实体经过大幅减速
- 水平速度 × 0.025
- 下落速度 × 0.05

## 使用方法

```cpp
// 创建屏障
auto barrier = std::make_unique<BarrierBlock>(
    BlockProperties(Material::BARRIER)
        .hardness(-1.0f)  // 不可破坏
);

// 创建命令方块
auto commandBlock = std::make_unique<CommandBlock>(
    BlockProperties(Material::REDSTONE_LIGHT)
        .hardness(-1.0f)
);

// 创建粘液块
auto slime = std::make_unique<SlimeBlock>(
    BlockProperties(Material::CLAY)
        .hardness(0.0f)
        .resistance(0.0f)
);

// 创建海绵（已通过 VanillaBlocks 注册）
Block* sponge = VanillaBlocks::SPONGE;
Block* wetSponge = VanillaBlocks::WET_SPONGE;

// 手动触发吸水
SpongeBlock* spongeBlock = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
if (spongeBlock) {
    bool absorbed = spongeBlock->tryAbsorbWater(world, pos);
}
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `world/WorldEvents` | 世界事件常量 |
| `world/fluid/FluidTags` | 流体标签检测 |
| `world/block/IBucketPickupHandler` | 水源舀取接口 |
| `world/block/blocks/LiquidBlock` | 液体方块类型检测 |
| `util/Direction` | 方向遍历 |

## 测试覆盖

- `tests/common/world/block/blocks/SpongeBlockTest.cpp`
  - SpongeBlockTest: 方块注册、属性验证、吸水逻辑
  - WetSpongeBlockTest: 下界干燥、主世界保持湿润
  - WorldEventsTest: 事件常量验证
