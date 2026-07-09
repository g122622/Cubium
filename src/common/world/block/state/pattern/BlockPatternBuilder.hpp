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

#include "BlockPattern.hpp"
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::blockpattern {

/**
 * @brief 方块模式构建器
 *
 * 对应 MC Java: net.minecraft.world.level.block.state.pattern.BlockPatternBuilder
 *
 * 流式 API 构建 3D 方块模式：
 * @code
 * auto pattern = BlockPatternBuilder::start()
 *     .aisle("       ", "       ", "       ", "   #   ", "       ", "       ", "       ")
 *     .aisle("       ", "       ", "       ", "   #   ", "       ", "       ", "       ")
 *     // ... 更多 aisle
 *     .where('#', BlockInWorld::hasState([](const BlockState& s){ return s.is(block); }))
 *     .build();
 * @endcode
 *
 * - 每个 aisle() 调用添加一层深度（depth）
 * - aisle 的字符串数组对应高度（height），字符串长度对应宽度（width）
 * - 字符 ' ' 默认匹配任何方块，其他字符需通过 where() 绑定谓词
 * - build() 时校验所有字符均已绑定谓词
 */
class BlockPatternBuilder {
public:
    BlockPatternBuilder()
    {
        // 空格默认匹配任何方块
        m_lookup[' '] = [](const BlockInWorld&) { return true; };
    }

    /**
     * @brief 添加一层模式（aisle）
     *
     * 对应 MC Java: BlockPatternBuilder.aisle(String...)
     *
     * @param aisle 高度方向的字符串数组，每个字符串代表一行的宽度
     * @return 自身引用（支持链式调用）
     */
    BlockPatternBuilder& aisle(std::vector<std::string> aisle);

    /**
     * @brief 创建构建器
     */
    static BlockPatternBuilder start() { return {}; }

    /**
     * @brief 绑定字符到谓词
     *
     * 对应 MC Java: BlockPatternBuilder.where(char, Predicate<BlockInWorld>)
     *
     * @param c 字符
     * @param predicate 谓词
     * @return 自身引用
     */
    BlockPatternBuilder& where(char c, BlockPattern::Predicate predicate);

    /**
     * @brief 构建方块模式
     *
     * 校验所有字符均已绑定谓词，然后构造 BlockPattern。
     *
     * @return 方块模式（unique_ptr 所有权）
     */
    [[nodiscard]] std::unique_ptr<BlockPattern> build();

private:
    /**
     * @brief 构建三维谓词数组
     *
     * 对应 MC Java: BlockPatternBuilder.createPattern()
     */
    [[nodiscard]] std::vector<std::vector<std::vector<BlockPattern::Predicate>>> _createPattern() const;

    std::vector<std::vector<std::string>> m_pattern;            ///< 每层 aisle 的字符串数组
    std::unordered_map<char, BlockPattern::Predicate> m_lookup; ///< 字符到谓词的映射
    i32 m_height = 0;
    i32 m_width = 0;
    std::set<char> m_unknownCharacters; ///< 未绑定谓词的字符
};

} // namespace mc::blockpattern
