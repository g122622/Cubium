# 📋 Cubium 项目代码规范 v1.0

## 1. 总则

### 1.1 规范目的

本规范旨在确保项目代码的**一致性**、**可维护性**、**可读性**和**安全性**，降低团队协作成本，提高代码质量。

### 1.2 例外处理

如有特殊情况需要违反本规范，必须：
1. 立即停止，向用户询问，征得同意之后方可继续
2. 在代码中添加明确注释说明原因
3. 结束任务之后给用户明确反馈

### 1.3 有关防御性编程

不要过度防御性编程，这会导致代码臃肿、不易发现真正的bug和架构缺陷。只在必要的边界和不受信任的输入处进行防御性检查，其他地方可以假设前置条件已经满足。

❌ 不好的例子：

```cpp
    // 区块 tick - 包括区块内实体、方块随机刻、区块状态更新等
    if (m_chunkManager && pendingTasksCount >= 0) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::ChunkTick");
        m_chunkManager->tick(pendingTasksCount);
    }

    // 光照更新 - 限制每 tick 最多处理 32768 个区块，避免过长卡顿
    if (m_lightManager && m_lightManager->hasLightWork()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::LightManager");
        m_lightManager->tick(32768, true, true);
    }

    if (m_skyLight != nullptr) {
        m_skyLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
    }
```

✅ 正确例子，最佳实践：

```cpp
    // 区块 tick - 包括区块内实体、方块随机刻、区块状态更新等
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::ChunkTick");
    MC_ASSERT_RELEASE(pendingTasksCount >= 0); // 转为断言，便于迅速暴露问题
    m_chunkManager->tick(pendingTasksCount);

    // 光照更新 - 限制每 tick 最多处理 32768 个区块，避免过长卡顿
    if (m_lightManager->hasLightWork()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick::LightManager");
        m_lightManager->tick(32768, true, true);
    }

    m_skyLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
```

这可能会导致一些空指针访问，但这通常是由于架构设计缺陷或其他bug引起的，过度防御会掩盖这些问题。正确的做法是通过单元测试、集成测试和代码审查来发现和修复这些问题，而不是在每个调用处添加冗余检查。

我能容忍这些错误发生，但我不能容忍你通过防御性编程来掩盖这些错误。

如果你在编写代码过程中发现现存了一些这样的冗余检查，务必顺手删除它们。

---

## 2. C++语言特性规范

### 2.1 C++标准

强制使用C++20标准。

### 2.2 允许使用的C++20特性

| 特性 | 使用建议 | 示例 |
|------|----------|------|
| `auto` | ✅ 推荐 | `auto iter = vec.begin();` |
| 结构化绑定 | ✅ 推荐 | `auto [x, y, z] = position;` |
| `std::optional` | ✅ 推荐 | `std::optional<Block> getBlock();` |
| `std::variant` | ✅ 推荐 | `std::variant<Block, Entity> hitResult;` |
| `std::any` | ⚠️ 谨慎 | 避免过度使用 |
| `if constexpr` | ✅ 推荐 | 模板元编程 |
| `std::string_view` | ✅ 推荐 | 只读字符串参数 |
| 折叠表达式 | ✅ 推荐 | 模板编程 |
| `std::filesystem` | ✅ 推荐 | 文件操作 |

### 2.3 禁止使用的特性

```cpp
// ❌ 禁止：C风格强制转换
int* ptr = (int*)malloc(sizeof(int));

// ✅ 推荐：C++风格强制转换
int* ptr = static_cast<int*>(malloc(sizeof(int)));

// ❌ 禁止：原始数组
int arr[10];

// ✅ 推荐：std::array或std::vector
std::array<int, 10> arr;
std::vector<int> vec;

// ❌ 禁止：宏定义常量
#define MAX_PLAYERS 100

// ✅ 推荐：constexpr常量
inline constexpr int MAX_PLAYERS = 100;

// ❌ 禁止：异常会导致性能问题，建议使用Result/Expected类型处理错误
// 对于现有存量代码中的异常处理，考虑到兼容性，暂时不强制重构，但新代码中强烈建议不要使用异常。
try {
    // 正常逻辑
} catch (...) {
    // 错误处理
}

// ✅ 推荐：使用Result/Expected类型
Result<void> result = doSomething();
if (!result.success()) {
    // 错误处理
}
```

