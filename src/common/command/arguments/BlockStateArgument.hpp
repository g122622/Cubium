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

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ArgumentType.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace command {

/**
 * @brief 方块状态输入结果
 *
 * 封装解析后的方块状态，包含方块状态指针。
 * 支持 `/setblock`、`/fill` 等命令使用。
 */
class BlockStateInput {
public:
    BlockStateInput() noexcept = default;
    explicit BlockStateInput(const BlockState* state) noexcept
        : m_state(state)
    {}

    [[nodiscard]] const BlockState* state() const noexcept { return m_state; }
    [[nodiscard]] bool isValid() const noexcept { return m_state != nullptr; }
    [[nodiscard]] const Block& getBlock() const noexcept { return m_state->getBlock(); }
    [[nodiscard]] u32 stateId() const noexcept { return m_state ? m_state->stateId() : 0; }

private:
    const BlockState* m_state = nullptr;
};

/**
 * @brief 方块状态参数类型
 *
 * 解析格式：`minecraft:stone[facing=north,half=top]` 或 `stone`
 *
 * 支持：
 * - 方块 ID 解析（带命名空间或默认 minecraft: 命名空间）
 * - 方块状态属性解析（方括号内的 key=value 格式）
 * - 完整的错误处理（未知方块、未知属性、无效值等）
 */
class BlockStateArgumentType : public ArgumentType<BlockStateInput> {
public:
    [[nodiscard]] BlockStateInput parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();

        // 读取方块 ID 部分（可能包含命名空间和属性）
        std::string blockIdStr = reader.readString();

        // 解析方块 ID 和属性
        return _parseBlockState(blockIdStr, reader, start);
    }

    [[nodiscard]] std::string getTypeName() const noexcept override { return "block_state"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"stone", "minecraft:stone", "stone[facing=north]", "oak_stairs[half=top,facing=east]"};
    }

    static std::shared_ptr<BlockStateArgumentType> blockState() { return std::make_shared<BlockStateArgumentType>(); }

private:
    /**
     * @brief 解析方块状态字符串
     *
     * @param input 输入字符串，如 "minecraft:stone[facing=north,half=top]"
     * @param reader 字符串读取器（用于错误定位）
     * @param start 开始位置
     * @return 解析后的方块状态输入
     */
    [[nodiscard]] BlockStateInput _parseBlockState(const std::string& input, StringReader& reader, i32 start)
    {
        // 分离方块名和属性部分
        size_t bracketPos = input.find('[');
        std::string blockName;
        std::string propsStr;

        if (bracketPos == std::string::npos) {
            // 没有属性
            blockName = input;
        } else {
            // 有属性
            blockName = input.substr(0, bracketPos);
            propsStr = input.substr(bracketPos + 1);
            // 移除结尾的 ']'
            if (!propsStr.empty() && propsStr.back() == ']') {
                propsStr.pop_back();
            }
        }

        // 解析 ResourceLocation
        ResourceLocation location = ResourceLocation::parse(blockName);

        // 获取方块
        Block* block = BlockRegistry::instance().getBlock(location);
        if (block == nullptr) {
            throw CommandException(CommandErrorType::Unknown, "Unknown block: " + location.toString(), start);
        }

        // 获取默认状态
        const BlockState* state = &block->defaultState();

        // 解析并应用属性
        if (!propsStr.empty()) {
            state = _applyProperties(block, state, propsStr, reader, start);
        }

        return BlockStateInput(state);
    }

    /**
     * @brief 解析并应用属性字符串
     *
     * @param block 方块
     * @param defaultState 默认状态
     * @param propsStr 属性字符串，如 "facing=north,half=top"
     * @param reader 字符串读取器
     * @param start 开始位置
     * @return 应用属性后的方块状态
     */
    [[nodiscard]] const BlockState* _applyProperties(const Block* block,
        const BlockState* defaultState,
        const std::string& propsStr,
        StringReader& reader,
        i32 start)
    {
        const auto& container = block->stateContainer();
        const auto& properties = container.properties();

        // 解析属性键值对
        std::unordered_map<const IProperty*, size_t> wantedProps;

        size_t startPos = 0;
        while (startPos < propsStr.size()) {
            // 跳过空白
            while (startPos < propsStr.size() && (propsStr[startPos] == ' ' || propsStr[startPos] == '\t')) {
                startPos++;
            }
            if (startPos >= propsStr.size()) break;

            // 读取属性名
            size_t eqPos = propsStr.find('=', startPos);
            if (eqPos == std::string::npos) {
                throw CommandException(CommandErrorType::Unknown,
                    "Expected '=' after property name in: " + propsStr.substr(startPos),
                    start);
            }

            std::string propName = propsStr.substr(startPos, eqPos - startPos);
            // 去除属性名两端的空白
            propName.erase(0, propName.find_first_not_of(" \t"));
            propName.erase(propName.find_last_not_of(" \t") + 1);

            // 读取属性值
            size_t valueStart = eqPos + 1;
            // 跳过值前的空白
            while (valueStart < propsStr.size() && (propsStr[valueStart] == ' ' || propsStr[valueStart] == '\t')) {
                valueStart++;
            }

            size_t valueEnd = propsStr.find(',', valueStart);
            if (valueEnd == std::string::npos) {
                valueEnd = propsStr.size();
            }

            std::string propValue = propsStr.substr(valueStart, valueEnd - valueStart);
            // 去除值两端的空白
            propValue.erase(0, propValue.find_first_not_of(" \t"));
            propValue.erase(propValue.find_last_not_of(" \t") + 1);

            // 查找属性
            auto propIt = properties.find(propName);
            if (propIt == properties.end()) {
                throw CommandException(CommandErrorType::Unknown,
                    "Unknown property '" + propName + "' for block " + block->blockLocation().toString(),
                    start);
            }

            const IProperty* prop = propIt->second;

            // 检查重复属性
            if (wantedProps.find(prop) != wantedProps.end()) {
                throw CommandException(
                    CommandErrorType::Unknown, "Duplicate property '" + propName + "' in block state", start);
            }

            // 解析属性值
            auto parsedValue = prop->parseValue(propValue);
            if (!parsedValue) {
                throw CommandException(CommandErrorType::Unknown,
                    "Invalid value '" + propValue + "' for property '" + propName + "' of block " +
                        block->blockLocation().toString(),
                    start);
            }

            wantedProps[prop] = *parsedValue;

            // 移动到下一个属性
            startPos = valueEnd + 1;
        }

        // 查找匹配的状态
        if (!wantedProps.empty()) {
            // 遍历所有状态查找匹配的
            for (const auto& candidate : container.validStates()) {
                if (!candidate) continue;

                bool matches = true;
                for (const auto& [prop, index] : wantedProps) {
                    const auto valueIndex = candidate->getValueIndex(*prop);
                    if (!valueIndex.has_value() || *valueIndex != index) {
                        matches = false;
                        break;
                    }
                }

                if (matches) {
                    return candidate.get();
                }
            }

            // 没有找到匹配的状态，构建错误消息
            std::string errorMsg = "No matching block state found for properties: ";
            bool first = true;
            for (const auto& [prop, index] : wantedProps) {
                if (!first) errorMsg += ", ";
                errorMsg += prop->name() + "=" + prop->valueToString(index);
                first = false;
            }
            throw CommandException(CommandErrorType::Unknown, errorMsg, start);
        }

        return defaultState;
    }
};

} // namespace command
} // namespace mc
