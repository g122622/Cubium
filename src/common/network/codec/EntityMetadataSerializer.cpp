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

#include "common/network/codec/EntityMetadataSerializer.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/item/component/DataComponentPatchWire.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace mc::network {

// ============================================================================
// 1.21.11 数据序列化器 ID（对齐 net.minecraft.network.syncher.EntityDataSerializers
// 静态初始化块的注册顺序——注册顺序即整数 ID，wire 上以 VarInt 传输）
// ============================================================================
namespace {
// 字符串长度上限（2^21 - 1，VarInt 3 字节能表示的最大值）。
// MC 协议字符串长度限制为 32767（Short.MAX_VALUE），但命令树等数据可能超过此限制；
// 使用 VarInt 编码可支持更大的字符串。本模块自用内联常量，与 PacketSerializer 的
// MAX_STRING_LENGTH 同值但独立定义，避免跨模块借常量。
constexpr u32 kMaxStringLength = 2097151u;

// MC 1.21.11 EntityDataSerializers 注册顺序（EntityDataSerializers.java:153-193）
enum class MetadataSerializerId : i32 {
    Byte = 0,              // i8              (ByteBufCodecs.BYTE)
    Int = 1,               // i32             (VAR_INT)
    Long = 2,              // i64             (VAR_LONG)
    Float = 3,             // f32             (FLOAT)
    String = 4,            // std::string     (STRING_UTF8)
    Component = 5,         // 文本组件（本项目暂用 String 承载）
    OptionalComponent = 6, // 可选文本组件
    ItemStack = 7,         // 物品堆
    Boolean = 8,           // bool            (BOOL)
    Rotations = 9,         // Vector3f / Vector2f (ROTATIONS，f32×3)
    BlockPos = 10,         // Vector3i        (BlockPos.STREAM_CODEC = i64 packed)
    OptionalBlockPos = 11, // 可选 BlockPos
    Direction = 12,        // 方向
    OptionalLivingEntityRef = 13,
    BlockState = 14,
    OptionalBlockState = 15,
    Particle = 16,
    Particles = 17,
    VillagerData = 18,
    OptionalUnsignedInt = 19,   // OptionalInt
    Pose = 20,                  // 姿态（本项目以 i8 承载枚举值，走 Byte）
    CatVariant = 21,            // Holder<CatVariant>（VarInt registryId）
    CowVariant = 22,            // Holder<CowVariant>
    WolfVariant = 23,           // Holder<WolfVariant>
    WolfSoundVariant = 24,      // Holder<WolfSoundVariant>
    FrogVariant = 25,           // Holder<FrogVariant>
    PigVariant = 26,            // Holder<PigVariant>
    ChickenVariant = 27,        // Holder<ChickenVariant>
    ZombieNautilusVariant = 28, // Holder<ZombieNautilusVariant>
    // 29 OptionalGlobalPos / 30 PaintingVariant / 31 SnifferState / 32 ArmadilloState /
    // 33 CopperGolemState / 34 WeatheringCopperState / 35 Vector3 / 36 Quaternion /
    // 37 ResolvableProfile
    HumanoidArm = 38, // Player.MAIN_HAND（VarInt ordinal：LEFT=0/RIGHT=1）
};

// DataValue 变体索引 → 1.21.11 序列化器 ID
// 变体顺序（EntityDataManager::DataValue::ValueType）：
//   0:i8 1:i32 2:i64 3:f32 4:string 5:bool 6:Vector3i 7:Vector2f 8:Vector3f 9:ItemStackView
//   10:PoseValue 11:OptionalComponentValue 12:OptionalBlockPosValue 13:ParticlesValue
//   14:OptionalBlockStateValue 15:BlockStateValue 16:DirectionValue 17:OptionalUnsignedIntValue
//   18:HolderVariantValue 19:VillagerDataValue
MetadataSerializerId getSerializerId(const entity::DataValue& value)
{
    switch (value.index()) {
        case 0:
            return MetadataSerializerId::Byte;
        case 1:
            return MetadataSerializerId::Int;
        case 2:
            return MetadataSerializerId::Long;
        case 3:
            return MetadataSerializerId::Float;
        case 4:
            return MetadataSerializerId::String;
        case 5:
            return MetadataSerializerId::Boolean;
        case 6:
            return MetadataSerializerId::BlockPos;
        case 7: // Vector2f → Rotations（z 补 0）
        case 8:
            return MetadataSerializerId::Rotations; // Vector3f → Rotations
        case 9:
            return MetadataSerializerId::ItemStack; // ItemStackView → ITEM_STACK（serializerId 7）
        case 10:
            return MetadataSerializerId::Pose; // PoseValue → POSE（serializerId 20）
        case 11:
            return MetadataSerializerId::OptionalComponent; // OptionalComponentValue（serializerId 6）
        case 12:
            return MetadataSerializerId::OptionalBlockPos; // OptionalBlockPosValue（serializerId 11）
        case 13:
            return MetadataSerializerId::Particles; // ParticlesValue（serializerId 17）
        case 14:
            return MetadataSerializerId::OptionalBlockState; // OptionalBlockStateValue（serializerId 15）
        case 15:
            return MetadataSerializerId::BlockState; // BlockStateValue（serializerId 14）
        case 16:
            return MetadataSerializerId::Direction; // DirectionValue（serializerId 12）
        case 17:
            return MetadataSerializerId::OptionalUnsignedInt; // OptionalUnsignedIntValue（serializerId 19）
        case 18:
            // HolderVariantValue：本次统一映射到 CAT_VARIANT(serializerId 21)。
            // TODO(复合类型分批): Wolf/Cat/Cow/Pig/Chicken variant 各有独立 serializer id
            // (21/23/22/26/27)，逐实体落地时按字段所属实体精确区分。
            return MetadataSerializerId::CatVariant;
        case 19:
            return MetadataSerializerId::VillagerData; // VillagerDataValue（serializerId 18）
        case 20:
            return MetadataSerializerId::HumanoidArm; // HumanoidArmValue（serializerId 38，Player.MAIN_HAND）
        case 21:
            // OptionalUuidValue → OPTIONAL_LIVING_ENTITY_REFERENCE（serializerId 13）。
            // vanilla TamableAnimal.DATA_OWNERUUID_ID（1.21.11 改为 Optional<EntityReference>，wire 仍是 UUID）。
            return MetadataSerializerId::OptionalLivingEntityRef;
        default:
            return MetadataSerializerId::Byte;
    }
}
} // namespace

