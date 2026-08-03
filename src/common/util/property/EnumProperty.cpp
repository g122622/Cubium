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

#include "EnumProperty.hpp"
#include "Properties.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace mc {

// ============================================================================
// DoorHinge Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::DoorHinge>::Traits::toString(
    const BlockStateProperties::DoorHinge& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::toString(
    const BlockStateProperties::DoubleBlockHalf& value)
{
    switch (value) {
        case BlockStateProperties::DoubleBlockHalf::Upper:
            return "upper";
        case BlockStateProperties::DoubleBlockHalf::Lower:
            return "lower";
        default:
            return "lower";
    }
}

std::optional<BlockStateProperties::DoubleBlockHalf>
EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::fromName(std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::ChestType>::Traits::toString(
    const BlockStateProperties::ChestType& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::AttachFace>::Traits::toString(
    const BlockStateProperties::AttachFace& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::StairsShape>::Traits::toString(
    const BlockStateProperties::StairsShape& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::SlabType>::Traits::toString(const BlockStateProperties::SlabType& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::WallHeight>::Traits::toString(
    const BlockStateProperties::WallHeight& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::BedPart>::Traits::toString(const BlockStateProperties::BedPart& value)
{
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
    std::string_view name)
{
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

std::string EnumProperty<BlockStateProperties::BellAttachment>::Traits::toString(
    const BlockStateProperties::BellAttachment& value)
{
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

std::optional<BlockStateProperties::BellAttachment>
EnumProperty<BlockStateProperties::BellAttachment>::Traits::fromName(std::string_view name)
{
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

// ============================================================================
// BambooLeaves Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::BambooLeaves>::Traits::toString(
    const BlockStateProperties::BambooLeaves& value)
{
    switch (value) {
        case BlockStateProperties::BambooLeaves::None:
            return "none";
        case BlockStateProperties::BambooLeaves::Small:
            return "small";
        case BlockStateProperties::BambooLeaves::Large:
            return "large";
        default:
            return "none";
    }
}

std::optional<BlockStateProperties::BambooLeaves> EnumProperty<BlockStateProperties::BambooLeaves>::Traits::fromName(
    std::string_view name)
{
    if (name == "none") {
        return BlockStateProperties::BambooLeaves::None;
    } else if (name == "small") {
        return BlockStateProperties::BambooLeaves::Small;
    } else if (name == "large") {
        return BlockStateProperties::BambooLeaves::Large;
    }
    return std::nullopt;
}

// ============================================================================
// Half Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::Half>::Traits::toString(const BlockStateProperties::Half& value)
{
    switch (value) {
        case BlockStateProperties::Half::Top:
            return "top";
        case BlockStateProperties::Half::Bottom:
            return "bottom";
        default:
            return "bottom";
    }
}

std::optional<BlockStateProperties::Half> EnumProperty<BlockStateProperties::Half>::Traits::fromName(
    std::string_view name)
{
    if (name == "top") {
        return BlockStateProperties::Half::Top;
    } else if (name == "bottom") {
        return BlockStateProperties::Half::Bottom;
    }
    return std::nullopt;
}

// ============================================================================
// RailShape Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::RailShape>::Traits::toString(
    const BlockStateProperties::RailShape& value)
{
    switch (value) {
        case BlockStateProperties::RailShape::NorthSouth:
            return "north_south";
        case BlockStateProperties::RailShape::EastWest:
            return "east_west";
        case BlockStateProperties::RailShape::AscendingEast:
            return "ascending_east";
        case BlockStateProperties::RailShape::AscendingWest:
            return "ascending_west";
        case BlockStateProperties::RailShape::AscendingNorth:
            return "ascending_north";
        case BlockStateProperties::RailShape::AscendingSouth:
            return "ascending_south";
        case BlockStateProperties::RailShape::SouthEast:
            return "south_east";
        case BlockStateProperties::RailShape::SouthWest:
            return "south_west";
        case BlockStateProperties::RailShape::NorthWest:
            return "north_west";
        case BlockStateProperties::RailShape::NorthEast:
            return "north_east";
        default:
            return "north_south";
    }
}

std::optional<BlockStateProperties::RailShape> EnumProperty<BlockStateProperties::RailShape>::Traits::fromName(
    std::string_view name)
{
    if (name == "north_south") {
        return BlockStateProperties::RailShape::NorthSouth;
    } else if (name == "east_west") {
        return BlockStateProperties::RailShape::EastWest;
    } else if (name == "ascending_east") {
        return BlockStateProperties::RailShape::AscendingEast;
    } else if (name == "ascending_west") {
        return BlockStateProperties::RailShape::AscendingWest;
    } else if (name == "ascending_north") {
        return BlockStateProperties::RailShape::AscendingNorth;
    } else if (name == "ascending_south") {
        return BlockStateProperties::RailShape::AscendingSouth;
    } else if (name == "south_east") {
        return BlockStateProperties::RailShape::SouthEast;
    } else if (name == "south_west") {
        return BlockStateProperties::RailShape::SouthWest;
    } else if (name == "north_west") {
        return BlockStateProperties::RailShape::NorthWest;
    } else if (name == "north_east") {
        return BlockStateProperties::RailShape::NorthEast;
    }
    return std::nullopt;
}

// ============================================================================
// RedstoneSide Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::RedstoneSide>::Traits::toString(
    const BlockStateProperties::RedstoneSide& value)
{
    switch (value) {
        case BlockStateProperties::RedstoneSide::Up:
            return "up";
        case BlockStateProperties::RedstoneSide::Side:
            return "side";
        case BlockStateProperties::RedstoneSide::None:
            return "none";
        default:
            return "none";
    }
}

std::optional<BlockStateProperties::RedstoneSide> EnumProperty<BlockStateProperties::RedstoneSide>::Traits::fromName(
    std::string_view name)
{
    if (name == "up") {
        return BlockStateProperties::RedstoneSide::Up;
    } else if (name == "side") {
        return BlockStateProperties::RedstoneSide::Side;
    } else if (name == "none") {
        return BlockStateProperties::RedstoneSide::None;
    }
    return std::nullopt;
}

// ============================================================================
// PistonType Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::PistonType>::Traits::toString(
    const BlockStateProperties::PistonType& value)
{
    switch (value) {
        case BlockStateProperties::PistonType::Default:
            return "normal";
        case BlockStateProperties::PistonType::Sticky:
            return "sticky";
        default:
            return "normal";
    }
}

std::optional<BlockStateProperties::PistonType> EnumProperty<BlockStateProperties::PistonType>::Traits::fromName(
    std::string_view name)
{
    if (name == "normal") {
        return BlockStateProperties::PistonType::Default;
    } else if (name == "sticky") {
        return BlockStateProperties::PistonType::Sticky;
    }
    return std::nullopt;
}

// ============================================================================
// ComparatorMode Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::ComparatorMode>::Traits::toString(
    const BlockStateProperties::ComparatorMode& value)
{
    switch (value) {
        case BlockStateProperties::ComparatorMode::Compare:
            return "compare";
        case BlockStateProperties::ComparatorMode::Subtract:
            return "subtract";
        default:
            return "compare";
    }
}

std::optional<BlockStateProperties::ComparatorMode>
EnumProperty<BlockStateProperties::ComparatorMode>::Traits::fromName(std::string_view name)
{
    if (name == "compare") {
        return BlockStateProperties::ComparatorMode::Compare;
    } else if (name == "subtract") {
        return BlockStateProperties::ComparatorMode::Subtract;
    }
    return std::nullopt;
}

// ============================================================================
// NoteBlockInstrument Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::NoteBlockInstrument>::Traits::toString(
    const BlockStateProperties::NoteBlockInstrument& value)
{
    switch (value) {
        case BlockStateProperties::NoteBlockInstrument::Harp:
            return "harp";
        case BlockStateProperties::NoteBlockInstrument::Basedrum:
            return "basedrum";
        case BlockStateProperties::NoteBlockInstrument::Snare:
            return "snare";
        case BlockStateProperties::NoteBlockInstrument::Hat:
            return "hat";
        case BlockStateProperties::NoteBlockInstrument::Bass:
            return "bass";
        case BlockStateProperties::NoteBlockInstrument::Flute:
            return "flute";
        case BlockStateProperties::NoteBlockInstrument::Bell:
            return "bell";
        case BlockStateProperties::NoteBlockInstrument::Guitar:
            return "guitar";
        case BlockStateProperties::NoteBlockInstrument::Chime:
            return "chime";
        case BlockStateProperties::NoteBlockInstrument::Xylophone:
            return "xylophone";
        case BlockStateProperties::NoteBlockInstrument::IronXylophone:
            return "iron_xylophone";
        case BlockStateProperties::NoteBlockInstrument::CowBell:
            return "cow_bell";
        case BlockStateProperties::NoteBlockInstrument::Didgeridoo:
            return "didgeridoo";
        case BlockStateProperties::NoteBlockInstrument::Bit:
            return "bit";
        case BlockStateProperties::NoteBlockInstrument::Banjo:
            return "banjo";
        case BlockStateProperties::NoteBlockInstrument::Pling:
            return "pling";
        default:
            return "harp";
    }
}

std::optional<BlockStateProperties::NoteBlockInstrument>
EnumProperty<BlockStateProperties::NoteBlockInstrument>::Traits::fromName(std::string_view name)
{
    if (name == "harp") {
        return BlockStateProperties::NoteBlockInstrument::Harp;
    } else if (name == "basedrum") {
        return BlockStateProperties::NoteBlockInstrument::Basedrum;
    } else if (name == "snare") {
        return BlockStateProperties::NoteBlockInstrument::Snare;
    } else if (name == "hat") {
        return BlockStateProperties::NoteBlockInstrument::Hat;
    } else if (name == "bass") {
        return BlockStateProperties::NoteBlockInstrument::Bass;
    } else if (name == "flute") {
        return BlockStateProperties::NoteBlockInstrument::Flute;
    } else if (name == "bell") {
        return BlockStateProperties::NoteBlockInstrument::Bell;
    } else if (name == "guitar") {
        return BlockStateProperties::NoteBlockInstrument::Guitar;
    } else if (name == "chime") {
        return BlockStateProperties::NoteBlockInstrument::Chime;
    } else if (name == "xylophone") {
        return BlockStateProperties::NoteBlockInstrument::Xylophone;
    } else if (name == "iron_xylophone") {
        return BlockStateProperties::NoteBlockInstrument::IronXylophone;
    } else if (name == "cow_bell") {
        return BlockStateProperties::NoteBlockInstrument::CowBell;
    } else if (name == "didgeridoo") {
        return BlockStateProperties::NoteBlockInstrument::Didgeridoo;
    } else if (name == "bit") {
        return BlockStateProperties::NoteBlockInstrument::Bit;
    } else if (name == "banjo") {
        return BlockStateProperties::NoteBlockInstrument::Banjo;
    } else if (name == "pling") {
        return BlockStateProperties::NoteBlockInstrument::Pling;
    }
    return std::nullopt;
}

// ============================================================================
// StructureMode Traits 实现
// ============================================================================

std::string EnumProperty<BlockStateProperties::StructureMode>::Traits::toString(
    const BlockStateProperties::StructureMode& value)
{
    switch (value) {
        case BlockStateProperties::StructureMode::Save:
            return "save";
        case BlockStateProperties::StructureMode::Load:
            return "load";
        case BlockStateProperties::StructureMode::Corner:
            return "corner";
        case BlockStateProperties::StructureMode::Data:
            return "data";
        default:
            return "save";
    }
}

std::optional<BlockStateProperties::StructureMode> EnumProperty<BlockStateProperties::StructureMode>::Traits::fromName(
    std::string_view name)
{
    if (name == "save") {
        return BlockStateProperties::StructureMode::Save;
    } else if (name == "load") {
        return BlockStateProperties::StructureMode::Load;
    } else if (name == "corner") {
        return BlockStateProperties::StructureMode::Corner;
    } else if (name == "data") {
        return BlockStateProperties::StructureMode::Data;
    }
    return std::nullopt;
}

// ============================================================================
// OxidationLevel Traits 实现 (1.17+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::OxidationLevel>::Traits::toString(
    const BlockStateProperties::OxidationLevel& value)
{
    switch (value) {
        case BlockStateProperties::OxidationLevel::Unaffected:
            return "unaffected";
        case BlockStateProperties::OxidationLevel::Exposed:
            return "exposed";
        case BlockStateProperties::OxidationLevel::Weathered:
            return "weathered";
        case BlockStateProperties::OxidationLevel::Oxidized:
            return "oxidized";
        default:
            return "unaffected";
    }
}

std::optional<BlockStateProperties::OxidationLevel>
EnumProperty<BlockStateProperties::OxidationLevel>::Traits::fromName(std::string_view name)
{
    if (name == "unaffected") {
        return BlockStateProperties::OxidationLevel::Unaffected;
    } else if (name == "exposed") {
        return BlockStateProperties::OxidationLevel::Exposed;
    } else if (name == "weathered") {
        return BlockStateProperties::OxidationLevel::Weathered;
    } else if (name == "oxidized") {
        return BlockStateProperties::OxidationLevel::Oxidized;
    }
    return std::nullopt;
}

// ============================================================================
// DripstoneThickness Traits 实现 (1.17+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::DripstoneThickness>::Traits::toString(
    const BlockStateProperties::DripstoneThickness& value)
{
    switch (value) {
        case BlockStateProperties::DripstoneThickness::TipMerge:
            return "tip_merge";
        case BlockStateProperties::DripstoneThickness::Tip:
            return "tip";
        case BlockStateProperties::DripstoneThickness::Frustum:
            return "frustum";
        case BlockStateProperties::DripstoneThickness::Middle:
            return "middle";
        case BlockStateProperties::DripstoneThickness::Base:
            return "base";
        default:
            return "tip";
    }
}

std::optional<BlockStateProperties::DripstoneThickness>
EnumProperty<BlockStateProperties::DripstoneThickness>::Traits::fromName(std::string_view name)
{
    if (name == "tip_merge") {
        return BlockStateProperties::DripstoneThickness::TipMerge;
    } else if (name == "tip") {
        return BlockStateProperties::DripstoneThickness::Tip;
    } else if (name == "frustum") {
        return BlockStateProperties::DripstoneThickness::Frustum;
    } else if (name == "middle") {
        return BlockStateProperties::DripstoneThickness::Middle;
    } else if (name == "base") {
        return BlockStateProperties::DripstoneThickness::Base;
    }
    return std::nullopt;
}

// ============================================================================
// Tilt Traits 实现 (1.17+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::Tilt>::Traits::toString(const BlockStateProperties::Tilt& value)
{
    switch (value) {
        case BlockStateProperties::Tilt::None:
            return "none";
        case BlockStateProperties::Tilt::Unstable:
            return "unstable";
        case BlockStateProperties::Tilt::Partial:
            return "partial";
        case BlockStateProperties::Tilt::Full:
            return "full";
        default:
            return "none";
    }
}

std::optional<BlockStateProperties::Tilt> EnumProperty<BlockStateProperties::Tilt>::Traits::fromName(
    std::string_view name)
{
    if (name == "none") {
        return BlockStateProperties::Tilt::None;
    } else if (name == "unstable") {
        return BlockStateProperties::Tilt::Unstable;
    } else if (name == "partial") {
        return BlockStateProperties::Tilt::Partial;
    } else if (name == "full") {
        return BlockStateProperties::Tilt::Full;
    }
    return std::nullopt;
}

// ============================================================================
// SculkSensorPhase Traits 实现 (1.19+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::SculkSensorPhase>::Traits::toString(
    const BlockStateProperties::SculkSensorPhase& value)
{
    switch (value) {
        case BlockStateProperties::SculkSensorPhase::Inactive:
            return "inactive";
        case BlockStateProperties::SculkSensorPhase::Active:
            return "active";
        case BlockStateProperties::SculkSensorPhase::Cooldown:
            return "cooldown";
        default:
            return "inactive";
    }
}

