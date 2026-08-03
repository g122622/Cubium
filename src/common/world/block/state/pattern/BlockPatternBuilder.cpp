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

#include "BlockPatternBuilder.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/state/pattern/BlockPattern.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::blockpattern {

BlockPatternBuilder& BlockPatternBuilder::aisle(std::vector<std::string> aisle)
{
    // 对应 MC Java: BlockPatternBuilder.aisle
    if (aisle.empty() || aisle[0].empty()) {
        throw std::invalid_argument("Empty pattern for aisle");
    }

    if (m_pattern.empty()) {
        m_height = static_cast<i32>(aisle.size());
        m_width = static_cast<i32>(aisle[0].size());
    }

    if (static_cast<i32>(aisle.size()) != m_height) {
        throw std::invalid_argument("Expected aisle with height of " + std::to_string(m_height) +
            ", but was given one with a height of " + std::to_string(aisle.size()) + ")");
    }

    for (const std::string& s : aisle) {
        if (static_cast<i32>(s.size()) != m_width) {
            throw std::invalid_argument("Not all rows in the given aisle are the correct width (expected " +
                std::to_string(m_width) + ", found one with " + std::to_string(s.size()) + ")");
        }

        for (char c : s) {
            if (m_lookup.find(c) == m_lookup.end()) {
                m_unknownCharacters.insert(c);
            }
        }
    }

    m_pattern.push_back(std::move(aisle));
    return *this;
}

BlockPatternBuilder& BlockPatternBuilder::where(char c, BlockPattern::Predicate predicate)
{
    m_lookup[c] = std::move(predicate);
    m_unknownCharacters.erase(c);
    return *this;
}

std::unique_ptr<BlockPattern> BlockPatternBuilder::build()
{
    return std::make_unique<BlockPattern>(_createPattern());
}

std::vector<std::vector<std::vector<BlockPattern::Predicate>>> BlockPatternBuilder::_createPattern() const
{
    if (!m_unknownCharacters.empty()) {
        std::string chars;
        for (char c : m_unknownCharacters) {
            if (!chars.empty()) {
                chars += ", ";
            }
            chars += c;
        }
        spdlog::error("BlockPatternBuilder: Predicates for character(s) {} are missing", chars);
        throw std::runtime_error("Predicates for character(s) " + chars + " are missing");
    }

    // predicate[depth][height][width]
    std::vector<std::vector<std::vector<BlockPattern::Predicate>>> predicate;
    predicate.resize(m_pattern.size());

    for (size_t i = 0; i < m_pattern.size(); ++i) {
        predicate[i].resize(static_cast<size_t>(m_height));
        for (size_t j = 0; j < static_cast<size_t>(m_height); ++j) {
            predicate[i][j].resize(static_cast<size_t>(m_width));
            for (size_t k = 0; k < static_cast<size_t>(m_width); ++k) {
                const char c = m_pattern[i][j][k];
                auto it = m_lookup.find(c);
                if (it == m_lookup.end()) {
                    // 不应到达此处（unknownCharacters 已校验），防御性抛异常
                    throw std::runtime_error("Missing predicate for character");
                }
                predicate[i][j][k] = it->second;
            }
        }
    }

    return predicate;
}

} // namespace mc::blockpattern
