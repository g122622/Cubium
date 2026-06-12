# 铜方块模块 (Copper Blocks)

本目录实现了 Minecraft 铜方块的氧化系统，包括所有可氧化铜变体和涂蜡铜变体。

## 架构概览

### 氧化接口

```
IOxidizableBlock (接口)
├── WeatheringCopperBlock (铜方块基类)
│   ├── WeatheringCopperGrateBlock (铜格栅)
│   ├── WeatheringCopperChainBlock (铜锁链)
│   ├── WeatheringCopperLanternBlock (铜灯笼)
│   └── CopperBulbBlock (铜灯)
├── WeatheringCopperStairBlock (铜楼梯)
├── WeatheringCopperSlabBlock (铜台阶)
├── WeatheringCopperDoorBlock (铜门)
└── WeatheringCopperTrapDoorBlock (铜活板门)
```

### IOxidizableBlock 接口

所有具有氧化等级的方块都实现 `IOxidizableBlock` 接口，提供统一的氧化识别和概率计算。

**核心方法：**

| 方法 | 说明 |
|------|------|
| `getOxidationLevel()` | 获取当前氧化等级 (Unaffected/Exposed/Weathered/Oxidized) |
| `getNextOxidationBlock()` | 获取下一氧化等级对应的方块，Oxidized 返回 nullptr |
| `getOxidationChanceModifier()` | 氧化概率修正系数，Unaffected=0.75，其余=1.0 |
| `tryOxidize()` | 执行完整的曼哈顿距离氧化算法 |

**氧化算法流程 (`IOxidizableBlock::tryOxidize()`)：**

1. 若已是最高氧化等级 (Oxidized)，直接返回
2. 获取下一氧化等级方块 (nullptr 则返回)
3. 外层门限概率：0.05688889 (~5.69%)
4. 扫描 4 格曼哈顿距离内的可氧化方块 (`dynamic_cast<IOxidizableBlock>`)
5. 若存在更低等级邻居 → 取消氧化
6. 计算最终概率：`f = (k+1)/(k+j+1)`, `f1 = f² × chanceModifier`
   - k = 更高等级邻居数
   - j = 同等级邻居数
7. 通过概率则替换为下一等级方块，使用 `withPropertiesOf()` 保留共有属性

### 继承方式

- **WeatheringCopperBlock** 继承 `Block` + `IOxidizableBlock`，作为简单铜方块的基类
- **WeatheringCopperStairBlock** 继承 `StairsBlock` + `IOxidizableBlock`
- **WeatheringCopperSlabBlock** 继承 `SlabBlock` + `IOxidizableBlock`
- **WeatheringCopperDoorBlock** 继承 `DoorBlock` + `IOxidizableBlock`（仅下半部分触发氧化，上半部分联动替换）
- **WeatheringCopperTrapDoorBlock** 继承 `TrapDoorBlock` + `IOxidizableBlock`

所有变体类在 `randomTick()` 中调用 `tryOxidize()` 执行氧化算法。Door 变体额外处理上半部分联动。

### 属性保留

氧化替换方块时，`tryOxidize()` 使用 `StateHolder::withPropertiesOf()` 方法自动保留共有属性（如楼梯朝向、台阶类型、含水状态等），确保氧化不会丢失方块状态。

### 涂蜡变体

每种铜变体都有对应的涂蜡版本（`Waxed*` 类），涂蜡方块不实现 `IOxidizableBlock`，不会氧化。

## 文件说明

| 文件 | 说明 |
|------|------|
| `IOxidizableBlock.hpp/cpp` | 氧化接口，包含共享的曼哈顿距离氧化算法 |
| `WeatheringCopperBlock.hpp/cpp` | 基础可氧化铜方块 |
| `WeatheringCopperStairBlock.hpp/cpp` | 可氧化铜楼梯 |
| `WeatheringCopperSlabBlock.hpp/cpp` | 可氧化铜台阶 |
| `WeatheringCopperDoorBlock.hpp/cpp` | 可氧化铜门（上下半部分联动） |
| `WeatheringCopperTrapDoorBlock.hpp/cpp` | 可氧化铜活板门 |
| `WeatheringCopperGrateBlock.hpp/cpp` | 可氧化铜格栅（含水） |
| `WeatheringCopperChainBlock.hpp/cpp` | 可氧化铜锁链（含水，轴向放置） |
| `WeatheringCopperLanternBlock.hpp/cpp` | 可氧化铜灯笼（含水，悬挂/站立） |
| `CopperBulbBlock.hpp/cpp` | 铜灯（红石控制，LIT/POWERED 状态） |