std::optional<BlockStateProperties::SculkSensorPhase>
EnumProperty<BlockStateProperties::SculkSensorPhase>::Traits::fromName(std::string_view name)
{
    if (name == "inactive") {
        return BlockStateProperties::SculkSensorPhase::Inactive;
    } else if (name == "active") {
        return BlockStateProperties::SculkSensorPhase::Active;
    } else if (name == "cooldown") {
        return BlockStateProperties::SculkSensorPhase::Cooldown;
    }
    return std::nullopt;
}

// ============================================================================
// TrialSpawnerState Traits 实现 (1.21+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::TrialSpawnerState>::Traits::toString(
    const BlockStateProperties::TrialSpawnerState& value)
{
    switch (value) {
        case BlockStateProperties::TrialSpawnerState::Inactive:
            return "inactive";
        case BlockStateProperties::TrialSpawnerState::WaitingForPlayers:
            return "waiting_for_players";
        case BlockStateProperties::TrialSpawnerState::Active:
            return "active";
        case BlockStateProperties::TrialSpawnerState::WaitingForRewardEjection:
            return "waiting_for_reward_ejection";
        case BlockStateProperties::TrialSpawnerState::EjectingReward:
            return "ejecting_reward";
        case BlockStateProperties::TrialSpawnerState::Cooldown:
            return "cooldown";
        default:
            return "inactive";
    }
}