// ============================================================================
// 序列化
// ============================================================================

std::vector<u8> EntityMetadataSerializer::serialize(const entity::EntityDataManager& manager, bool dirtyOnly)
{
    std::vector<u8> output;

    const auto& entries = manager.getAllEntries();

    for (const auto& [id, entry] : entries) {
        // 如果只要脏数据，跳过非脏数据
        if (dirtyOnly && !entry.dirty) {
            continue;
        }

        serializeEntry(id, entry.value, output);
    }

    // 结束标记
    output.push_back(0xFF);

    return output;
}

void EntityMetadataSerializer::serializeEntry(u16 id, const entity::DataValue& value, std::vector<u8>& output)
{
    // 1.21.11 wire：byte(index) + VarInt(serializerId) + value
    // （对齐 SynchedEntityData.DataValue.write：writeByte(id) + writeVarInt(serializerId) + codec.encode）
    const auto sid = getSerializerId(value);
    // 【临时诊断】BlockState/OptionalBlockState 序列化器本不应被任何服务端实体使用
    // （Enderman carry-state 用 i32 stateId 走 Int serializer）。真客户端崩溃
    // "field 8 ... new=...twisting_vines(class BlockState)" 表明某实体的某字段被当成
    // BlockState 序列化。此处记录字段 index + variant index + serializerId 以定位根因。
    if (sid == MetadataSerializerId::BlockState || sid == MetadataSerializerId::OptionalBlockState) {
        // spdlog::warn("[MetaDiag] BlockState-family entry: fieldIndex={} variantIndex={} serializerId={}",
        //     id,
        //     value.index(),
        //     static_cast<i32>(sid));
    }
    output.push_back(static_cast<u8>(id & 0xFF));
    _writeVarInt(static_cast<i32>(sid), output);

    // 数据（按 1.21.11 序列化器 codec 编码）
    switch (value.index()) {
        case 0: { // i8 → BYTE
            output.push_back(static_cast<u8>(value.get<i8>()));
            break;
        }
        case 1: { // i32 → INT (VAR_INT)
            _writeVarInt(value.get<i32>(), output);
            break;
        }
        case 2: { // i64 → LONG (VAR_LONG)
            _writeVarLong(value.get<i64>(), output);
            break;
        }
        case 3: { // f32 → FLOAT（大端，Java friendly buf）
            f32 val = value.get<f32>();
            _writeBigEndianF32(val, output);
            break;
        }
        case 4: { // std::string → STRING (VarInt len + UTF-8)
            _writeString(value.get<std::string>(), output);
            break;
        }
        case 5: { // bool → BOOLEAN
            output.push_back(value.get<bool>() ? 1 : 0);
            break;
        }
        case 6: { // Vector3i → BLOCK_POS（i64 packed，X26/Z26/Y12，对齐 BlockPos.asLong）
            auto pos = value.get<Vector3i>();
            const i64 packed = BlockPos(pos.x, pos.y, pos.z).asLong();
            _writeBigEndianI64(packed, output);
            break;
        }
        case 7: { // Vector2f → ROTATIONS（f32×3，z 补 0）
            auto rot = value.get<Vector2f>();
            _writeBigEndianF32(rot.x, output);
            _writeBigEndianF32(rot.y, output);
            _writeBigEndianF32(0.0f, output);
            break;
        }
        case 8: { // Vector3f → ROTATIONS（f32×3）
            auto rot = value.get<Vector3f>();
            _writeBigEndianF32(rot.x, output);
            _writeBigEndianF32(rot.y, output);
            _writeBigEndianF32(rot.z, output);
            break;
        }
        case 9: { // ItemStackView → ITEM_STACK（1.21.11 OPTIONAL_STREAM_CODEC：
            // count<=0 即空（仅 VarInt(0)）；否则 VarInt(count) + VarInt(itemId) + DataComponentPatch 字节。
            // patch 之前无外层长度前缀，patch 以自身 VarInt(addedCount)+VarInt(removedCount) 自终止
            // （对齐 vanilla ItemStack.OPTIONAL_STREAM_CODEC + DataComponentPatch.STREAM_CODEC）。
            // 与 JavaPlayCodecs::writeItemStack 同源。）
            auto view = value.get<network::ir::play::ItemStackView>();
            if (view.count <= 0 || view.itemId == 0) {
                _writeVarInt(0, output);
            } else {
                _writeVarInt(view.count, output);
                _writeVarInt(static_cast<i32>(view.itemId), output);
                output.insert(output.end(), view.componentsPatch.begin(), view.componentsPatch.end());
            }
            break;
        }
        case 10: { // PoseValue → POSE（VarInt(Pose.id)，枚举值即 vanilla Pose.id）
            const auto pose = value.get<entity::PoseValue>();
            _writeVarInt(static_cast<i32>(pose.value), output);
            break;
        }
        case 11: { // OptionalComponentValue → OPTIONAL_COMPONENT
            // 1 byte present + 若 present 则 NBT Component：纯文本折叠 StringTag 0x08 + U16 BE 长度 + UTF8
            // （对齐 vanilla OPTIONAL_COMPONENT_STREAM_CODEC + ComponentSerialization，本项目纯文本走
            // writeTextComponentNbt 的 StringTag 折叠形式）。
            const auto comp = value.get<entity::OptionalComponentValue>();
            output.push_back(comp.present ? 1 : 0);
            if (comp.present) {
                output.push_back(0x08); // StringTag id
                _writeBigEndianU16(static_cast<u16>(comp.text.size()), output);
                output.insert(output.end(), comp.text.begin(), comp.text.end());
            }
            break;
        }
        case 12: { // OptionalBlockPosValue → OPTIONAL_BLOCK_POS
            // 1 byte present + 若 present 则大端 packed i64（BlockPos.asLong：X26/Z26/Y12）
            // （对齐 vanilla BlockPos.STREAM_CODEC.apply(ByteBufCodecs::optional)）。
            const auto obp = value.get<entity::OptionalBlockPosValue>();
            output.push_back(obp.present ? 1 : 0);
            if (obp.present) {
                const i64 packed = BlockPos(obp.pos.x, obp.pos.y, obp.pos.z).asLong();
                _writeBigEndianI64(packed, output);
            }
            break;
        }
        case 13: { // ParticlesValue → PARTICLES
            // VarInt(count) + 每个粒子的 codec。本项目当前仅同步空列表（count=0），
            // 已足够让真 Java 客户端通过 Particles 字段类型校验。
            // （对齐 vanilla ParticleTypes.STREAM_CODEC.apply(ByteBufCodecs.list())）。
            const auto particles = value.get<entity::ParticlesValue>();
            (void)particles;         // 当前恒空列表；扩展粒子同步时改为遍历粒子列表
            _writeVarInt(0, output); // count=0（空列表）
            break;
        }
        case 14: { // OptionalBlockStateValue → OPTIONAL_BLOCK_STATE
            // VarInt(stateId)；0 表示空（present=false 或 stateId=0）。
            // （对齐 vanilla OPTIONAL_BLOCK_STATE_CODEC = VarInt(Block.getId)，0=empty）
            const auto obs = value.get<entity::OptionalBlockStateValue>();
            _writeVarInt(obs.present ? static_cast<i32>(obs.stateId) : 0, output);
            break;
        }
        case 15: { // BlockStateValue → BLOCK_STATE
            // VarInt(stateId)。（对齐 vanilla BLOCK_STATE_CODEC = VarInt(Block.getId)）
            const auto bs = value.get<entity::BlockStateValue>();
            _writeVarInt(static_cast<i32>(bs.stateId), output);
            break;
        }
        case 16: { // DirectionValue → DIRECTION
            // VarInt(Direction 3bit id)。项目 mc::Direction 序与 vanilla 一致。
            // （对齐 vanilla DIRECTION_STREAM_CODEC = idMapper(Direction::byId, Direction::ordinal)）
            const auto dir = value.get<entity::DirectionValue>();
            _writeVarInt(dir.direction, output);
            break;
        }
        case 17: { // OptionalUnsignedIntValue → OPTIONAL_UNSIGNED_INT
            // 1 byte isPresent + 若 present 则 VarInt(value)。
            // （对齐 vanilla OPTIONAL_UNSIGNED_INT = OptionalInt codec：boolean + VarInt）
            const auto oui = value.get<entity::OptionalUnsignedIntValue>();
            output.push_back(oui.present ? 1 : 0);
            if (oui.present) {
                _writeVarInt(oui.value, output);
            }
            break;
        }
        case 18: { // HolderVariantValue → Holder variant（本次统一 serializerId 21 CAT_VARIANT）
            // VarInt(registryId)。Holder 引用模式，0 也是合法 id。
            // TODO(复合类型分批): 逐实体区分 serializerId（21 Cat/22 Cow/23 Wolf/24 WolfSound/
            // 25 Frog/26 Pig/27 Chicken/28 ZombieNautilus）。当前 getSerializerId 统一返回 21。
            const auto hv = value.get<entity::HolderVariantValue>();
            _writeVarInt(hv.registryId, output);
            break;
        }
        case 19: { // VillagerDataValue → VILLAGER_DATA
            // VarInt(type) + VarInt(profession) + VarInt(level)（三段，无长度前缀）。
            // （对齐 vanilla VillagerData.STREAM_CODEC = composite(holderRegistry(VILLAGER_TYPE),
            // holderRegistry(VILLAGER_PROFESSION), VAR_INT)）
            const auto vd = value.get<entity::VillagerDataValue>();
            _writeVarInt(vd.type, output);
            _writeVarInt(vd.profession, output);
            _writeVarInt(vd.level, output);
            break;
        }
        case 20: { // HumanoidArmValue → HUMANOID_ARM
            // VarInt(HumanoidArm ordinal：LEFT=0/RIGHT=1)。用于 Player.MAIN_HAND。
            // （对齐 vanilla HUMANOID_ARM = idMapper(HumanoidArm::byId, HumanoidArm::ordinal)）
            const auto arm = value.get<entity::HumanoidArmValue>();
            _writeVarInt(arm.arm, output);
            break;
        }
        case 21: { // OptionalUuidValue → OPTIONAL_LIVING_ENTITY_REFERENCE
            // 1 byte present + 若 present 则 16 字节大端连续 UUID（MSB 8 字节 BE + LSB 8 字节 BE，
            // 对齐 FriendlyByteBuf.readUUID/writeUUID = UUIDUtil.STREAM_CODEC）。
            // vanilla TamableAnimal.DATA_OWNERUUID_ID 用此 codec（1.21.11 Optional<EntityReference>，
            // EntityReference.streamCodec = UUIDUtil.STREAM_CODEC，wire 仍是 UUID）。
            const auto ou = value.get<entity::OptionalUuidValue>();
            output.push_back(ou.present ? 1 : 0);
            if (ou.present) {
                // Uuid 是 std::array<u8,16>，按大端连续写入（与 _writeBigEndianI64 两次等价）。
                output.insert(output.end(), ou.uuid.begin(), ou.uuid.end());
            }
            break;
        }
        default:
            // 未知类型：写空字节占位（不应发生——getSerializerId 已兜底 Byte）
            output.push_back(0);
            break;
    }
}

