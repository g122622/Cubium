# Core Module

The `common/core` module provides foundational types, error handling, constants, and settings infrastructure for the Minecraft Reborn project. It is the most fundamental module in the codebase, with no dependencies on other internal modules.

## Directory Structure

```
src/common/core/
├── Types.hpp                    # Primitive types, game types, vector types
├── Result.hpp                   # Error handling (Result<T>, Error, ErrorCode)
├── Result.cpp                   # Result implementation
├── Constants.hpp                # Math, game, network, world, entity constants
├── EnumSet.hpp                  # Enum set utility (bitset-based)
├── BlockRaycastResult.hpp       # Block raycast result type
├── GameDirectory.hpp/cpp        # 游戏目录管理器（统一管理所有游戏路径）
├── DefaultValues.hpp            # 集中默认值定义
└── settings/
    ├── SettingsBase.hpp         # Settings base class
    ├── SettingsBase.cpp         # Settings implementation
    ├── SettingsTypes.hpp        # Option types (Boolean, Range, Float, Enum, std::string)
    └── ResourcePackListOption.hpp  # Resource pack list option
```

## Files Overview

### Types.hpp

Core type definitions for the entire project.

**Primitive Types:**
```cpp
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
```

**Simple Vector Types (for command system):**
```cpp
struct Vector3i { i32 x, y, z; };
struct Vector3d { f64 x, y, z; };
struct Vector3f { f32 x, y, z; };
struct Vector2f { f32 x, y; };
```

**Game-Specific Types:**
```cpp
using ChunkCoord   = i32;   // Chunk coordinate
using BlockCoord   = i32;   // Block coordinate (within chunk)
using WorldHeight  = i32;   // World height
using EntityId     = u64;   // Entity identifier
using ItemId       = u16;   // Item identifier
using BiomeId      = u16;   // Biome identifier
using DimensionId  = i32;   // Dimension identifier
using PlayerId     = u64;   // Player identifier
```

**Enum Types:**
```cpp
enum class Dimension : DimensionId { Overworld = 0, Nether = 1, TheEnd = 2 };
enum class GameMode : u8 { Survival = 0, Creative = 1, Adventure = 2, Spectator = 3 };
enum class Difficulty : u8 { Peaceful = 0, Easy = 1, Normal = 2, Hard = 3 };
enum class BlockFace : u8 { Bottom, Top, North, South, West, East };
enum class BlockShape : u8 { Empty, Full, Partial, Custom };
```

---

### Result.hpp

Comprehensive error handling system using `Result<T>` type.

**ErrorCode Enum (50+ codes in categories):**
```cpp
enum class ErrorCode : i32 {
    Success = 0,
    // General errors
    Unknown = -1, InvalidArgument = -2, NullPointer = -3, ...
    // Resource errors
    NotFound = -100, AlreadyExists = -101, ...
    // File errors
    FileNotFound = -200, FileOpenFailed = -201, ...
    // Network errors
    ConnectionFailed = -300, ConnectionClosed = -301, ...
    // Game errors
    InvalidBlock = -400, InvalidItem = -401, ...
    // Render errors
    InitializationFailed = -600, ...
    // Resource pack errors
    ResourcePackNotFound = -700, ...
};
```

**Error Class:**
```cpp
class Error {
public:
    Error(ErrorCode code, std::string_view message = "", std::string_view source = "");
    [[nodiscard]] ErrorCode code() const noexcept;
    [[nodiscard]] const std::string& message() const noexcept;
    [[nodiscard]] bool success() const noexcept;
    [[nodiscard]] std::string toString() const;

    // Static factory methods
    static Error ok();
    static Error notFound(std::string_view message = "");
    static Error invalidArgument(std::string_view message = "");
};
```

**Result<T> Template:**
```cpp
// Usage example
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Error(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;  // Implicit conversion
}

auto result = divide(10, 2);
if (result.success()) {
    int value = result.value();
} else {
    // Handle error
}

// valueOr for default value
int value = result.valueOr(0);

// map for transformation
Result<string> strResult = result.map<int>([](int v) { return std::to_string(v); });
```

**TRY Macros:**
```cpp
// MC_TRY - early return on error
MC_TRY(someOperation());  // Returns error if operation fails

// MC_TRY_ASSIGN - assign value or return error
MC_TRY_ASSIGN(value, someOperation());
```

---

### Constants.hpp

Game constants organized by namespace.

