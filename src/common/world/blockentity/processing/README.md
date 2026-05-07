# 加工类方块实体模块

提供熔炉、高炉、烟熏炉、信标、潮涌核心等加工类方块实体的实现。

## 目录结构

```
processing/
├── AbstractFurnaceEntity.hpp/cpp  # 熔炉基类
├── FurnaceEntity.hpp/cpp          # 普通熔炉
├── BlastFurnaceEntity.hpp/cpp     # 高炉
├── SmokerEntity.hpp/cpp           # 烟熏炉
├── FurnaceInventory.hpp/cpp       # 熔炉背包
├── BrewingStandEntity.hpp/cpp     # 酿造台
├── BeaconEntity.hpp/cpp           # 信标
├── ConduitEntity.hpp/cpp          # 潮涌核心
└── README.md
```

## 文件详解

### AbstractFurnaceEntity.hpp/cpp

**职责**：熔炉方块实体基类，提供燃烧和熔炼的通用逻辑。

**主要功能**：
- 燃烧管理（燃烧时间、燃料消耗）
- 熔炼进度（熔炼时间、配方匹配）
- 红石比较器信号
- 锁定功能（继承自LockableBlockEntity）
- **ISidedInventory 接口支持**
- **火苗噼啪音效**（燃烧时 1/20 概率播放）

**火苗噼啪音效（MC 1.16.5 对齐）**：
```cpp
// AbstractFurnaceEntity::tick() 中实现
if (isBurning()) {
    if (!world.isClientSide() && world.getRandom().nextInt(20) == 0) {
        world.playSound(getFireCrackleSound(), SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }
}
```

**槽位访问规则（ISidedInventory）**：
- 上方 (Direction::Up)：输入槽（槽位 0）
- 下方 (Direction::Down)：输出槽（槽位 2）、燃料槽（槽位 1）
- 侧面：燃料槽（槽位 1）

**熔炼状态机** (参考 MC 1.16.5)：
```
每tick:
1. 如果燃烧中: burnTime--

2. 如果可以熔炼:
   if (burnTime <= 0 && 有燃料):
       消耗燃料
       burnTime = burnTimeTotal
       cookTimeTotal = 配方.熔炼时间

   if (burnTime > 0):
       cookTime++
       if (cookTime >= cookTimeTotal):
           执行熔炼
           cookTime = 0

3. 如果不燃烧但有进度:
   cookTime -= 2  // 进度回退，而非清零
```

**关键方法**：
- `tick()` - 每tick更新
- `isBurning()` - 检查是否正在燃烧
- `getComparatorSignal()` - 红石比较器信号
- `isFuel()` - 检查物品是否为燃料
- `getBurnTime()` - 获取燃料燃烧时间

### FurnaceEntity.hpp/cpp

**职责**：普通熔炉实体。

**特性**：
- 熔炼时间：200 tick
- 配方类型：SMELTING
- 经验倍率：1.0

### BlastFurnaceEntity.hpp/cpp

**职责**：高炉实体，冶炼矿石和金属。

**特性**：
- 熔炼时间：100 tick（2倍速度）
- 配方类型：BLASTING
- 经验倍率：0.5
- 仅能熔炼矿石和金属物品
- **燃料消耗速度是普通熔炉的2倍**（同样燃料只能燃烧一半时间）

**MC 1.16.5 对齐**：
- `canSmelt()` 仅接受 BLASTING 类型配方
- `getBurnTimeForFuel()` 返回基础燃烧时间的一半

### SmokerEntity.hpp/cpp

**职责**：烟熏炉实体，烹饪食物。

**特性**：
- 熔炼时间：100 tick（2倍速度）
- 配方类型：SMOKING
- 经验倍率：0.5
- 仅能烹饪食物
- **燃料消耗速度是普通熔炉的2倍**（同样燃料只能燃烧一半时间）

**MC 1.16.5 对齐**：
- `canSmelt()` 仅接受 SMOKING 类型配方
- `getBurnTimeForFuel()` 返回基础燃烧时间的一半

### FurnaceInventory.hpp/cpp

**职责**：熔炉专用的3槽背包。

**槽位定义**：
```cpp
static constexpr i32 SLOT_INPUT = 0;   // 输入槽
static constexpr i32 SLOT_FUEL = 1;    // 燃料槽
static constexpr i32 SLOT_OUTPUT = 2;  // 输出槽
```

