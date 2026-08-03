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

#include "AnimationMetadata.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::resource::metadata {

AnimationMetadata AnimationMetadata::fromJson(const nlohmann::json& json)
{
    AnimationMetadata metadata;

    if (!json.is_object()) {
        return metadata;
    }

    // 解析frametime（默认1），如果显式指定必须 >= 1
    metadata.frametime = json.value("frametime", 1);
    if (metadata.frametime < 1) {
        metadata.frametime = 1;
    }

    // 解析width和height（-1表示自动检测），如果显式指定必须 >= 1
    metadata.width = json.value("width", -1);
    metadata.height = json.value("height", -1);

    // 解析interpolate标志
    metadata.interpolate = json.value("interpolate", false);

    // 解析frames数组
    if (json.contains("frames") && json["frames"].is_array()) {
        const auto& framesArray = json["frames"];
        metadata.frames.reserve(framesArray.size());

        for (size_t i = 0; i < framesArray.size(); ++i) {
            const auto& frame = framesArray[i];
            if (frame.is_number_integer()) {
                // 简单形式：直接是帧索引数字
                const i32 index = frame.get<i32>();
                // 帧索引必须 >= 0
                if (index < 0) {
                    continue; // 跳过无效帧
                }
                metadata.frames.emplace_back(index, -1);
            } else if (frame.is_object()) {
                // 对象形式：{ "index": N, "time": M }
                // index是必需字段
                if (!frame.contains("index")) {
                    continue; // 跳过无效帧
                }
                const i32 index = frame["index"].get<i32>();
                // 帧索引必须 >= 0
                if (index < 0) {
                    continue; // 跳过无效帧
                }

                i32 time = -1;
                if (frame.contains("time")) {
                    time = frame["time"].get<i32>();
                    // 如果显式指定time，必须 >= 1
                    if (time < 1) {
                        time = -1; // 使用默认值
                    }
                }
                metadata.frames.emplace_back(index, time);
            }
        }
    }

    return metadata;
}

AnimationMetadata AnimationMetadata::fromMcmeta(const std::vector<u8>& mcmetaData, u32 imageWidth, u32 imageHeight)
{
    if (mcmetaData.empty() || imageWidth == 0 || imageHeight == 0) {
        return AnimationMetadata();
    }

    try {
        const std::string jsonText(mcmetaData.begin(), mcmetaData.end());
        const auto json = nlohmann::json::parse(jsonText);

        if (!json.is_object() || !json.contains("animation")) {
            return AnimationMetadata();
        }

        AnimationMetadata metadata = fromJson(json["animation"]);

        // 自动检测帧尺寸：当width和height都未指定时，使用min(imageWidth, imageHeight)
        if (metadata.width <= 0 && metadata.height <= 0) {
            const u32 minDim = std::min(imageWidth, imageHeight);
            metadata.width = static_cast<i32>(minDim);
            metadata.height = static_cast<i32>(minDim);
        } else if (metadata.width <= 0) {
            metadata.width = metadata.height;
        } else if (metadata.height <= 0) {
            metadata.height = metadata.width;
        }

        // 验证帧尺寸
        if (metadata.width <= 0 || metadata.height <= 0) {
            return AnimationMetadata();
        }

        if (static_cast<u32>(metadata.width) > imageWidth || static_cast<u32>(metadata.height) > imageHeight) {
            return AnimationMetadata();
        }

        // 验证帧尺寸能整除图像尺寸
        if ((imageWidth % static_cast<u32>(metadata.width)) != 0 ||
            (imageHeight % static_cast<u32>(metadata.height)) != 0) {
            return AnimationMetadata();
        }

        // 如果没有自定义帧序列，计算总帧数
        // 总帧数 = 图像高度 / 帧高度
        // 注意：帧序列会在AnimatedSprite中根据实际帧数填充

        return metadata;
    }
    catch (const nlohmann::json::exception& e) {
        // JSON解析失败
        return AnimationMetadata();
    }
}

} // namespace mc::resource::metadata