// ============================================================================
// 反序列化
// ============================================================================

bool EntityMetadataSerializer::deserialize(const std::vector<u8>& data, entity::EntityDataManager& manager)
{
    size_t offset = 0;

    while (offset < data.size()) {
        // 1.21.11 wire：byte(index) + VarInt(serializerId) + value，0xFF 结束
        u8 index = data[offset++];

        // 检查结束标记
        if (index == 0xFF) {
            break;
        }

        // 序列化器 ID（VarInt，对齐 DataValue.read）
        i32 serializerId = _readVarInt(data.data(), data.size(), offset);
        const auto sid = static_cast<MetadataSerializerId>(serializerId);

        switch (sid) {
            case MetadataSerializerId::Byte: {
                if (offset >= data.size()) return false;
                const i8 value = static_cast<i8>(data[offset++]);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Int: {
                i32 value = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Long: {
                i64 value = _readVarLong(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Float: {
                f32 val;
                if (!_readBigEndianF32(data.data(), data.size(), offset, val)) return false;
                (void)manager.setRaw(index, entity::DataValue(val));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::String:
            case MetadataSerializerId::Component: {
                // Component 暂以字符串承载。1.21.11 真组件 NBT codec 依赖 ITextComponent 的 NBT
                // 序列化（对齐 ComponentSerialization.CODEC），属独立大项；落地后此处改读 NBT compound。
                std::string value = _readString(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::OptionalComponent: {
                // 1.21.11 OPTIONAL_COMPONENT = 1 byte present + 若 present 则 NBT Component。
                // 本项目纯文本折叠为 StringTag 0x08 + U16 BE 长度 + UTF8（与 writeTextComponentNbt 同源）。
                if (offset >= data.size()) return false;
                const bool present = data[offset++] != 0;
                std::string text;
                if (present) {
                    if (offset >= data.size()) return false;
                    const u8 tagId = data[offset++];
                    if (tagId != 0x08) {
                        // 非 StringTag 折叠形式（真复合 NBT 组件），暂不支持——停止解析。
                        return false;
                    }
                    u16 len = 0;
                    if (!_readBigEndianU16(data.data(), data.size(), offset, len)) return false;
                    if (offset + static_cast<size_t>(len) > data.size()) return false;
                    text.assign(reinterpret_cast<const char*>(data.data() + offset), static_cast<size_t>(len));
                    offset += static_cast<size_t>(len);
                }
                (void)manager.setRaw(
                    index, entity::DataValue(entity::OptionalComponentValue{present, std::move(text)}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Pose: {
                // 1.21.11 POSE = VarInt(Pose.id)。枚举值即 vanilla Pose.id（非 ordinal）。
                const i32 poseId = _readVarInt(data.data(), data.size(), offset);
                const auto pose = static_cast<entity::EntityPose>(poseId);
                (void)manager.setRaw(index, entity::DataValue(entity::PoseValue{pose}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Boolean: {
                if (offset >= data.size()) return false;
                bool value = data[offset++] != 0;
                (void)manager.setRaw(index, entity::DataValue(value));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::BlockPos: {
                i64 packed;
                if (!_readBigEndianI64(data.data(), data.size(), offset, packed)) return false;
                const BlockPos pos = BlockPos::fromLong(packed);
                (void)manager.setRaw(index, entity::DataValue(entity::Vector3i(pos.x, pos.y, pos.z)));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Rotations: {
                // 1.21.11 ROTATIONS = f32×3。本项目 DataValue 无独立 Rotations 类型，
                // 反序列化为 Vector3f（覆盖 x/y/z）。Vector2f 写入端补 z=0，读端仍得 Vector3f——
                // 因 Vector2f/Vector3f 当前无活元数据参数，此对称差异不影响线上行为。
                f32 x;
                f32 y;
                f32 z;
                if (!_readBigEndianF32(data.data(), data.size(), offset, x)) return false;
                if (!_readBigEndianF32(data.data(), data.size(), offset, y)) return false;
                if (!_readBigEndianF32(data.data(), data.size(), offset, z)) return false;
                (void)manager.setRaw(index, entity::DataValue(entity::Vector3f(x, y, z)));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::ItemStack: {
                // 1.21.11 ITEM_STACK = OPTIONAL_STREAM_CODEC：VarInt(count)；count<=0 即空；
                // 否则 VarInt(itemId) + DataComponentPatch 字节（无外层长度前缀，patch 以自身
                // VarInt(addedCount)+VarInt(removedCount) 自终止）。patch 是 DataComponentPatch 的
                // 1.21.11 wire 字节，元数据层透传存入 ItemStackView，由客户端 fromItemStackView
                // 还原为业务侧 ItemStack。
                const i32 count = _readVarInt(data.data(), data.size(), offset);
                network::ir::play::ItemStackView view;
                if (count <= 0) {
                    view.itemId = 0;
                    view.count = 0;
                } else {
                    const i32 itemId = _readVarInt(data.data(), data.size(), offset);
                    // 用 ByteBuf 视图按 vanilla 自终止规则消费 patch 区段，原样取出字节。
                    buffer::ByteBuf patchBuf(data.data() + offset, data.size() - offset);
                    auto patchBytesResult = ::mc::item::component::readPatchBytesFromWire(patchBuf);
                    if (patchBytesResult.failed()) {
                        return false;
                    }
                    view.componentsPatch = std::move(patchBytesResult.value());
                    offset += patchBuf.readPosition();
                    view.itemId = static_cast<u32>(itemId);
                    view.count = count;
                }
                (void)manager.setRaw(index, entity::DataValue(view));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::OptionalBlockPos: {
                // 1.21.11 OPTIONAL_BLOCK_POS = 1 byte present + 若 present 则大端 packed i64。
                if (offset >= data.size()) return false;
                const bool present = data[offset++] != 0;
                entity::Vector3i pos{0, 0, 0};
                if (present) {
                    i64 packed;
                    if (!_readBigEndianI64(data.data(), data.size(), offset, packed)) return false;
                    const BlockPos bp = BlockPos::fromLong(packed);
                    pos = entity::Vector3i(bp.x, bp.y, bp.z);
                }
                (void)manager.setRaw(index, entity::DataValue(entity::OptionalBlockPosValue{present, pos}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Particles: {
                // 1.21.11 PARTICLES = VarInt(count) + 每个粒子 codec。本项目当前不支持
                // 真粒子同步，仅消费 count 并跳过 count 个粒子（粒子 codec 暂无项目侧实现，
                // 正常路径客户端不会发非空粒子到服务端）。count=0 即空列表。
                const i32 count = _readVarInt(data.data(), data.size(), offset);
                // 无粒子 codec，仅支持空列表；非空列表无法解析，停止。
                if (count != 0) {
                    return false;
                }
                (void)manager.setRaw(index, entity::DataValue(entity::ParticlesValue{true}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::OptionalBlockState: {
                // VarInt(stateId)；0=空。
                const i32 sid = _readVarInt(data.data(), data.size(), offset);
                const bool present = sid != 0;
                (void)manager.setRaw(
                    index, entity::DataValue(entity::OptionalBlockStateValue{present, static_cast<u32>(sid)}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::BlockState: {
                const i32 sid = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(entity::BlockStateValue{static_cast<u32>(sid)}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::Direction: {
                const i32 d = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(entity::DirectionValue{d}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::OptionalUnsignedInt: {
                // 1 byte isPresent + 若 present 则 VarInt(value)。
                if (offset >= data.size()) return false;
                const bool present = data[offset++] != 0;
                i32 v = 0;
                if (present) {
                    v = _readVarInt(data.data(), data.size(), offset);
                }
                (void)manager.setRaw(index, entity::DataValue(entity::OptionalUnsignedIntValue{present, v}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::VillagerData: {
                // VarInt(type) + VarInt(profession) + VarInt(level)。
                const i32 type = _readVarInt(data.data(), data.size(), offset);
                const i32 profession = _readVarInt(data.data(), data.size(), offset);
                const i32 level = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(entity::VillagerDataValue{type, profession, level}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::CatVariant:
            case MetadataSerializerId::CowVariant:
            case MetadataSerializerId::WolfVariant:
            case MetadataSerializerId::WolfSoundVariant:
            case MetadataSerializerId::FrogVariant:
            case MetadataSerializerId::PigVariant:
            case MetadataSerializerId::ChickenVariant:
            case MetadataSerializerId::ZombieNautilusVariant: {
                // 所有 Holder<Variant> 系列 wire 一致：VarInt(registryId)。
                const i32 id = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(entity::HolderVariantValue{id}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::HumanoidArm: {
                // VarInt(HumanoidArm ordinal：LEFT=0/RIGHT=1)。
                const i32 a = _readVarInt(data.data(), data.size(), offset);
                (void)manager.setRaw(index, entity::DataValue(entity::HumanoidArmValue{a}));
                manager.clearDirty(index);
                break;
            }
            case MetadataSerializerId::OptionalLivingEntityRef: {
                // 1 byte present + 若 present 则 16 字节大端连续 UUID。
                // （对齐 vanilla OPTIONAL_LIVING_ENTITY_REFERENCE = EntityReference.streamCodec()
                // = UUIDUtil.STREAM_CODEC = FriendlyByteBuf.readUUID/writeUUID，16 字节大端连续）
                if (offset >= data.size()) return false;
                const bool present = data[offset++] != 0;
                mc::Uuid uuid{};
                if (present) {
                    if (offset + 16 > data.size()) return false;
                    std::memcpy(uuid.data(), data.data() + offset, 16);
                    offset += 16;
                }
                (void)manager.setRaw(index, entity::DataValue(entity::OptionalUuidValue{present, uuid}));
                manager.clearDirty(index);
                break;
            }
            default:
                // 未实现的序列化器（Particle/VillagerData/BlockState 等）：无法跳过变长值，停止解析。
                // 本项目元数据用 Byte/Int/Long/Float/String/Boolean/ItemStack/Pose/OptionalComponent，
                // 故正常路径不会落到此分支。
                return false;
        }
    }

    return true;
}

// ============================================================================
// 辅助方法
// ============================================================================

void EntityMetadataSerializer::_writeVarInt(i32 value, std::vector<u8>& output) noexcept
{
    u32 uval = static_cast<u32>(value);
    do {
        u8 byte = uval & 0x7F;
        uval >>= 7;
        if (uval != 0) {
            byte |= 0x80;
        }
        output.push_back(byte);
    } while (uval != 0);
}

void EntityMetadataSerializer::_writeVarLong(i64 value, std::vector<u8>& output) noexcept
{
    u64 uval = static_cast<u64>(value);
    do {
        u8 byte = uval & 0x7F;
        uval >>= 7;
        if (uval != 0) {
            byte |= 0x80;
        }
        output.push_back(byte);
    } while (uval != 0);
}

i32 EntityMetadataSerializer::_readVarInt(const u8* data, size_t size, size_t& offset) noexcept
{
    i32 result = 0;
    i32 shift = 0;

    while (offset < size) {
        u8 byte = data[offset++];
        result |= static_cast<i32>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    return result;
}

i64 EntityMetadataSerializer::_readVarLong(const u8* data, size_t size, size_t& offset) noexcept
{
    i64 result = 0;
    i32 shift = 0;

    while (offset < size) {
        u8 byte = data[offset++];
        result |= static_cast<i64>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    return result;
}

void EntityMetadataSerializer::_writeString(const std::string& str, std::vector<u8>& output) noexcept
{
    // 截断过长的字符串
    size_t writeLen = std::min(str.size(), static_cast<size_t>(kMaxStringLength));
    // 字符串长度 (VarInt)
    _writeVarInt(static_cast<i32>(writeLen), output);
    // 字符串内容
    output.insert(output.end(), str.begin(), str.begin() + writeLen);
}

std::string EntityMetadataSerializer::_readString(const u8* data, size_t size, size_t& offset) noexcept
{
    i32 length = _readVarInt(data, size, offset);

    // 验证长度
    if (length < 0) {
        return "";
    }
    if (static_cast<size_t>(length) > kMaxStringLength) {
        return ""; // 字符串过长，返回空字符串
    }
    if (offset + static_cast<size_t>(length) > size) {
        return "";
    }

    std::string result(reinterpret_cast<const char*>(data + offset), static_cast<size_t>(length));
    offset += static_cast<size_t>(length);
    return result;
}

void EntityMetadataSerializer::_writeBigEndianF32(f32 value, std::vector<u8>& output) noexcept
{
    // Java FriendlyByteBuf.writeFloat 大端；按字节序无关方式拼装
    u32 bits;
    std::memcpy(&bits, &value, sizeof(f32));
    output.push_back(static_cast<u8>((bits >> 24) & 0xFF));
    output.push_back(static_cast<u8>((bits >> 16) & 0xFF));
    output.push_back(static_cast<u8>((bits >> 8) & 0xFF));
    output.push_back(static_cast<u8>(bits & 0xFF));
}

void EntityMetadataSerializer::_writeBigEndianI64(i64 value, std::vector<u8>& output) noexcept
{
    u64 bits = static_cast<u64>(value);
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<u8>((bits >> shift) & 0xFF));
    }
}

bool EntityMetadataSerializer::_readBigEndianF32(const u8* data, size_t size, size_t& offset, f32& out) noexcept
{
    if (offset + sizeof(f32) > size) return false;
    u32 bits = (static_cast<u32>(data[offset]) << 24) | (static_cast<u32>(data[offset + 1]) << 16) |
        (static_cast<u32>(data[offset + 2]) << 8) | static_cast<u32>(data[offset + 3]);
    std::memcpy(&out, &bits, sizeof(f32));
    offset += sizeof(f32);
    return true;
}

bool EntityMetadataSerializer::_readBigEndianI64(const u8* data, size_t size, size_t& offset, i64& out) noexcept
{
    if (offset + sizeof(i64) > size) return false;
    u64 bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits = (bits << 8) | static_cast<u64>(data[offset + i]);
    }
    out = static_cast<i64>(bits);
    offset += sizeof(i64);
    return true;
}

void EntityMetadataSerializer::_writeBigEndianU16(u16 value, std::vector<u8>& output) noexcept
{
    output.push_back(static_cast<u8>((value >> 8) & 0xFF));
    output.push_back(static_cast<u8>(value & 0xFF));
}

bool EntityMetadataSerializer::_readBigEndianU16(const u8* data, size_t size, size_t& offset, u16& out) noexcept
{
    if (offset + sizeof(u16) > size) return false;
    out = static_cast<u16>((static_cast<u16>(data[offset]) << 8) | static_cast<u16>(data[offset + 1]));
    offset += sizeof(u16);
    return true;
}

} // namespace mc::network
