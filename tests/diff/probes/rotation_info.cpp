/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @brief 差分能力：`Rotation` 的枚举语义与步进旋转（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.block.Rotation`：
 *
 * ```java
 * NONE               (0, "none",                OctahedralGroup.IDENTITY)
 * CLOCKWISE_90       (1, "clockwise_90",        OctahedralGroup.ROT_90_Y_NEG)
 * CLOCKWISE_180      (2, "180",                 OctahedralGroup.ROT_180_FACE_XZ)
 * COUNTERCLOCKWISE_90(3, "counterclockwise_90", OctahedralGroup.ROT_90_Y_POS)
 *
 * public static final IntFunction<Rotation> BY_ID =
 *     ByIdMap.continuous(Rotation::getIndex, values(), ByIdMap.OutOfBoundsStrategy.WRAP);
 *
 * public Direction rotate(Direction dir) {
 *     if (dir.getAxis() == Axis.Y) {
 *         return dir;                       // 上下方向不受水平旋转影响
 *     }
 *     return switch (this) {
 *         case CLOCKWISE_90        -> dir.getClockWise();
 *         case CLOCKWISE_180       -> dir.getOpposite();
 *         case COUNTERCLOCKWISE_90 -> dir.getCounterClockWise();
 *         default                  -> dir;  // NONE
 *     };
 * }
 *
 * public int rotate(int value, int modulus) {
 *     return switch (this) {
 *         case CLOCKWISE_90        -> (value + modulus / 4) % modulus;
 *         case CLOCKWISE_180       -> (value + modulus / 2) % modulus;
 *         case COUNTERCLOCKWISE_90 -> (value + modulus * 3 / 4) % modulus;
 *         default                  -> value;
 *     };
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`CLOCKWISE_180` 的序列化名是 `"180"`，不是 `"clockwise_180"`。**
 *
 *    | 枚举 | `getSerializedName()` |
 *    |---|---|
 *    | `NONE` | `none` |
 *    | `CLOCKWISE_90` | `clockwise_90` |
 *    | **`CLOCKWISE_180`** | **`180`** |
 *    | `COUNTERCLOCKWISE_90` | `counterclockwise_90` |
 *
 *    这是四个里**唯一不符合「小写下划线」惯例的**。若按枚举名机械转小写得 `clockwise_180`，
 *    与原版不符，会让使用 `Rotation.CODEC` 的方块状态解析失败。
 *
 * 2. **`BY_ID` 用 `WRAP`，与其它能力的 `ZERO` 完全不同。**
 *
 *    ```java
 *    case WRAP -> id -> at[Mth.positiveModulo(id, length)];
 *    ```
 *
 *    - `byId(0..3)` 正常
 *    - **`byId(4)` → `NONE`**（回绕）
 *    - **`byId(-1)` → `COUNTERCLOCKWISE_90`**（`positiveModulo(-1,4) = 3`）
 *    - **`byId(9)` → `CLOCKWISE_90`**（`9 % 4 = 1`）
 *
 *    对比：[`dye_color`](./dye_color.md) / [`equipment_slot`](./equipment_slot.md) /
 *    [`heightmap_types`](./heightmap_types.md) 越界返回第 0 项；
 *    [`direction_plane`](./direction_plane.md) 用 `abs(id % len)`。
 *    **本能力是第四种策略。**
 *
 *    `positiveModulo` 与 `abs(a % b)` 的差别在负数上：`positiveModulo(-1,4)=3` 而
 *    `abs(-1 % 4)=1`。
 *
 * 3. **`rotate(Direction)` 对 Y 轴方向是恒等。**
 *
 *    ```java
 *    if (dir.getAxis() == Axis.Y) return dir;
 *    ```
 *
 *    `rotate(CLOCKWISE_90, UP) == UP`，`rotate(CLOCKWISE_180, DOWN) == DOWN`。
 *    水平旋转**永远不改变上下方向**。
 *
 * 4. **`rotate(int, int)` 是「按模数步进」，不是普通取模。**
 *
 *    ```java
 *    CLOCKWISE_90:        (value + modulus / 4) % modulus
 *    CLOCKWISE_180:       (value + modulus / 2) % modulus
 *    COUNTERCLOCKWISE_90: (value + modulus * 3 / 4) % modulus
 *    ```
 *
 *    **注意 `modulus * 3 / 4` 是先乘后除**（不是 `modulus / 4 * 3`），对不能被 4 整除的模数
 *    会得到不同结果。例如 `modulus = 6`：`6 * 3 / 4 = 4`，而 `6 / 4 * 3 = 3`。
 *
 *    且**全部用 Java 的 `%`（向零截断）**，`value` 为负时结果可能为负——
 *    与 `positiveModulo` 不同。
 *
 * 单元格式：
 * - `info|<旋转>` —— `index,serializedName`
 * - `byId|<id>` —— `BY_ID` 查找
 * - `getRotated|<a>|<b>` —— `a.getRotated(b)`
 * - `rotateDir|<旋转>|<方向>` —— `rotate(Direction)`
 * - `rotateInt|<旋转>|<value>|<modulus>` —— `rotate(int, int)`
 *
 * TODO: 未覆盖 `getRandom` / `getShuffled`（需要随机源）、`OctahedralGroup` 的底层群运算。
 */
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 一个旋转值。
struct RotationInfo {
    const char* name;
    i32 index;
    const char* serializedName;
};

/// 原版 4 个旋转，按 index 升序。
constexpr RotationInfo ROTATIONS[] = {
    {"NONE", 0, "none"},
    {"CLOCKWISE_90", 1, "clockwise_90"},
    {"CLOCKWISE_180", 2, "180"},
    {"COUNTERCLOCKWISE_90", 3, "counterclockwise_90"},
};

constexpr i32 ROTATION_COUNT = 4;

/// 旋转名 → 旋转。
const RotationInfo* findRotation(const std::string& name)
{
    for (const auto& rot : ROTATIONS) {
        if (name == rot.name) {
            return &rot;
        }
    }
    return nullptr;
}

/// 原版 `Mth.positiveModulo`：结果始终非负。
i32 positiveModulo(i32 value, i32 modulus)
{
    const i32 result = value % modulus;
    return result < 0 ? result + modulus : result;
}

/// 原版 `Rotation.BY_ID`（WRAP 策略）。
const RotationInfo* byId(i32 id)
{
    return &ROTATIONS[positiveModulo(id, ROTATION_COUNT)];
}

/// 原版 `Rotation.getRotated`：把另一个旋转叠加到本旋转上。
const RotationInfo* getRotated(const RotationInfo& base, const RotationInfo& other)
{
    return &ROTATIONS[(base.index + other.index) % ROTATION_COUNT];
}

/// 方向名 → 是否 Y 轴。
bool isYAxis(const std::string& dir)
{
    return dir == "UP" || dir == "DOWN";
}

/// 原版 `Direction.getClockWise()`：水平面顺时针。
const char* getClockWise(const std::string& dir)
{
    if (dir == "NORTH") {
        return "EAST";
    }
    if (dir == "EAST") {
        return "SOUTH";
    }
    if (dir == "SOUTH") {
        return "WEST";
    }
    if (dir == "WEST") {
        return "NORTH";
    }
    return nullptr;
}

/// 原版 `Direction.getCounterClockWise()`：水平面逆时针。
const char* getCounterClockWise(const std::string& dir)
{
    if (dir == "NORTH") {
        return "WEST";
    }
    if (dir == "WEST") {
        return "SOUTH";
    }
    if (dir == "SOUTH") {
        return "EAST";
    }
    if (dir == "EAST") {
        return "NORTH";
    }
    return nullptr;
}

/// 原版 `Direction.getOpposite()`。
const char* getOpposite(const std::string& dir)
{
    if (dir == "NORTH") {
        return "SOUTH";
    }
    if (dir == "SOUTH") {
        return "NORTH";
    }
    if (dir == "EAST") {
        return "WEST";
    }
    if (dir == "WEST") {
        return "EAST";
    }
    if (dir == "UP") {
        return "DOWN";
    }
    if (dir == "DOWN") {
        return "UP";
    }
    return nullptr;
}

/// 原版 `Rotation.rotate(Direction)`。
std::string rotateDir(const std::string& rotName, const std::string& dir)
{
    if (isYAxis(dir)) {
        return dir;
    }
    if (rotName == "NONE") {
        return dir;
    }
    if (rotName == "CLOCKWISE_90") {
        const char* result = getClockWise(dir);
        if (result == nullptr) {
            throw std::runtime_error("rotation_info: 未知方向：" + dir);
        }
        return result;
    }
    if (rotName == "CLOCKWISE_180") {
        const char* result = getOpposite(dir);
        if (result == nullptr) {
            throw std::runtime_error("rotation_info: 未知方向：" + dir);
        }
        return result;
    }
    if (rotName == "COUNTERCLOCKWISE_90") {
        const char* result = getCounterClockWise(dir);
        if (result == nullptr) {
            throw std::runtime_error("rotation_info: 未知方向：" + dir);
        }
        return result;
    }
    throw std::runtime_error("rotation_info: 未知旋转：" + rotName);
}

/// 原版 `Rotation.rotate(int, int)`：注意 `modulus * 3 / 4` 是先乘后除。
i32 rotateInt(const std::string& rotName, i32 value, i32 modulus)
{
    if (modulus == 0) {
        throw std::runtime_error("rotation_info: 模数为 0");
    }
    if (rotName == "CLOCKWISE_90") {
        return (value + modulus / 4) % modulus;
    }
    if (rotName == "CLOCKWISE_180") {
        return (value + modulus / 2) % modulus;
    }
    if (rotName == "COUNTERCLOCKWISE_90") {
        return (value + modulus * 3 / 4) % modulus;
    }
    return value;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // info：全部 4 个
        "info|NONE",
        "info|CLOCKWISE_90",
        "info|CLOCKWISE_180",
        "info|COUNTERCLOCKWISE_90",
        // byId：合法与越界（WRAP 策略）
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|3",
        "byId|4",
        "byId|5",
        "byId|9",
        "byId|-1",
        "byId|-2",
        "byId|-4",
        // getRotated：叠加
        "getRotated|NONE|NONE",
        "getRotated|NONE|CLOCKWISE_90",
        "getRotated|CLOCKWISE_90|CLOCKWISE_90",
        "getRotated|CLOCKWISE_90|COUNTERCLOCKWISE_90",
        "getRotated|CLOCKWISE_180|CLOCKWISE_180",
        "getRotated|COUNTERCLOCKWISE_90|CLOCKWISE_90",
        // rotateDir：Y 轴恒等
        "rotateDir|CLOCKWISE_90|UP",
        "rotateDir|CLOCKWISE_90|DOWN",
        "rotateDir|CLOCKWISE_180|UP",
        "rotateDir|COUNTERCLOCKWISE_90|DOWN",
        // rotateDir：水平面
        "rotateDir|NONE|NORTH",
        "rotateDir|CLOCKWISE_90|NORTH",
        "rotateDir|CLOCKWISE_90|EAST",
        "rotateDir|CLOCKWISE_90|SOUTH",
        "rotateDir|CLOCKWISE_90|WEST",
        "rotateDir|CLOCKWISE_180|NORTH",
        "rotateDir|CLOCKWISE_180|EAST",
        "rotateDir|COUNTERCLOCKWISE_90|NORTH",
        "rotateDir|COUNTERCLOCKWISE_90|EAST",
        // rotateInt：模 4（能被 4 整除）
        "rotateInt|CLOCKWISE_90|0|4",
        "rotateInt|CLOCKWISE_90|3|4",
        "rotateInt|CLOCKWISE_180|0|4",
        "rotateInt|CLOCKWISE_180|3|4",
        "rotateInt|COUNTERCLOCKWISE_90|0|4",
        "rotateInt|COUNTERCLOCKWISE_90|1|4",
        "rotateInt|NONE|2|4",
        // rotateInt：模 6（不能整除 4，暴露先乘后除）
        "rotateInt|CLOCKWISE_90|0|6",
        "rotateInt|COUNTERCLOCKWISE_90|0|6",
        "rotateInt|CLOCKWISE_180|0|6",
        // rotateInt：负值（Java % 向零截断）
        "rotateInt|CLOCKWISE_90|-1|4",
        "rotateInt|COUNTERCLOCKWISE_90|-1|6",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("rotation_info: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "info" && parts.size() == 2) {
        const RotationInfo* rot = findRotation(parts[1]);
        if (rot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const std::string value = std::to_string(rot->index) + "," + rot->serializedName;
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "byId" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", std::string(byId(std::stoi(parts[1]))->name)}}.dump();
    }
    if (mode == "getRotated" && parts.size() == 3) {
        const RotationInfo* base = findRotation(parts[1]);
        const RotationInfo* other = findRotation(parts[2]);
        if (base == nullptr || other == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        return json{{"unit", unit}, {"value", std::string(getRotated(*base, *other)->name)}}.dump();
    }
    if (mode == "rotateDir" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", rotateDir(parts[1], parts[2])}}.dump();
    }
    if (mode == "rotateInt" && parts.size() == 4) {
        const i32 value = static_cast<i32>(std::stoll(parts[2]));
        const i32 modulus = static_cast<i32>(std::stoll(parts[3]));
        return json{{"unit", unit}, {"value", rotateInt(parts[1], value, modulus)}}.dump();
    }

    throw std::runtime_error("rotation_info: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("rotation_info", &writeList, &runLine)

} // namespace mc::diff
