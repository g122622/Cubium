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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"

#include <vector>

namespace mc::client::ui::minecraft::targetinfo {

enum class TargetInfoKind : u8 {
    None,
    Block,
    Entity,
};

class TargetInfoSnapshot {
public:
    TargetInfoSnapshot(TargetInfoKind kind, std::string title, std::vector<std::string> details, u32 accentColor);

    [[nodiscard]] static TargetInfoSnapshot none();

    [[nodiscard]] bool hasTarget() const noexcept { return m_kind != TargetInfoKind::None; }
    [[nodiscard]] TargetInfoKind kind() const noexcept { return m_kind; }
    [[nodiscard]] const std::string& title() const noexcept { return m_title; }
    [[nodiscard]] const std::vector<std::string>& details() const noexcept { return m_details; }
    [[nodiscard]] u32 accentColor() const noexcept { return m_accentColor; }

private:
    TargetInfoKind m_kind;
    std::string m_title;
    std::vector<std::string> m_details;
    u32 m_accentColor;
};

[[nodiscard]] std::string humanizeIdentifier(std::string_view identifier);
[[nodiscard]] std::string humanizeResourceLocation(const ResourceLocation& location);
[[nodiscard]] std::string formatDistance(f32 distance);
[[nodiscard]] std::string formatBlockPos(const BlockPos& pos);
[[nodiscard]] std::string formatDirection(Direction direction);

} // namespace mc::client::ui::minecraft::targetinfo