std::optional<BlockStateProperties::TrialSpawnerState>
EnumProperty<BlockStateProperties::TrialSpawnerState>::Traits::fromName(std::string_view name)
{
    if (name == "inactive") {
        return BlockStateProperties::TrialSpawnerState::Inactive;
    } else if (name == "waiting_for_players") {
        return BlockStateProperties::TrialSpawnerState::WaitingForPlayers;
    } else if (name == "active") {
        return BlockStateProperties::TrialSpawnerState::Active;
    } else if (name == "waiting_for_reward_ejection") {
        return BlockStateProperties::TrialSpawnerState::WaitingForRewardEjection;
    } else if (name == "ejecting_reward") {
        return BlockStateProperties::TrialSpawnerState::EjectingReward;
    } else if (name == "cooldown") {
        return BlockStateProperties::TrialSpawnerState::Cooldown;
    }
    return std::nullopt;
}

// ============================================================================
// VaultState Traits 实现 (1.21+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::VaultState>::Traits::toString(
    const BlockStateProperties::VaultState& value)
{
    switch (value) {
        case BlockStateProperties::VaultState::Inactive:
            return "inactive";
        case BlockStateProperties::VaultState::Active:
            return "active";
        case BlockStateProperties::VaultState::Unlocking:
            return "unlocking";
        case BlockStateProperties::VaultState::Ejecting:
            return "ejecting";
        default:
            return "inactive";
    }
}