**Game Constants:**
```cpp
namespace game {
    constexpr f32 PLAYER_HEIGHT = 1.8f;       // 玩家站立高度
    constexpr f32 PLAYER_EYE_HEIGHT = 1.62f;  // 玩家眼睛高度
    constexpr f32 PLAYER_WIDTH = 0.6f;        // 玩家宽度
    constexpr f32 PLAYER_SNEAK_HEIGHT = 1.5f; // 玩家潜行高度
    constexpr f32 PLAYER_SWIM_HEIGHT = 0.6f;  // 玩家游泳/鞘翅高度
    constexpr f32 PLAYER_SLEEP_HEIGHT = 0.2f; // 玩家睡觉/死亡高度
    constexpr f32 PLAYER_MAX_HEALTH = 20.0f;
    constexpr f32 PLAYER_MAX_AIR = 300.0f;
    constexpr i32 MAX_LIGHT_LEVEL = 15;
    constexpr i32 DAY_LENGTH_TICKS = 24000;
}
```

**Network Constants:**
```cpp
namespace network {
    constexpr i32 PROTOCOL_VERSION = 754;   // MC 1.16.5
    constexpr u16 DEFAULT_PORT = 25565;      // Java版默认端口
    constexpr u16 DEFAULT_RCON_PORT = 25575;
    constexpr Size MAX_PACKET_SIZE = 2097152;  // 2MB
    constexpr u32 KEEP_ALIVE_INTERVAL_MS = 15000;
}
```

**World Constants:**
```cpp
namespace world {
    constexpr i32 CHUNK_WIDTH = 16;
    constexpr i32 CHUNK_HEIGHT = 256;
    constexpr i32 CHUNK_SECTION_HEIGHT = 16;
    constexpr i32 SEA_LEVEL = 63;  // MC 1.16.5 海平面高度
    constexpr i32 CHUNK_LOAD_RADIUS = 10;
}
```

**Entity Constants:**
```cpp
namespace entity {
    constexpr Size MAX_ENTITIES = 65536;
    constexpr Size MAX_PLAYERS = 256;
    constexpr i32 DEFAULT_ENTITY_TRACKING_RANGE_CHUNKS = 8;   // 默认实体追踪距离（区块）
    constexpr i32 PLAYER_TRACKING_RANGE_CHUNKS = 32;          // 玩家追踪距离（区块）
}
```

---

### EnumSet.hpp

Efficient bitset-based enum set implementation.

```cpp
template<typename E>
class EnumSet {
public:
    // Requires enum with Count value
    static_assert(std::is_enum_v<E>);

    EnumSet();                          // Empty set
    EnumSet(std::initializer_list<E>);  // From initializer

    EnumSet& set(E value);              // Add value
    EnumSet& reset(E value);            // Remove value
    EnumSet& flip(E value);             // Toggle value

    bool test(E value) const;           // Check if contains
    bool operator[](E value) const;     // Same as test()

    EnumSet& clear();                   // Remove all
    EnumSet& setAll();                  // Add all

    bool empty() const;                 // No values
    bool any() const;                   // At least one value
    bool all() const;                   // All values
    size_t count() const;               // Number of values

    // Set operations
    EnumSet operator|(const EnumSet&) const;  // Union
    EnumSet operator&(const EnumSet&) const;  // Intersection
    EnumSet operator-(const EnumSet&) const;  // Difference
    EnumSet operator^(const EnumSet&) const;  // XOR
    EnumSet operator~() const;                 // Complement

    bool intersects(const EnumSet&) const;     // Has any common value
    bool contains(const EnumSet&) const;       // Contains all values

    template<typename Func>
    void forEach(Func&& func) const;           // Iterate values
};

// Requires enum with Count value:
enum class MyEnum { A, B, C, Count };
EnumSet<MyEnum> set{MyEnum::A, MyEnum::B};
```

---

### BlockRaycastResult.hpp

Result of raycasting against blocks.

```cpp
enum class RaycastType : u8 { Miss, Block };

class BlockRaycastResult {
public:
    // Factory methods
    static BlockRaycastResult miss();
    static BlockRaycastResult hit(const Vector3& hitPos, const BlockPos& blockPos,
                                   Direction face, f32 distance);

    // Query methods
    RaycastType type() const;
    bool isMiss() const;
    bool isHit() const;

    // Access (only valid when isHit() is true)
    const Vector3& hitPosition() const;
    const BlockPos& blockPos() const;
    Direction face() const;
    f32 distance() const;

    // Get adjacent block position (for block placement)
    BlockPos adjacentPos() const;
};
```

---

### Settings System (settings/)

Settings management with JSON persistence and change notifications.

**SettingsBase.hpp:**
```cpp
class SettingsBase {
public:
    Result<void> load(const std::filesystem::path& path);
    Result<void> save(const std::filesystem::path& path) const;

    void loadFromJson(const nlohmann::json& j);
    void saveToJson(nlohmann::json& j) const;

    void registerOption(const std::string& group, IOption* option);
    void resetToDefaults();
    void resetGroupToDefaults(const std::string& group);

    void enableAutoSave(std::filesystem::path path);
    void disableAutoSave();

    static std::filesystem::path getSettingsPath(const std::string& appName);
    static std::filesystem::path ensureSettingsDir(const std::string& appName);

protected:
    void onSettingChanged();
};
```

