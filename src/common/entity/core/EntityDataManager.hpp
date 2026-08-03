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

#pragma once

#include "DataParameter.hpp"
#include "EntityClassRegistry.hpp"
#include "EntityPose.hpp"
#include "common/command/ICommandSource.hpp" // for mc::Uuid (std::array<u8,16>)
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::f32;
using mc::i32;
using mc::i64;
using mc::i8;
using mc::u16;
using mc::Uuid;

using mc::Vector2f;
using mc::Vector3f;
using mc::Vector3i;

/**
 * @brief Pose 字段值包装
 *
 * 对齐 vanilla 1.21.11 Entity.DATA_POSE（Pose serializer，EntityDataSerializers id=20）。
 * wire 上以 VarInt(EntityPose 枚举值=vanilla Pose.id) 传输，与 Byte(serializerId=0，wire 单字节)
 * 不同，故需独立 variant 类型区分。
 */
struct PoseValue {
    EntityPose value{EntityPose::Standing};

    PoseValue() = default;
    explicit PoseValue(EntityPose v) noexcept
        : value(v)
    {}

    bool operator==(const PoseValue& other) const noexcept { return value == other.value; }
    bool operator!=(const PoseValue& other) const noexcept { return value != other.value; }
};

/**
 * @brief Optional<Component> 字段值包装
 *
 * 对齐 vanilla 1.21.11 Entity.DATA_CUSTOM_NAME（OptionalComponent serializer，
 * EntityDataSerializers id=6）。wire = 1 byte present + 若 present 则 NBT Component
 * （纯文本折叠为 StringTag 0x08 + U16 BE 长度 + UTF8，与 writeTextComponentNbt 同源）。
 */
struct OptionalComponentValue {
    bool present{false};
    std::string text; // present 时为组件纯文本（getUnformattedText）

    OptionalComponentValue() = default;
    OptionalComponentValue(bool p, std::string t) noexcept
        : present(p)
        , text(std::move(t))
    {}

