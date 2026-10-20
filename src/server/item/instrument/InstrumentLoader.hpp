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

#include "common/core/Result.hpp"
#include "common/item/instrument/InstrumentDefinition.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <memory>

namespace mc::resource {
class DataPackRepository;
class IResourcePack;
} // namespace mc::resource

namespace mc {
namespace server::item {

/**
 * @brief 乐器数据包加载器（对应 vanilla `Instrument` 的 `DIRECT_CODEC`）
 *
 * 加载路径：`data/<namespace>/instrument/<path>.json`。
 *
 * JSON 字段（全部必填，与 vanilla 一致）：
 * @code
 * {
 *   "description": { "translate": "instrument.minecraft.ponder_goat_horn" },
 *   "range": 256.0,
 *   "sound_event": "minecraft:item.goat_horn.sound.0",
 *   "use_duration": 7.0
 * }
 * @endcode
 *
 * `use_duration` 与 `range` 必须为正数（vanilla `ExtraCodecs.POSITIVE_FLOAT`）。
 */
class InstrumentLoader {
public:
    InstrumentLoader() = delete;

    /**
     * @brief 从数据包仓库加载全部乐器定义
     *
     * 同名条目覆盖注册表中的内置定义；解析失败的条目告警后跳过。
     *
     * @param dataPackList 数据包仓库
     * @return 加载的乐器数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载全部乐器定义
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 解析单个乐器 JSON 对象
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param location 乐器资源位置（用于错误信息）
     * @return 乐器定义，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<mc::item::instrument::InstrumentDefinition>> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& location);
};

} // namespace server::item
} // namespace mc