### 2.4 类设计规范

```cpp
// ✅ 推荐：Rule of Five
class Chunk {
public:
    Chunk();                                    // 默认构造函数
    Chunk(const Chunk& other);                  // 拷贝构造
    Chunk(Chunk&& other) noexcept;              // 移动构造
    Chunk& operator=(const Chunk& other);       // 拷贝赋值
    Chunk& operator=(Chunk&& other) noexcept;   // 移动赋值
    ~Chunk();                                   // 析构函数
    
    // 工厂方法
    static std::unique_ptr<Chunk> create(int32_t x, int32_t z);
    
private:
    // 数据成员
    std::vector<Block> m_blocks;
    ChunkPos m_position;
    // 私有方法（私有方法必须加前缀_以示区分）
    void _loadFromDisk();
};

// ✅ 必须：明确指定访问控制
class Entity {
public:
    // 公共接口
    void update(float deltaTime);
    
protected:
    // 派生类可访问
    virtual void onTick();
    
private:
    // 私有实现
    void _internalUpdate();
    EntityID m_id;
};

// ❌ 禁止：友元滥用
class A {
    friend class B;  // 除非必要，否则避免
};
```

### 2.5 函数设计规范

```cpp
// ✅ 推荐：参数顺序（必要参数在前，可选参数在后）
void spawnEntity(EntityType type, 
                 Vector3 position,
                 std::optional<EntityData> data = std::nullopt);

// ✅ 推荐：使用string_view传递只读字符串
void loadTexture(std::string_view path);

// ✅ 推荐：const引用传递大对象
void processChunk(const Chunk& chunk);

// ✅ 推荐：移动语义
void setTexture(std::unique_ptr<Texture> texture);

// ✅ 推荐：[[nodiscard]]标记必须检查返回值的函数
[[nodiscard]] Result<void> initialize();

// ✅ 推荐：[[nodiscard]]标记可能产生新资源的函数
[[nodiscard]] std::unique_ptr<Entity> createEntity();

// ✅ 推荐：复用BlockPos、ChunkPos等已有工具类表示方块/区块坐标，降低参数复杂度
```

---

## 3. 命名规范

### 3.1 文件命名

| 类型 | 规范 | 示例 |
|------|------|------|
| 头文件 | `PascalCase.hpp` | `ChunkManager.hpp` |
| 源文件 | `PascalCase.cpp` | `ChunkManager.cpp` |
| CMake文件 | `PascalCase.cmake` | `CompilerWarnings.cmake` |
| 测试文件 | `test_*.cpp` | `test_chunk.cpp` |
| 配置脚本 | `kebab-case.sh` | `build-project.sh` |

### 3.2 类型命名

```cpp
// ✅ 类/结构体：PascalCase
class ChunkManager;
struct Vector3;
enum class BlockType;

// ✅ 模板参数：PascalCase
template<typename T, typename Allocator>
class Buffer;

// ✅ 类型别名：PascalCase
using ChunkPos = Vector3;
using EntityList = std::vector<Entity*>;

// ✅ 必须：接口类以I开头
class IEntityManager;
```

### 3.3 变量命名

```cpp
// ✅ 成员变量：m_前缀 + camelCase
class Player {
private:
    float m_health;
    std::string m_name;
    Vector3 m_position;
};

// ✅ 局部变量：camelCase
void update() {
    float health = 100.0f;
    std::string playerName = "Steve";
}

// ✅ 全局变量：g_前缀 + camelCase
inline constexpr int g_maxPlayers = 100;

// ✅ 静态成员变量：s_前缀 + camelCase
class Entity {
private:
    static uint64_t s_nextId;
};

// ✅ 常量：UPPER_SNAKE_CASE
inline constexpr int MAX_CHUNKS = 1024;
inline constexpr float GRAVITY = 9.81f;

// ✅ 枚举值：PascalCase（scoped enum）
enum class BlockType {
    Air,
    Stone,
    Dirt,
    Grass
};
```

### 3.4 函数命名

私有方法用_前缀以示区分。

### 3.5 命名空间规范