    bool operator==(const OptionalComponentValue& other) const noexcept
    {
        return present == other.present && text == other.text;
    }
    bool operator!=(const OptionalComponentValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief Optional<BlockPos> 字段值包装
 *
 * 对齐 vanilla 1.21.11 LivingEntity.SLEEPING_POS_ID（OptionalBlockPos serializer，
 * EntityDataSerializers id=11）。wire = 1 byte present + 若 present 则大端 packed i64
 * （BlockPos.asLong：X26/Z26/Y12，与 BlockPos.STREAM_CODEC 一致）。
 */
struct OptionalBlockPosValue {
    bool present{false};
    Vector3i pos; // present 时为方块位置

    OptionalBlockPosValue() = default;
    OptionalBlockPosValue(bool p, Vector3i p_pos) noexcept
        : present(p)
        , pos(p_pos)
    {}

    bool operator==(const OptionalBlockPosValue& other) const noexcept
    {
        return present == other.present && pos == other.pos;
    }
    bool operator!=(const OptionalBlockPosValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief List<ParticleOptions> 字段值包装
 *
 * 对齐 vanilla 1.21.11 LivingEntity.DATA_EFFECT_PARTICLES（Particles serializer，
 * EntityDataSerializers id=17）。wire = VarInt(count) + 每个粒子的 codec。
 *
 * 本项目当前未实现完整粒子同步：默认持有空列表（count=0），wire 仅写 VarInt(0)。
 * 这已足够让真 Java 客户端通过字段类型校验（客户端按 Particles 反序列化空列表）。
 * 实际药水粒子由客户端依据药水效果本地渲染，无需服务端同步粒子列表。
 */
struct ParticlesValue {
    bool empty{true}; // 仅空列表场景；扩展粒子同步时改为承载粒子列表

    ParticlesValue() = default;
    explicit ParticlesValue(bool e) noexcept
        : empty(e)
    {}

    bool operator==(const ParticlesValue& other) const noexcept { return empty == other.empty; }
    bool operator!=(const ParticlesValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief HumanoidArm 字段值包装
 *
 * 对齐 vanilla 1.21.11 的 HUMANOID_ARM（EntityDataSerializers id=38）。
 * wire = VarInt(HumanoidArm ordinal：LEFT=0/RIGHT=1)。用于 Player.DATA_PLAYER_MAIN_HAND。
 * 与普通 Int(serializerId=1) 区分：vanilla 客户端按字段 serializerId 严格类型校验，
 * 故 MAIN_HAND 必须用 id=38 而非 id=1，否则 set_entity_data 类型校验失败。
 */
struct HumanoidArmValue {
    i32 arm{0}; // 0=LEFT, 1=RIGHT（HumanoidArm.ordinal）

    HumanoidArmValue() = default;
    explicit HumanoidArmValue(i32 a) noexcept
        : arm(a)
    {}

    bool operator==(const HumanoidArmValue& other) const noexcept { return arm == other.arm; }
    bool operator!=(const HumanoidArmValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief OptionalLivingEntityReference 字段值包装
 *
 * 对齐 vanilla 1.21.11 的 OPTIONAL_LIVING_ENTITY_REFERENCE（EntityDataSerializers id=13）。
 * vanilla TamableAnimal.DATA_OWNERUUID_ID 即此类型（1.21.11 从 Optional<UUID> 改为
 * Optional<EntityReference<LivingEntity>>，但 wire 仍是 UUID：EntityReference.streamCodec()
 * = UUIDUtil.STREAM_CODEC）。
 * wire = 1 byte present + 若 present 则 16 字节大端连续 UUID（MSB 8 字节 BE + LSB 8 字节 BE，
 * 对齐 FriendlyByteBuf.readUUID/writeUUID）。注意 NBT 存档格式不同（vanilla 用 int[4]），
 * 此 struct 仅承载 wire 同步语义，NBT 由实体侧自行处理。
 */
struct OptionalUuidValue {
    bool present{false};
    Uuid uuid{}; // present 时为 16 字节 UUID（std::array<u8,16>）

    OptionalUuidValue() = default;
    OptionalUuidValue(bool p, Uuid u) noexcept
        : present(p)
        , uuid(u)
    {}

    bool operator==(const OptionalUuidValue& other) const noexcept
    {
        return present == other.present && uuid == other.uuid;
    }
    bool operator!=(const OptionalUuidValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief Optional<BlockState> 字段值包装
 *
 * 对齐 vanilla 1.21.11 的 OPTIONAL_BLOCK_STATE（EntityDataSerializers id=15）。
 * wire = VarInt(stateId)；0 表示空（无方块状态）。
 * 本项目 BlockState 以原始 stateId(u32) 标识，此处直接承载 stateId，序列化层写 VarInt。
 */
struct OptionalBlockStateValue {
    bool present{false}; // false 时 stateId 无意义（wire 写 0）
    u32 stateId{0};      // present 时为 BlockState 的 stateId

    OptionalBlockStateValue() = default;
    OptionalBlockStateValue(bool p, u32 id) noexcept
        : present(p)
        , stateId(id)
    {}

    bool operator==(const OptionalBlockStateValue& other) const noexcept
    {
        return present == other.present && stateId == other.stateId;
    }
    bool operator!=(const OptionalBlockStateValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief BlockState 字段值包装（非可选）
 *
 * 对齐 vanilla 1.21.11 的 BLOCK_STATE（EntityDataSerializers id=14）。
 * wire = VarInt(stateId)。与 OptionalBlockStateValue 区分：vanilla 此 serializer 永不空。
 */
struct BlockStateValue {
    u32 stateId{0};

    BlockStateValue() = default;
    explicit BlockStateValue(u32 id) noexcept
        : stateId(id)
    {}

    bool operator==(const BlockStateValue& other) const noexcept { return stateId == other.stateId; }
    bool operator!=(const BlockStateValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief Direction 字段值包装
 *
 * 对齐 vanilla 1.21.11 的 DIRECTION（EntityDataSerializers id=12）。
 * wire = VarInt(Direction 3bit id)。项目 mc::Direction 枚举序与 vanilla 一致
 * (Down=0/Up=1/North=2/South=3/West=4/East=5)，故直接以 i32 承载枚举值。
 */
struct DirectionValue {
    i32 direction{0}; // mc::Direction 枚举值

    DirectionValue() = default;
    explicit DirectionValue(i32 d) noexcept
        : direction(d)
    {}

    bool operator==(const DirectionValue& other) const noexcept { return direction == other.direction; }
    bool operator!=(const DirectionValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief OptionalUnsignedInt 字段值包装
 *
 * 对齐 vanilla 1.21.11 的 OPTIONAL_UNSIGNED_INT（EntityDataSerializers id=19）。
 * vanilla OptionalInt wire = 1 byte isPresent + 若 present 则 VarInt(value)。
 * 用于 Player 肩鹦鹉（parrot variant id）等。
 */
struct OptionalUnsignedIntValue {
    bool present{false};
    i32 value{0}; // present 时为无符号值（VarInt 编码）

    OptionalUnsignedIntValue() = default;
    OptionalUnsignedIntValue(bool p, i32 v) noexcept
        : present(p)
        , value(v)
    {}

    bool operator==(const OptionalUnsignedIntValue& other) const noexcept
    {
        return present == other.present && value == other.value;
    }
    bool operator!=(const OptionalUnsignedIntValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief Holder<Variant> 字段值包装
 *
 * 对齐 vanilla 1.21.11 的各类 Holder 变体 serializer（CAT_VARIANT id=21、
 * WOLF_VARIANT id=23、COW_VARIANT、PIG_VARIANT、CHICKEN_VARIANT 等）。
 * 这些 serializer 共用同一 wire 格式：VarInt(registryId)（holder 引用模式，
 * 0 也是合法 id）。本包装以 registryId 承载，serializerId 由具体 DataParameter
 * 的注册侧决定（同一 variant 类型复用本结构，序列化层按 variant index 写 id=21
 * 占位——实际每个实体类的 variant serializer id 在序列化层需区分，当前统一映射
 * 到 HOLDER_VARIANT，详细见 EntityMetadataSerializer 注释）。
 *
 * TODO(复合类型分批): 当前统一映射 Holder variant → HOLDER_VARIANT(serializerId 21)。
 * 真实 vanilla 中 Wolf/Cat/Cow/Pig/Chicken 各有独立 serializer id(21/23/22/26/27)，
 * 待逐实体落地时在序列化层按字段所属实体精确区分 id。本次先用 21 占位保证
 * Wolf(最常见驯服生物)可通过；其余 Holder variant 字段在补齐前以 TODO 标注。
 */
struct HolderVariantValue {
    i32 registryId{0};

    HolderVariantValue() = default;
    explicit HolderVariantValue(i32 id) noexcept
        : registryId(id)
    {}

    bool operator==(const HolderVariantValue& other) const noexcept { return registryId == other.registryId; }
    bool operator!=(const HolderVariantValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief VillagerData 字段值包装
 *
 * 对齐 vanilla 1.21.11 的 VILLAGER_DATA（EntityDataSerializers id=18）。
 * wire = VarInt(type) + VarInt(profession) + VarInt(level)（三段 VarInt，无长度前缀）。
 * type/profession 为 holder registryId（vanilla VillagerType/VillagerProfession），
 * level 为 1..5。experience 不进同步（vanilla record 不含）。
 */
struct VillagerDataValue {
    i32 type{0};       // VillagerType registryId
    i32 profession{0}; // VillagerProfession registryId
    i32 level{1};      // 1..5

    VillagerDataValue() = default;
    VillagerDataValue(i32 t, i32 p, i32 l) noexcept
        : type(t)
        , profession(p)
        , level(l)
    {}

    bool operator==(const VillagerDataValue& other) const noexcept
    {
        return type == other.type && profession == other.profession && level == other.level;
    }
    bool operator!=(const VillagerDataValue& other) const noexcept { return !(*this == other); }
};

/**
 * @brief 数据参数值包装
 *
 * 用于存储任意类型的数据参数值
 */
class DataValue {
public:
    // 支持的数据类型
    // 注：ItemStackView（index 9）承载掉落物等实体的物品本体，经 EntityMetadataSerializer
    // 的 serializerId 7（ITEM_STACK）双向 codec 同步。variant 备选项顺序即 index，
    // EntityMetadataSerializer::getSerializerId/serializeEntry/deserialize 按 index 分支。
    //   0:i8 1:i32 2:i64 3:f32 4:string 5:bool 6:Vector3i 7:Vector2f 8:Vector3f 9:ItemStackView
    //   10:PoseValue(→Pose id20) 11:OptionalComponentValue(→OptionalComponent id6)
    //   12:OptionalBlockPosValue(→OptionalBlockPos id11) 13:ParticlesValue(→Particles id17)
    //   14:OptionalBlockStateValue(→OptionalBlockState id15) 15:BlockStateValue(→BlockState id14)
    //   16:DirectionValue(→Direction id12) 17:OptionalUnsignedIntValue(→OptionalUnsignedInt id19)
    //   18:HolderVariantValue(→Holder variant,本次统一 id21 占位,见 TODO) 19:VillagerDataValue(→VillagerData id18)
    //   20:HumanoidArmValue(→HumanoidArm id38,Player.MAIN_HAND)
    //   21:OptionalUuidValue(→OptionalLivingEntityRef id13,TamableAnimal.DATA_OWNERUUID)
    // 新增类型须同步更新 EntityMetadataSerializer 三处分支。
    using ValueType = std::variant<i8,
        i32,
        i64,
        f32,
        std::string,
        bool,
        Vector3i,
        Vector2f,
        Vector3f,
        network::ir::play::ItemStackView,
        PoseValue,
        OptionalComponentValue,
        OptionalBlockPosValue,
        ParticlesValue,
        OptionalBlockStateValue,
        BlockStateValue,
        DirectionValue,
        OptionalUnsignedIntValue,
        HolderVariantValue,
        VillagerDataValue,
        HumanoidArmValue,
        OptionalUuidValue>;

    DataValue() = default;

    template <typename T>
    explicit DataValue(T value)
        : m_value(value)
    {}

    template <typename T>
    [[nodiscard]] T get() const
    {
        return std::get<T>(m_value);
    }

    template <typename T>
    void set(T value)
    {
        m_value = value;
    }

    [[nodiscard]] const ValueType& value() const noexcept { return m_value; }
    [[nodiscard]] size_t index() const noexcept { return m_value.index(); }

    bool operator==(const DataValue& other) const noexcept { return m_value == other.m_value; }

    bool operator!=(const DataValue& other) const noexcept { return m_value != other.m_value; }

private:
    ValueType m_value;
};

/**
 * @brief 数据条目
 *
 * 存储单个数据参数的值和脏标记
 */
struct DataEntry {
    DataValue value;
    bool dirty = false;
};

/**
 * @brief 实体数据管理器
 *
 * 管理实体的同步数据参数。用于：
 * - 客户端-服务端数据同步
 * - 实体状态管理（生命值、燃烧状态等）
 *
 * 使用方式：
 * @code
 * class MyEntity : public Entity {
 *     static DataParameter<i32> HEALTH;
 *
 *     MyEntity() {
 *         m_dataManager.registerParam(HEALTH, 20);
 *     }
 *
 *     void setHealth(i32 health) {
 *         m_dataManager.set(HEALTH, health);
 *     }
 *
 *     i32 getHealth() const {
 *         return m_dataManager.get<i32>(HEALTH);
 *     }
 * };
 * @endcode
 */
class EntityDataManager {
public:
    /**
     * @brief 创建数据参数键
     * @return 持哨兵 id 的数据参数键
     *
     * 不分配真实 id——返回哨兵 kUnassignedId(0xFFFF)。真实 id 由 registerParam 在
     * 运行时按当前正在注册的实体类继承链分配（对齐 vanilla ClassTreeIdRegistry）。
     * 这规避了跨翻译单元静态初始化顺序未定义的风险：父类 classInfo 是否已就绪在
     * 静态期无法保证，但 registerData 运行时父类 registerData 必先执行，父类 id 已分配。
     *
     * 所有 `DataParameter<X> Foo::PARAM = EntityDataManager::createKey<X>();` 调用点
     * 签名不变。
     */
    template <typename T>
    static DataParameter<T> createKey()
    {
        return DataParameter<T>(kUnassignedId);
    }

    /**
     * @brief RAII 守卫：在 registerData 中标记当前正在注册的实体类
     *
     * 构造时压入类标识栈（t_currentClass 指向栈顶），析构时弹栈。registerData 链中
     * 基类 registerData 先执行（其 guard 压入基类 classInfo，分配基类字段，析构弹出），
     * 子类 registerData 再压入子类 classInfo 分配子类字段。allocateIdForCurrentClass
     * 始终按栈顶类沿父链分配，与 vanilla ClassTreeIdRegistry.define(clazz) 一致。
     */
    class ClassRegisterGuard {
    public:
        ClassRegisterGuard(EntityDataManager& dm, const EntityClassInfo& cls)
            : m_dm(dm)
        {
            m_dm.beginClassRegistration(cls);
        }
        ~ClassRegisterGuard() { m_dm.endClassRegistration(); }

        ClassRegisterGuard(const ClassRegisterGuard&) = delete;
        ClassRegisterGuard& operator=(const ClassRegisterGuard&) = delete;

    private:
        EntityDataManager& m_dm;
    };

    /// 压入类标识栈（由 ClassRegisterGuard 调用）
    void beginClassRegistration(const EntityClassInfo& cls)
    {
        t_classStack.push_back(&cls);
        t_currentClass = &cls;
    }

    /// 弹出类标识栈（由 ClassRegisterGuard 调用）
    void endClassRegistration()
    {
        if (!t_classStack.empty()) {
            t_classStack.pop_back();
        }
        t_currentClass = t_classStack.empty() ? nullptr : t_classStack.back();
    }

    /**
     * @brief 为当前正在注册的实体类分配下一个 synched-data id
     *
     * 对齐 vanilla ClassTreeIdRegistry.define(clazz)：沿 t_currentClass 父类链查找
     * 已分配的最高 id（含父类），+1 续接，写入当前类的 lastAssignedId。
     *
     * - Entity(无父)8 字段 → id 0..7，Entity.lastAssignedId=7
     * - VehicleEntity(parent=Entity) → 首字段 id 8
     * - AbstractBoat(parent=VehicleEntity) → 首字段 id 11
     *
     * 若无当前类（t_currentClass==nullptr，如测试直接 registerParam），退化为全局自增
     * t_fallbackId，保持向后兼容。
     */
    u16 allocateIdForCurrentClass()
    {
        if (t_currentClass != nullptr) {
            i32 highest = -1;
            for (const EntityClassInfo* c = t_currentClass; c != nullptr; c = c->parent) {
                const i32 v = c->lastAssignedId.load(std::memory_order_acquire);
                if (v > highest) {
                    highest = v;
                }
            }
            const i32 newId = highest + 1;
            t_currentClass->lastAssignedId.store(newId, std::memory_order_release);
            return static_cast<u16>(newId);
        }
        // 无类上下文（测试/遗留路径）：全局自增兜底
        return static_cast<u16>(t_fallbackId++);
    }

    EntityDataManager() = default;

    /**
     * @brief 注册数据参数
     * @param param 参数键（持哨兵 id，首次注册时填入真实 id）
     * @param defaultValue 默认值
     *
     * 必须在使用前注册所有参数。id 由 allocateIdForCurrentClass 按当前实体类继承链
     * 分配（对齐 vanilla ClassTreeIdRegistry），并经 param.assignId 写回静态成员。
     *
     * 幂等：DataParameter 静态成员被所有同类实体实例共享。首次构造时 param 持哨兵
     * kUnassignedId，分配真实 id 并写回；后续同类实例构造时 param 已持真实 id，
     * 直接复用，不重复分配（否则会沿父链读到已分配的最高 id 再 +1，错乱编号）。
     */
    template <typename T>
    void registerParam(DataParameter<T>& param, T defaultValue)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (param.id() == kUnassignedId) {
            // 静态 DataParameter 被所有同类实体实例共享，首次分配须跨实例互斥，
            // 否则多线程并发首次构造同类实体时会重复分配 id。双重检查锁：全局
            // s_idAllocationMutex 串行化"检查哨兵→分配→写回"序列。
            std::lock_guard<std::mutex> allocLock(s_idAllocationMutex);
            if (param.id() == kUnassignedId) {
                const u16 id = allocateIdForCurrentClass();
                param.assignId(id);
            }
        }
        m_entries[param.id()] = DataEntry{DataValue(std::move(defaultValue)), false};
    }

    /**
     * @brief 设置参数值
     * @param param 参数键
     * @param value 新值
     *
     * 如果值发生变化，标记为脏数据
     */
    template <typename T>
    void set(DataParameter<T> param, T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(param.id());
        if (it == m_entries.end()) {
            // 参数未注册，创建新条目
            m_entries[param.id()] = DataEntry{DataValue(value), true};
            return;
        }

        DataEntry& entry = it->second;
        if (entry.value.get<T>() != value) {
            entry.value.set(value);
            entry.dirty = true;
        }
    }

    /**
     * @brief 按原始参数ID设置值
     * @param id 参数ID
     * @param value 新值
     * @return 如果值发生变化则返回 true
     */
    bool setRaw(u16 id, const DataValue& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(id);
        if (it == m_entries.end()) {
            m_entries[id] = DataEntry{value, true};
            return true;
        }

        DataEntry& entry = it->second;
        if (entry.value != value) {
            entry.value = value;
            entry.dirty = true;
            return true;
        }

        return false;
    }

    /**
     * @brief 获取参数值
     * @param param 参数键
     * @return 参数值
     */
    template <typename T>
    [[nodiscard]] T get(DataParameter<T> param) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(param.id());
        if (it == m_entries.end()) {
            return T{};
        }
        return it->second.value.template get<T>();
    }

    /**
     * @brief 按原始参数ID获取值
     * @param id 参数ID
     * @return 数据值指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const DataValue* getRaw(u16 id) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(id);
        if (it == m_entries.end()) {
            return nullptr;
        }
        return &it->second.value;
    }

    /**
     * @brief 检查参数是否存在
     * @param id 参数ID
     */
    [[nodiscard]] bool hasParam(u16 id) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.find(id) != m_entries.end();
    }

    /**
     * @brief 获取所有脏数据条目
     * @return 脏数据ID列表
     */
    [[nodiscard]] std::vector<u16> getDirtyParams() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<u16> dirty;
        for (const auto& [id, entry] : m_entries) {
            if (entry.dirty) {
                dirty.push_back(id);
            }
        }
        return dirty;
    }

    /**
     * @brief 清除脏标记
     * @param id 参数ID，如果为 0xFFFF 则清除所有
     */
    void clearDirty(u16 id = 0xFFFF)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (id == 0xFFFF) {
            for (auto& [entryId, entry] : m_entries) {
                entry.dirty = false;
            }
        } else {
            auto it = m_entries.find(id);
            if (it != m_entries.end()) {
                it->second.dirty = false;
            }
        }
    }

    /**
     * @brief 检查是否有脏数据
     */
    [[nodiscard]] bool hasDirtyData() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& [id, entry] : m_entries) {
            if (entry.dirty) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 获取所有数据条目（用于序列化）
     */
    [[nodiscard]] const std::unordered_map<u16, DataEntry>& getAllEntries() const { return m_entries; }

    /**
     * @brief 从其他管理器复制数据
     * @param other 源管理器
     */
    void copyFrom(const EntityDataManager& other)
    {
        // 使用 scoped_lock 避免死锁
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_entries = other.m_entries;
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<u16, DataEntry> m_entries;

    // 哨兵 id：createKey 返回此值，registerParam 填入真实 id
    static constexpr u16 kUnassignedId = 0xFFFF;

    // 静态 DataParameter 首次分配 id 的跨实例互斥量（双重检查锁外层锁）。
    // 注意：这是进程级共享的，因为 DataParameter 是实体类的静态成员，
    // 跨所有 EntityDataManager 实例共享同一个 param.id() 写入。
    static inline std::mutex s_idAllocationMutex;

    // 类标识栈（thread_local）：registerData 链中 ClassRegisterGuard 压入/弹出当前类
    static inline thread_local std::vector<const EntityClassInfo*> t_classStack;
    static inline thread_local const EntityClassInfo* t_currentClass = nullptr;
    // 无类上下文时的全局自增兜底（测试/遗留路径）
    static inline thread_local u32 t_fallbackId = 0;
};

} // namespace mc::entity
