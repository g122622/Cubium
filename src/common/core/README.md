# Core Module

核心模块提供基础类型定义、错误处理、常量和设置系统，是整个项目最基础的模块，不依赖任何其他内部模块。

## 目录结构

```
src/common/core/
├── Types.hpp                    # 基础类型定义（i8/u8/f32等、游戏类型、枚举）
├── Result.hpp                   # 错误处理系统（Result<T>、Error、ErrorCode）
├── Result.cpp                   # Result 实现
├── Constants.hpp                # 游戏常量（命名空间组织：game/network/entity/item/capacity；world 已迁移至 WorldConstants.hpp）
├── EnumSet.hpp                  # 枚举集合工具（基于 std::bitset）
├── BlockRaycastResult.hpp       # 方块射线投射结果类型
├── GameDirectory.hpp/cpp        # 游戏目录管理器（统一管理所有游戏路径）
├── DefaultValues.hpp            # 集中默认值定义
└── settings/                    # 设置系统
    ├── SettingsBase.hpp/cpp     # 设置基类（JSON 持久化、变更通知）
    ├── SettingsTypes.hpp        # 设置选项类型（Boolean/Range/Float/Enum/String）
    └── ResourcePackListOption.hpp  # 资源包列表选项
```

## 内部模块关系

```
Types.hpp ←── Result.hpp ←── SettingsTypes.hpp
    ↓              ↓               ↓
Constants.hpp   BlockRaycastResult.hpp  SettingsBase.hpp
                                       ↓
                              ResourcePackListOption.hpp
```

- **Types.hpp** 是最基础的文件，仅依赖标准库
- **Result.hpp** 依赖 Types.hpp
- **Constants.hpp** 依赖 Types.hpp、WorldConstants.hpp（mc::world 常量已迁移至 WorldConstants.hpp，Constants.hpp 通过 include 重新导出）
- **EnumSet.hpp** 独立，仅依赖标准库
- **BlockRaycastResult.hpp** 依赖 `common/util/math/Vector3.hpp`、`common/util/Direction.hpp`、`common/world/chunk/ChunkPos.hpp`
- **SettingsTypes.hpp** 依赖 Result.hpp
- **SettingsBase.hpp** 依赖 SettingsTypes.hpp
- **ResourcePackListOption.hpp** 依赖 SettingsTypes.hpp

## 上下游外部依赖关系

**上游依赖（本模块依赖的外部库）：**
- `<cstdint>`, `<string>`, `<optional>`, `<variant>`, `<functional>`, `<filesystem>` - 标准库
- `<nlohmann/json.hpp>` - JSON 序列化
- `<spdlog/spdlog.h>` - 日志输出

**上游依赖（本模块依赖的其他内部模块）：**
- `common/util/math/Vector3.hpp` - BlockRaycastResult 使用
- `common/util/Direction.hpp` - BlockRaycastResult 使用
- `common/world/chunk/ChunkPos.hpp` - BlockRaycastResult 使用

**下游依赖（依赖本模块的模块）：**
- **几乎所有其他模块** - Types.hpp、Result.hpp、Constants.hpp 是整个项目的基础依赖
- `common/world/` - 世界相关类型
- `common/entity/` - 实体系统
- `common/network/` - 网络系统
- `server/` - 服务端
- `client/` - 客户端

## 容易踩的坑

### 1. Result<T> 值访问
- 调用 `result.value()` 前必须检查 `result.success()`，否则会抛出异常
- 安全访问使用 `result.valueOr(defaultValue)` 

### 2. EnumSet 要求
- 枚举类型**必须**有 `Count` 值作为最后一个元素，用于定义 bitset 大小
```cpp
// 正确
enum class MyEnum { A, B, C, Count };
// 错误 - 会导致 EnumSet 无法编译
enum class MyEnum { A, B, C };
```

### 3. Settings 注册顺序
- Option 必须在构造函数中注册，**注册前**不能调用 `load()`
- Option 必须是成员变量，不能是局部变量（指针必须保持有效）

### 4. BlockRaycastResult 访问
- `hitPosition()`、`blockPos()`、`face()`、`distance()` 仅在 `isHit()` 为 true 时有效
- `adjacentPos()` 返回用于放置方块的相邻位置

### 5. FloatOption 精度
- 浮点比较使用 epsilon (0.0001f)
- `isDefault()` 对接近默认值的值可能返回 true

### 6. RangeOption 自动截断
- 值会自动截断到 [min, max] 范围，不会报错

### 7. 常量使用注意事项（见 PROJECT_CONVENTIONS.md）
- 高度限制只能使用 `mc::world::MIN_BUILD_HEIGHT`、`mc::world::MAX_BUILD_HEIGHT`，不能硬编码 0、256
- 区块尺寸只能使用 `mc::world::CHUNK_WIDTH` 等，不能硬编码 16
- `CHUNK_HEIGHT` 和 `MAX_BUILD_HEIGHT` 值不同（384 vs 320），语义也完全不同
- `mc::world` 命名空间的常量定义在 `WorldConstants.hpp` 中，`Constants.hpp` 通过 include 重新导出以保持兼容

### 8. 添加新错误码
- 在 `Result.hpp` 的 `ErrorCode` 枚举中添加
- 常用错误码可添加对应的 `Error` 工厂方法

### 9. 添加新 Option 类型
- 继承 `IOption` 接口
- 实现所有必需方法