需要使用命名空间隔离各个子系统的标识符。下面是最佳实践：

```cpp
namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性修改器操作类型
 *
 * 定义属性修改器如何影响基础值
 */
enum class Operation : u8 {
    // ...
}}}}

// ❌ 禁止：不允许使用 using namespace std！
using namespace std;  // ❌
```

【重要】对于只在单个 .cpp 文件中使用的类、函数、全局变量等，应放入匿名 namespace，避免污染，**并在其命名前加上下划线prefix**，而不是使用static或全局命名空间。

---

## 4. 日志使用规范

1. 不允许使用spdlog::debug和spdlog::trace的日志级别，它们会降低性能。只允许用info以及以上级别的日志。
2. 对于任何偏移正常路径、逻辑等异常情况，必须使用warn以及以上级别的日志记录，不允许使用info级别来掩盖他们。（包括但不限于：fallback、未实现、弃用等）
3. 所有日志内容和错误消息必须使用英文，不允许用中文

## 5. 注释与文档规范

### 5.1 Doxygen文档注释

只在头文件中使用Doxygen风格的文档注释即可。

```cpp
/**
 * @brief 加载区块
 * 
 * 从磁盘或缓存中加载指定位置的区块。如果区块不存在，
 * 则使用世界生成器生成新区块。
 * 
 * @param pos 区块位置
 * @param priority 加载优先级（0-10，10最高）
 * @return Result<std::shared_ptr<Chunk>> 加载结果
 * 
 * @note 此方法是异步的，返回后区块可能尚未完全加载
 * @warning 不要在主线程中调用高优先级加载
 * 
 */
Result<std::shared_ptr<Chunk>> loadChunk(ChunkPos pos, int priority);
```

### 5.2 行内注释

推荐使用较多的简体中文行内注释解释逻辑、算法步骤、设计决策等。

```cpp
// ✅ 推荐：解释"为什么"而不是"是什么"
// 使用二次插值平滑相机移动，避免突变
float alpha = smoothstep(0.0f, 1.0f, deltaTime * 5.0f);

// ✅ 【重要】必须：标记待办事项。所有没做完的工作、妥协、简化实现等都必须明确标记，避免遗忘：
// TODO: 优化区块加载算法，当前O(n²)复杂度
// FIXME: 内存泄漏问题，需要在析构函数中释放
// TODO: 临时解决方案，等待其他模块完成后重构
// NOTE: 此处性能关键，不要随意修改
```

【重要】暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等，必须加上TODO注释（注释中要有明文`TODO`，便于全文搜索），如果对应逻辑缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。

### 5.3 代码区域标记

```cpp
// ============================================================================
// 区块加载系统
// ============================================================================
```

### 5.4 注释必须全部使用简体中文

### 5.5 禁止出现“对齐MC”之类的注释

下面这些“对齐MC”之类的注释不能出现；如果你发现代码中有这些注释，顺手将它们删除：

```cpp
    // 参考 MC 1.16.5 Minecraft.runTick() 中闪电闪烁的处理 ❌这行注释要删了
    m_renderer->setLightningFlashBrightness(m_world.weather().lightningFlashBrightness());

    // 对齐 MC 1.21.11 FirstPersonRenderer::renderArmFirstPerson。 ❌这行注释要删了
    const f32 sqrtSwing = std::sqrt(swingProgress);

    // 参考: net.minecraft.block.LiquidBlock#reactWithNeighbors   ❌这行注释要删了

/**
 * @brief 基于方块坐标的位置目标
 *
 * 对齐 MC 1.16.5 BlockPosWrapper，始终使用方块中心点作为导航/注视位置。 ❌这行注释要删了
 */
class BlockPosTarget final : public IPositionTarget {}

    // 水生生物 - MC 1.16.5 权重 ❌这行注释要删了
    info.setMaxWaterCreatureInstances(DEFAULT_MAX_WATER_CREATURES);
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:cod", 15, 3, 6)); // MC 1.16.5: 15 ❌这行注释要删了
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:salmon", 15, 1, 5)); // MC 1.16.5: 15 ❌这行注释要删了
    info.addWaterCreatureSpawn(SpawnEntry("minecraft:squid", 3, 1, 4)); // MC 1.16.5: 3 ❌这行注释要删了

        // 触发通用的 EffectsChangedTrigger
        // 参考 MC 1.16.5: CriteriaTriggers.EFFECTS_CHANGED.trigger(ServerPlayer) ❌这行注释要删了
```