**SettingsTypes.hpp:**
```cpp
// Interface
class IOption {
    virtual std::string getKey() const = 0;
    virtual SettingsValue getValue() const = 0;
    virtual bool setValue(const SettingsValue& value) = 0;
    virtual void serialize(nlohmann::json& j) const = 0;
    virtual void deserialize(const nlohmann::json& j) = 0;
    virtual void reset() = 0;
    virtual bool isDefault() const = 0;
};

// Boolean option
class BooleanOption : public IOption {
    BooleanOption(std::string key, bool defaultValue = false);
    bool get() const;
    void set(bool value);
    operator bool() const;
    void onChange(SettingsCallback<bool> callback);
};

// Integer range option (with clamping)
class RangeOption : public IOption {
    RangeOption(std::string key, i32 min, i32 max, i32 defaultValue);
    i32 get() const;
    void set(i32 value);  // Auto-clamps to [min, max]
    operator i32() const;
    i32 min() const;
    i32 max() const;
    void onChange(SettingsCallback<i32> callback);
};

// Float option (with clamping)
class FloatOption : public IOption {
    FloatOption(std::string key, f32 min, f32 max, f32 defaultValue);
    f32 get() const;
    void set(f32 value);  // Auto-clamps to [min, max]
    operator f32() const;
    void onChange(SettingsCallback<f32> callback);
};

// Enum option (with name mapping)
template<typename T>
class EnumOption : public IOption {
    EnumOption(std::string key, std::vector<T> values, T defaultValue, std::vector<std::string> names);
    T get() const;
    void set(T value);
    bool setByName(const std::string& name);
    std::string getName() const;
    operator T() const;
    void onChange(SettingsCallback<T> callback);
};

// std::string option
class StringOption : public IOption {
    StringOption(std::string key, std::string defaultValue = "");
    const std::string& get() const;
    void set(std::string value);
    operator std::string_view() const;
    void onChange(SettingsCallback<std::string> callback);
};
```

**ResourcePackListOption.hpp:**
```cpp
struct ResourcePackEntry {
    std::string path;
    bool enabled = true;
    i32 priority = 0;
};

class ResourcePackListOption : public IOption {
    void add(const ResourcePackEntry& entry);
    bool remove(const std::string& path);
    const ResourcePackEntry* find(const std::string& path) const;
    bool setEnabled(const std::string& path, bool enabled);
    bool setPriority(const std::string& path, i32 priority);

    std::vector<ResourcePackEntry> getSortedEntries() const;
    std::vector<ResourcePackEntry> getEnabledEntries() const;
    std::vector<ResourcePackEntry> getSortedEnabledEntries() const;

    void onChange(std::function<void(const std::vector<ResourcePackEntry>&)> callback);
};
```

**Usage Example:**
```cpp
class MySettings : public SettingsBase {
public:
    MySettings() {
        registerOption("video", &fullscreen);
        registerOption("video", &renderDistance);
        registerOption("audio", &volume);
    }

    BooleanOption fullscreen{"fullscreen", false};
    RangeOption renderDistance{"renderDistance", 2, 32, 12};
    FloatOption volume{"volume", 0.0f, 1.0f, 0.8f};
};

MySettings settings;
settings.load("settings.json");
settings.fullscreen.set(true);  // Auto-saves if enabled
settings.fullscreen.onChange([](bool v) { spdlog::info("Fullscreen: {}", v); });
```

---

## Module Overview

### Overall Responsibility

The core module provides:
1. **Type Definitions** - Primitive types, game-specific types, and simple structures
2. **Error Handling** - Result<T> based error handling without exceptions
3. **Constants** - Game, network, world, and entity constants
4. **Utilities** - EnumSet for efficient enum set operations
5. **Settings Infrastructure** - Persistent settings with JSON serialization

### Inputs and Outputs

**Inputs:**
- None (fundamental module)

**Outputs:**
- Type definitions used throughout the codebase
- Error handling utilities
- Game constants
- Settings management

### Dependencies

**External Dependencies:**
- `<cstdint>` - Integer types
- `<string>`, `<string_view>` - std::string types
- `<optional>` - Optional type
- `<variant>` - For SettingsValue
- `<functional>` - Callbacks
- `<filesystem>` - Settings file paths
- `<nlohmann/json.hpp>` - JSON serialization for settings
- `<spdlog/spdlog.h>` - Logging in SettingsBase.cpp

