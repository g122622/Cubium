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

std::optional<BlockStateProperties::DoorHinge> EnumProperty<BlockStateProperties::DoorHinge>::Traits::fromName(
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

std::optional<BlockStateProperties::DoubleBlockHalf> EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::fromName(
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

std::optional<BlockStateProperties::ChestType> EnumProperty<BlockStateProperties::ChestType>::Traits::fromName(
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

std::optional<BlockStateProperties::AttachFace> EnumProperty<BlockStateProperties::AttachFace>::Traits::fromName(
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

// ============================================================================
// StairsShape Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::StairsShape>::Traits::toString(
    const BlockStateProperties::StairsShape& value) {
    switch (value) {
        case BlockStateProperties::StairsShape::Straight:
            return "straight";
        case BlockStateProperties::StairsShape::InnerLeft:
            return "inner_left";
        case BlockStateProperties::StairsShape::InnerRight:
            return "inner_right";
        case BlockStateProperties::StairsShape::OuterLeft:
            return "outer_left";
        case BlockStateProperties::StairsShape::OuterRight:
            return "outer_right";
        default:
            return "straight";
    }
}

std::optional<BlockStateProperties::StairsShape> EnumProperty<BlockStateProperties::StairsShape>::Traits::fromName(
    StringView name) {
    if (name == "straight") {
        return BlockStateProperties::StairsShape::Straight;
    } else if (name == "inner_left") {
        return BlockStateProperties::StairsShape::InnerLeft;
    } else if (name == "inner_right") {
        return BlockStateProperties::StairsShape::InnerRight;
    } else if (name == "outer_left") {
        return BlockStateProperties::StairsShape::OuterLeft;
    } else if (name == "outer_right") {
        return BlockStateProperties::StairsShape::OuterRight;
    }
    return std::nullopt;
}

// ============================================================================
// SlabType Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::SlabType>::Traits::toString(
    const BlockStateProperties::SlabType& value) {
    switch (value) {
        case BlockStateProperties::SlabType::Bottom:
            return "bottom";
        case BlockStateProperties::SlabType::Top:
            return "top";
        case BlockStateProperties::SlabType::Double:
            return "double";
        default:
            return "bottom";
    }
}

std::optional<BlockStateProperties::SlabType> EnumProperty<BlockStateProperties::SlabType>::Traits::fromName(
    StringView name) {
    if (name == "bottom") {
        return BlockStateProperties::SlabType::Bottom;
    } else if (name == "top") {
        return BlockStateProperties::SlabType::Top;
    } else if (name == "double") {
        return BlockStateProperties::SlabType::Double;
    }
    return std::nullopt;
}

// ============================================================================
// WallHeight Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::WallHeight>::Traits::toString(
    const BlockStateProperties::WallHeight& value) {
    switch (value) {
        case BlockStateProperties::WallHeight::None:
            return "none";
        case BlockStateProperties::WallHeight::Low:
            return "low";
        case BlockStateProperties::WallHeight::Tall:
            return "tall";
        default:
            return "none";
    }
}

std::optional<BlockStateProperties::WallHeight> EnumProperty<BlockStateProperties::WallHeight>::Traits::fromName(
    StringView name) {
    if (name == "none") {
        return BlockStateProperties::WallHeight::None;
    } else if (name == "low") {
        return BlockStateProperties::WallHeight::Low;
    } else if (name == "tall") {
        return BlockStateProperties::WallHeight::Tall;
    }
    return std::nullopt;
}

// ============================================================================
// BedPart Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::BedPart>::Traits::toString(
    const BlockStateProperties::BedPart& value) {
    switch (value) {
        case BlockStateProperties::BedPart::Head:
            return "head";
        case BlockStateProperties::BedPart::Foot:
            return "foot";
        default:
            return "foot";
    }
}

std::optional<BlockStateProperties::BedPart> EnumProperty<BlockStateProperties::BedPart>::Traits::fromName(
    StringView name) {
    if (name == "head") {
        return BlockStateProperties::BedPart::Head;
    } else if (name == "foot") {
        return BlockStateProperties::BedPart::Foot;
    }
    return std::nullopt;
}

// ============================================================================
// BellAttachment Traits 实现
// ============================================================================

String EnumProperty<BlockStateProperties::BellAttachment>::Traits::toString(
    const BlockStateProperties::BellAttachment& value) {
    switch (value) {
        case BlockStateProperties::BellAttachment::Floor:
            return "floor";
        case BlockStateProperties::BellAttachment::Ceiling:
            return "ceiling";
        case BlockStateProperties::BellAttachment::SingleWall:
            return "single_wall";
        case BlockStateProperties::BellAttachment::DoubleWall:
            return "double_wall";
        default:
            return "floor";
    }
}

std::optional<BlockStateProperties::BellAttachment> EnumProperty<BlockStateProperties::BellAttachment>::Traits::fromName(
    StringView name) {
    if (name == "floor") {
        return BlockStateProperties::BellAttachment::Floor;
    } else if (name == "ceiling") {
        return BlockStateProperties::BellAttachment::Ceiling;
    } else if (name == "single_wall") {
        return BlockStateProperties::BellAttachment::SingleWall;
    } else if (name == "double_wall") {
        return BlockStateProperties::BellAttachment::DoubleWall;
    }
    return std::nullopt;
}

} // namespace mc