---

## 6. 内存管理规范

### 6.1 智能指针使用

```cpp
// ✅ 推荐：优先使用std::unique_ptr
std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>();

// ✅ 推荐：共享所有权使用std::shared_ptr
std::shared_ptr<Texture> texture = std::make_shared<Texture>();

// ❌ 严禁：裸new/delete
Type* ptr = new Type();  // ❌ 尽可能不要使用裸指针
delete ptr;              // ❌
```

### 6.2 容器使用

```cpp
// ✅ 推荐：预分配容量
blocks.reserve(16 * 256 * 16);  // 区块大小
// ✅ 推荐：使用emplace_back代替push_back
// ✅ 推荐：使用string_view避免拷贝
void processString(std::string_view str);
```

---

## 7. 错误处理规范

### 7.1 Result类型使用

```cpp
// ✅ 推荐：使用Result类型处理错误
Result<void> initialize() {
    auto result = createWindow();
    if (!result.success()) {
        return result;  // 传播错误
    }
    
    result = createRenderer();
    if (!result.success()) {
        return result;
    }
    
    return Result<void>::ok();
}

Result<void> initialize() {
    TRY(createWindow());
    TRY(createRenderer());
    return Result<void>::ok();
}

// ✅ 推荐：携带错误信息
class Error {
public:
    enum class Code {
        Success = 0,
        NotFound,
        InvalidArgument,
        OutOfMemory,
        // ...
    };
    
    Code code() const;
    const std::string& message() const;
    const std::string& source() const;  // 错误位置
};

// 使用
Result<Chunk*> loadChunk(ChunkPos pos) {
    if (!chunkExists(pos)) {
        return Error{
            Error::Code::NotFound,
            fmt::format("Chunk at {} not found", pos),
            "ChunkManager::loadChunk"
        };
    }
    // ...
}
```

---

## 8. include路径规范

```cpp
// ✅ 推荐：使用./或不使用./
#include "Vector3.hpp"// ✅
#include "./Vector3.hpp"// ✅
#include "./math/Vector3.hpp"// ✅

// ❌ 禁止：使用`../`跳转到上级目录，这会导致路径混乱，增加维护难度
#include "../../common/util/math/Vector3.hpp"// ❌
// 这种情况下建议换成绝对路径：
#include "common/util/math/Vector3.hpp"// ✅
```

---

## 9. 性能相关规范

### 9.2 避免的性能陷阱

```cpp
// ❌ 禁止：不必要的拷贝
std::string getName() { return m_name; }  // ❌

// ✅ 推荐：返回const引用
const std::string& getName() const { return m_name; }  // ✅

// ❌ 禁止：在循环中重复计算
for (int i = 0; i < vec.size(); i++) {  // size()每次调用
    // ...
}

// ✅ 推荐：缓存循环条件
for (int i = 0, n = vec.size(); i < n; i++) {  // ✅
    // ...
}

```

---

## 10. 安全规范

### 10.1 输入验证

```cpp
// ✅ 推荐：验证所有外部输入
Result<void> loadFile(const std::string& path) {
    // 验证路径
    if (path.empty()) {
        return Error::invalidArgument("Path cannot be empty");
    }
    
    // 防止路径遍历
    if (path.find("..") != std::string::npos) {
        return Error::invalidArgument("Invalid path");
    }
    
    // 验证文件扩展名
    if (!path.ends_with(".json")) {
        return Error::invalidArgument("Invalid file type");
    }
    
    // ...
}

// ✅ 推荐：限制输入大小
void processMessage(const std::string& message) {
    constexpr size_t MAX_MESSAGE_LENGTH = 256;
    if (message.length() > MAX_MESSAGE_LENGTH) {
        return Error::invalidArgument("Message too long");
    }
    // ...
}
```

---

当你发现了存量代码中有任何地方违反了上述任意规范，请你顺手直接修改它们，你有权限直接修改任何违反规范的代码。