std::optional<BlockStateProperties::VaultState> EnumProperty<BlockStateProperties::VaultState>::Traits::fromName(
    std::string_view name)
{
    if (name == "inactive") {
        return BlockStateProperties::VaultState::Inactive;
    } else if (name == "active") {
        return BlockStateProperties::VaultState::Active;
    } else if (name == "unlocking") {
        return BlockStateProperties::VaultState::Unlocking;
    } else if (name == "ejecting") {
        return BlockStateProperties::VaultState::Ejecting;
    }
    return std::nullopt;
}

// ============================================================================
// CreakingHeartState Traits 实现 (1.21.2+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::CreakingHeartState>::Traits::toString(
    const BlockStateProperties::CreakingHeartState& value)
{
    switch (value) {
        case BlockStateProperties::CreakingHeartState::Uprooted:
            return "uprooted";
        case BlockStateProperties::CreakingHeartState::Dormant:
            return "dormant";
        case BlockStateProperties::CreakingHeartState::Awake:
            return "awake";
        default:
            return "uprooted";
    }
}

std::optional<BlockStateProperties::CreakingHeartState>
EnumProperty<BlockStateProperties::CreakingHeartState>::Traits::fromName(std::string_view name)
{
    if (name == "uprooted") {
        return BlockStateProperties::CreakingHeartState::Uprooted;
    } else if (name == "dormant") {
        return BlockStateProperties::CreakingHeartState::Dormant;
    } else if (name == "awake") {
        return BlockStateProperties::CreakingHeartState::Awake;
    }
    return std::nullopt;
}

