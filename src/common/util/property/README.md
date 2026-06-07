# Property 模块

方块状态属性系统，参考 Minecraft Java 1.16.5 的 `net.minecraft.state` 包实现。

## 目录结构

```
src/common/util/property/
├── IProperty.hpp           # 属性接口基类（类型擦除）
├── Property.hpp            # 类型安全的属性模板基类
├── BooleanProperty.hpp     # 布尔属性
├── IntegerProperty.hpp     # 整数属性
├── EnumProperty.hpp        # 枚举属性模板
├── DirectionProperty.hpp   # 方向属性（DirectionProperty、AxisProperty）
├── StateContainer.hpp      # 状态容器模板
├── StateHolder.hpp         # 状态持有者基类模板
├── Properties.hpp          # 预定义方块状态属性集合（BlockStateProperties）
└── FluidProperties.hpp     # 流体专用属性定义
```

## 模块概述

Property 模块实现了 Minecraft 的方块状态属性系统，核心功能：
- 定义方块属性（布尔、整数、枚举类型）
- 管理状态组合（预计算所有可能状态）
- 提供 O(1) 复杂度的状态转换
- 序列化支持（字符串与值的相互转换）

## 内部模块关系

```
IProperty (接口基类)
    ↑
Property<T> (模板基类)
    ↑
┌───────────────────────────────────┐
│ BooleanProperty  IntegerProperty  │
│ EnumProperty<E>  DirectionProperty│
└───────────────────────────────────┘
         ↓
StateContainer<Owner, State> ←→ StateHolder<Owner, State>
         ↓
BlockStateProperties / FluidProperties (预定义属性)
```

**核心组件职责：**
- `IProperty`：类型擦除接口，允许统一处理不同类型属性
- `Property<T>`：模板基类，管理允许值列表和值到索引映射
- `BooleanProperty/IntegerProperty/EnumProperty`：具体属性类型实现
- `StateContainer`：预计算所有状态组合，建立转换表
- `StateHolder`：不可变状态对象基类，提供类型安全的状态访问
- `BlockStateProperties`：所有标准方块属性的懒加载单例集合
- `FluidProperties`：流体专用属性（LEVEL_1_8、FALLING）

## 上下游依赖关系

**上游依赖：**
- `common/core/Types.hpp`：基础类型（i32、u32、std::string、std::optional）
- `common/util/Direction.hpp`：Direction、Axis 枚举及其工具类
- `common/world/gen/jigsaw/JigsawOrientation.hpp`：JigsawOrientation 枚举

**下游使用方：**
- `common/world/block/`：Block、BlockState 定义方块状态
- `common/world/fluid/`：FluidState 定义流体状态
- `server/block/`：各种具体方块实现（定义属性、创建状态容器）

## 容易踩的坑

### 1. 状态空间爆炸

多属性组合会导致状态数量急剧增长。例如 `16 * 16 * 16 * 2 * 6 = 49,152` 个状态。Minecraft 通常限制属性数量在 16 个以内，每个属性值数量也要控制。

### 2. 属性实例必须唯一

不同实例的同名属性不相等。必须使用静态属性或预定义属性（如 `BlockStateProperties::LIT()`），不要重复创建同名属性。

### 3. 整数属性范围限制

`IntegerProperty::create()` 的 min 必须 >= 0，max 必须 > min，否则抛异常。

### 4. 状态不可变

状态对象不可修改，`with()` 返回新状态引用，原状态不变。不要尝试调用不存在的 `set()` 方法。

### 5. 布尔属性返回值类型

`Property<bool>::valueAt()` 返回值而非引用（因为 `std::vector<bool>` 特化），不能用非常量引用接收。

### 6. 属性名称格式

属性名称必须符合 `[a-z0-9_]+` 正则，不能包含大写字母或特殊字符（如连字符）。

### 7. 流体等级与方块等级不同

`FluidProperties::LEVEL_1_8()` 范围是 1-8（8=源头），与方块的 `LEVEL_0_15` 不同。映射关系：方块 level=0 对应流体 level=8；方块 level=1-7 对应流体 level=1-7；方块 level=8-15 对应流体 level=8 + falling=true。

### 8. 预定义属性是懒加载单例

`BlockStateProperties` 中的属性通过函数内静态变量实现（C++11 magic statics），线程安全，首次调用时创建。同一属性多次调用返回同一引用。