**便捷方法**：
- `getInputItem()` / `setInputItem()` - 输入槽操作
- `getFuelItem()` / `setFuelItem()` - 燃料槽操作
- `getOutputItem()` / `setOutputItem()` - 输出槽操作
- `consumeInput()` / `consumeFuel()` - 消耗物品
- `addToOutput()` - 向输出槽添加物品

### BeaconEntity.hpp/cpp

**职责**：信标方块实体，提供金字塔效果。

**特性**：
- 金字塔等级检测（1-4层）
- 主效果：速度、急迫、抗性提升、跳跃提升、力量
- 辅助效果：生命恢复（4层金字塔）
- 效果范围：等级 × 10 + 10 格
- 支付物品：铁锭、金锭、钻石、绿宝石、下界合金锭
- 光束渲染：支持染色玻璃颜色叠加

**MC 1.16.5 对齐**：
- 每 80 tick 检测金字塔结构
- 每 80 tick 应用效果
- 光束颜色混合算法

### ConduitEntity.hpp/cpp

**职责**：潮涌核心方块实体，水下信标。

**特性**：
- 水包围检测：中心周围 3x3x3 必须全部是水
- 框架检测：海晶石、海晶石砖、暗海晶石、海晶灯
- 激活条件：至少 16 个框架方块
- 效果范围：(框架数 / 7) × 16 格
- 攻击能力：42+ 框架方块时可攻击敌对生物

**效果机制**：
- 潮涌能量：持续 260 tick
- 每 40 tick 检测结构和应用效果
- 仅对在水中（isWet）的玩家生效

**MC 1.16.5 对齐**：
- 框架检测位置计算
- 敌对生物攻击逻辑
- 目标追踪和持久化

## 模块关系

```mermaid
graph TB
    BlockEntity[BlockEntity]
    ContainerBlockEntity[ContainerBlockEntity]
    LockableBlockEntity[LockableBlockEntity]
    AbstractFurnaceEntity[AbstractFurnaceEntity]
    FurnaceEntity[FurnaceEntity]
    BlastFurnaceEntity[BlastFurnaceEntity]
    SmokerEntity[SmokerEntity]
    FurnaceInventory[FurnaceInventory]
    SmeltingRecipe[SmeltingRecipe]
    IInventory[IInventory]

    BlockEntity --> ContainerBlockEntity
    ContainerBlockEntity --> LockableBlockEntity
    LockableBlockEntity --> AbstractFurnaceEntity
    AbstractFurnaceEntity --> FurnaceEntity
    AbstractFurnaceEntity --> BlastFurnaceEntity
    AbstractFurnaceEntity --> SmokerEntity
    AbstractFurnaceEntity -.组合.-> FurnaceInventory
    FurnaceInventory -.实现.-> IInventory
    AbstractFurnaceEntity -.依赖.-> SmeltingRecipe
```

## 依赖项

### 内部依赖
- `world/blockentity/core/LockableBlockEntity.hpp` - 可锁定基类
- `entity/inventory/IInventory.hpp` - 背包接口
- `item/crafting/SmeltingRecipe.hpp` - 熔炼配方

### 外部依赖
- `<memory>` - 智能指针
- `<array>` - 静态数组

## 使用方法

### 创建熔炉实体

```cpp
// 创建普通熔炉
auto furnace = std::make_unique<FurnaceEntity>(BlockPos(0, 0, 0));

// 设置输入物品
furnace->getFurnaceInventory().setInputItem(ItemStack(Items::IRON_ORE, 32));

// 设置燃料
furnace->getFurnaceInventory().setFuelItem(ItemStack(Items::COAL, 64));

// 检查燃烧状态
if (furnace->isBurning()) {
    i32 remaining = furnace->getBurnTime();
    i32 progress = furnace->getCookTime();
}
```

### 创建高炉

```cpp
auto blastFurnace = std::make_unique<BlastFurnaceEntity>(BlockPos(0, 0, 0));
// 高炉只能熔炼矿石和金属
```

### 创建烟熏炉

```cpp
auto smoker = std::make_unique<SmokerEntity>(BlockPos(0, 0, 0));
// 烟熏炉只能烹饪食物
```

## 三种熔炉对比