// ============================================================================
// SideChainPart Traits 实现 (1.21.4+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::SideChainPart>::Traits::toString(
    const BlockStateProperties::SideChainPart& value)
{
    switch (value) {
        case BlockStateProperties::SideChainPart::Unconnected:
            return "unconnected";
        case BlockStateProperties::SideChainPart::Left:
            return "left";
        case BlockStateProperties::SideChainPart::Center:
            return "center";
        case BlockStateProperties::SideChainPart::Right:
            return "right";
    }
    return "unconnected";
}

std::optional<BlockStateProperties::SideChainPart> EnumProperty<BlockStateProperties::SideChainPart>::Traits::fromName(
    std::string_view name)
{
    if (name == "unconnected") {
        return BlockStateProperties::SideChainPart::Unconnected;
    } else if (name == "left") {
        return BlockStateProperties::SideChainPart::Left;
    } else if (name == "center") {
        return BlockStateProperties::SideChainPart::Center;
    } else if (name == "right") {
        return BlockStateProperties::SideChainPart::Right;
    }
    return std::nullopt;
}

// ============================================================================
// CopperGolemPose Traits 实现 (1.21.11+)
// ============================================================================

std::string EnumProperty<BlockStateProperties::CopperGolemPose>::Traits::toString(
    const BlockStateProperties::CopperGolemPose& value)
{
    switch (value) {
        case BlockStateProperties::CopperGolemPose::Standing:
            return "standing";
        case BlockStateProperties::CopperGolemPose::Sitting:
            return "sitting";
        case BlockStateProperties::CopperGolemPose::Running:
            return "running";
        case BlockStateProperties::CopperGolemPose::Star:
            return "star";
        default:
            return "standing";
    }
}

std::optional<BlockStateProperties::CopperGolemPose>
EnumProperty<BlockStateProperties::CopperGolemPose>::Traits::fromName(std::string_view name)
{
    if (name == "standing") {
        return BlockStateProperties::CopperGolemPose::Standing;
    } else if (name == "sitting") {
        return BlockStateProperties::CopperGolemPose::Sitting;
    } else if (name == "running") {
        return BlockStateProperties::CopperGolemPose::Running;
    } else if (name == "star") {
        return BlockStateProperties::CopperGolemPose::Star;
    }
    return std::nullopt;
}

} // namespace mc
