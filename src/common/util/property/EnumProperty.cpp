#include "EnumProperty.hpp"
#include "Properties.hpp"

namespace mc {

// ============================================================================
// DoorHinge Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::DoorHinge>::Traits::toString(
    const BlockStateProperties::DoorHinge& value) {
    switch (value) {
        case BlockStateProperties::DoorHinge::Left:
            return "left";
        case BlockStateProperties::DoorHinge::Right:
            return "right";
        default:
            return "left";
    }
}

Optional<BlockStateProperties::DoorHinge> EnumProperty<BlockStateProperties::DoorHinge>::Traits::fromName(
    StringView name) {
    if (name == "left") {
        return BlockStateProperties::DoorHinge::Left;
    } else if (name == "right") {
        return BlockStateProperties::DoorHinge::Right;
    }
    return std::nullopt;
}

// ============================================================================
// DoubleBlockHalf Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::toString(
    const BlockStateProperties::DoubleBlockHalf& value) {
    switch (value) {
        case BlockStateProperties::DoubleBlockHalf::Upper:
            return "upper";
        case BlockStateProperties::DoubleBlockHalf::Lower:
            return "lower";
        default:
            return "lower";
    }
}

Optional<BlockStateProperties::DoubleBlockHalf> EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::fromName(
    StringView name) {
    if (name == "upper") {
        return BlockStateProperties::DoubleBlockHalf::Upper;
    } else if (name == "lower") {
        return BlockStateProperties::DoubleBlockHalf::Lower;
    }
    return std::nullopt;
}

// ============================================================================
// ChestType Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::ChestType>::Traits::toString(
    const BlockStateProperties::ChestType& value) {
    switch (value) {
        case BlockStateProperties::ChestType::Single:
            return "single";
        case BlockStateProperties::ChestType::Left:
            return "left";
        case BlockStateProperties::ChestType::Right:
            return "right";
        default:
            return "single";
    }
}

Optional<BlockStateProperties::ChestType> EnumProperty<BlockStateProperties::ChestType>::Traits::fromName(
    StringView name) {
    if (name == "single") {
        return BlockStateProperties::ChestType::Single;
    } else if (name == "left") {
        return BlockStateProperties::ChestType::Left;
    } else if (name == "right") {
        return BlockStateProperties::ChestType::Right;
    }
    return std::nullopt;
}

// ============================================================================
// AttachFace Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::AttachFace>::Traits::toString(
    const BlockStateProperties::AttachFace& value) {
    switch (value) {
        case BlockStateProperties::AttachFace::Floor:
            return "floor";
        case BlockStateProperties::AttachFace::Wall:
            return "wall";
        case BlockStateProperties::AttachFace::Ceiling:
            return "ceiling";
        default:
            return "wall";
    }
}

Optional<BlockStateProperties::AttachFace> EnumProperty<BlockStateProperties::AttachFace>::Traits::fromName(
    StringView name) {
    if (name == "floor") {
        return BlockStateProperties::AttachFace::Floor;
    } else if (name == "wall") {
        return BlockStateProperties::AttachFace::Wall;
    } else if (name == "ceiling") {
        return BlockStateProperties::AttachFace::Ceiling;
    }
    return std::nullopt;
}

} // namespace mc