**Internal Dependencies:**
- `common/util/math/Vector3.hpp` - Used by BlockRaycastResult
- `common/util/Direction.hpp` - Used by BlockRaycastResult
- `common/world/chunk/ChunkPos.hpp` - Used by BlockRaycastResult

### Usage

Include specific headers as needed:

```cpp
// Basic types and game constants
#include "common/core/Types.hpp"

// Error handling
#include "common/core/Result.hpp"

// All constants
#include "common/core/Constants.hpp"

// Enum set utility
#include "common/core/EnumSet.hpp"

// Block raycast
#include "common/core/BlockRaycastResult.hpp"

// Settings system
#include "common/core/settings/SettingsBase.hpp"
#include "common/core/settings/SettingsTypes.hpp"
```

### Common Pitfalls

1. **Result<T> Value Access**
   - Always check `result.success()` before calling `result.value()`
   - `value()` throws if the result contains an error
   - Use `valueOr(default)` for safe access with default value

2. **EnumSet Requirements**
   - Enum type MUST have a `Count` value as the last element
   - The `Count` value defines the bitset size
   ```cpp
   // CORRECT
   enum class MyEnum { A, B, C, Count };
   EnumSet<MyEnum> set;  // Size = Count = 3

   // WRONG - will cause issues
   enum class MyEnum { A, B, C };  // No Count
   ```

3. **Settings Registration**
   - Options must be registered in the constructor BEFORE loading
   - Option pointers must remain valid for the lifetime of SettingsBase
   - Use member variables, not local variables

4. **FloatOption Precision**
   - Float comparison uses epsilon (0.0001f)
   - `isDefault()` may return true for values very close to default

5. **RangeOption Clamping**
   - Values are automatically clamped to [min, max]
   - No error is raised for out-of-range values

6. **BlockRaycastResult Validity**
   - `hitPosition()`, `blockPos()`, `face()`, `distance()` are only valid when `isHit()` is true
   - `adjacentPos()` returns the position for placing a block adjacent to the hit face

### Test Coverage

Tests are located in `tests/common/test_core.cpp` and `tests/common/SettingsTest.cpp`:

**Types.hpp Tests:**
- `Types.IntegerSizes` - Verify integer type sizes
- `Types.FloatSizes` - Verify float type sizes
- `Types.TypeAliases` - Verify type aliases
- `Types.EnumTypes` - Test enum values
- `Types.BlockFaceValues` - Test BlockFace enum

**Result.hpp Tests:**
- `Result.SuccessWithValue` - Basic success case
- `Result.FailureWithError` - Basic error case
- `Result.ValueOrDefault` - Default value handling
- `Result.VoidResult` - Result<void> specialization
- `Result.ChainingOperations` - Error propagation
- `Result.StringResult` - std::string result type
- `Result.VectorResult` - Vector result type
- `Result.MoveSemantics` - Move semantics for unique_ptr

**Error Tests:**
- `ErrorTest.Construction` - Error construction
- `ErrorTest.Comparison` - Error code comparison
- `ErrorTest.ErrorCodeValues` - Error code value verification

**Constants Tests:**
- `Constants.MathConstants` - Math constant values
- `Constants.GameConstants` - Game constant values
- `Constants.WorldConstants` - World constant values
- `Constants.NetworkConstants` - Network constant values

**Settings Tests:**
- All option types (Boolean, Range, Float, Enum, std::string)
- Serialization/deserialization
- Callback invocation
- Default value handling
- SettingsBase save/load
- Auto-save functionality
- ResourcePackListOption operations

---

## Architecture Notes

### Design Decisions

1. **Primitive Type Aliases**
   - Uses `i8`, `i16`, etc. instead of `int8_t` for clarity and brevity
   - Ensures consistent type sizes across platforms

2. **Result<T> vs Exceptions**
   - Chosen for explicit error handling without exceptions
   - Forces callers to handle errors explicitly
   - Zero overhead in success case (variant storage)

3. **Settings System Design**
   - Inheritance-based with `SettingsBase` as the base class
   - Options register themselves with their parent settings
   - Change callbacks enable reactive UI updates
   - JSON serialization for human-readable config files

4. **EnumSet Implementation**
   - Uses `std::bitset` for efficient storage
   - Requires `Count` enum value for size
   - Provides set operations (union, intersection, etc.)

### Extension Points

1. **Adding New Error Codes**
   - Add to `ErrorCode` enum in `Result.hpp`
   - Add corresponding factory method to `Error` class if commonly used

2. **Adding New Constants**
   - Add to appropriate namespace in `Constants.hpp`
   - Create new namespace if a new category is needed

3. **Adding New Option Types**
   - Inherit from `IOption` interface
   - Implement all required methods
   - Add to `SettingsTypes.hpp` or create new file

4. **Adding New Game Types**
   - Add to `Types.hpp`
   - Consider adding to `constants` namespace for related values