| 特性 | 普通熔炉 | 高炉 | 烟熏炉 |
|-----|---------|------|-------|
| 熔炼时间 | 200 tick | 100 tick | 100 tick |
| 配方类型 | SMELTING | BLASTING | SMOKING |
| 可熔炼物 | 全部 | 仅矿石/金属 | 仅食物 |
| 经验倍率 | 1.0 | 0.5 | 0.5 |

## 容易踩的坑

### 1. 熔炼进度回退

不燃烧时进度应该回退2，而非清零：

```cpp
// 错误：清零进度
if (!isBurning()) {
    m_cookTime = 0;
}

// 正确：回退进度
if (!isBurning() && m_cookTime > 0) {
    m_cookTime -= 2;
}
```

### 2. 燃料消耗时机

燃料应该在燃烧时间耗尽且需要继续熔炼时才消耗：

```cpp
// 错误：每次都消耗燃料
if (canSmelt()) {
    burnFuel();
}

// 正确：燃烧时间耗尽时才消耗
if (m_burnTime <= 0 && canSmelt() && hasFuel()) {
    burnFuel();
}
```

### 3. 输出槽满检查

熔炼前必须检查输出槽是否可以接受产物：

```cpp
// 错误：未检查输出槽
if (!m_inventory.isInputEmpty()) {
    smelt();
}

// 正确：检查输出槽
if (canSmelt()) {
    smelt();
}

bool canSmelt() const {
    ItemStack output = getRecipeOutput();
    ItemStack current = m_inventory.getOutputItem();
    return current.isEmpty() ||
           (current.canStackWith(output) &&
            current.getCount() + output.getCount() <= current.getMaxStackSize());
}
```

### 4. 配方缓存

每次输入变化时重新查询配方：

```cpp
void tick() {
    // 检查输入是否变化
    ItemStack input = m_inventory.getInputItem();
    if (input != m_lastInput) {
        m_lastRecipe = findRecipe(input);
        m_lastInput = input;
    }
}
```

## 测试用例

测试文件位于 `tests/common/world/blockentity/`：

- `FurnaceEntityTest.cpp` - 熔炉实体测试（含燃烧时间测试）
- `FurnaceInventoryTest.cpp` - 熔炉背包测试
- `SmeltingRecipeTest.cpp` - 熔炼配方测试

### 测试覆盖

- 燃烧时间管理
- 熔炼进度
- 配方匹配
- 输出槽满处理
- 红石比较器信号
- 锁定功能
- 序列化和反序列化

## 燃烧时间数据（MC 1.16.5 对齐）

### 燃烧时间表

| 燃料 | 燃烧时间 (tick) | 燃烧时间 (秒) |
|-----|----------------|--------------|
| **岩浆桶** | 20000 | 1000 |
| **煤炭块** | 16000 | 800 |
| **烈焰棒** | 2400 | 120 |
| **煤炭/木炭** | 1600 | 80 |
| **干海带块** | 4001 | ~200 |
| **脚手架** | 400 | 20 |
| **原木/木板/木头/去皮原木/去皮木头** | 300 | 15 |
| **木质楼梯/栅栏/栅栏门/门/活板门/压力板** | 300 | 15 |
| **书架/音符盒/合成台/光照探测器** | 300 | 15 |
| **弓/钓鱼竿/弩** | 300 | 15 |
| **木质台阶** | 150 | 7.5 |
| **木制工具（镐/斧/锹/锄/剑）** | 200 | 10 |
| **木棍/碗/树苗/木质按钮/羊毛** | 100 | 5 |
| **地毯** | 67 | ~3.3 |
| **竹子** | 50 | 2.5 |

### 实现细节

燃烧时间数据在 `AbstractFurnaceEntity.cpp` 的 `getBurnTimeByItem()` 函数中实现。

**物品识别方式**：
- 已注册物品（如煤炭、木棍）：通过 `Items::XXX` 静态指针比较
- 方块物品（如原木、木板）：通过 `BlockItemRegistry` 获取对应物品

**添加新燃料的步骤**：
1. 确保物品/方块已在 `Items.hpp` 或 `VanillaBlocks.hpp` 中声明
2. 确保物品/方块已在 `Items.cpp` 或 `VanillaBlocks.cpp` 中注册
3. 确保方块物品已在 `BlockItemRegistry.cpp` 中注册
4. 在 `getBurnTimeByItem()` 中添加燃烧时间判断

**参考**：MC 1.16.5 `AbstractFurnaceTileEntity.getBurnTimes()`
