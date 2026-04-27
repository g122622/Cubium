#include "AnimationMetadata.hpp"
#include <nlohmann/json.hpp>

namespace mc::resource::metadata {

AnimationMetadata AnimationMetadata::fromJson(const nlohmann::json& json) {
    AnimationMetadata metadata;

    if (!json.is_object()) {
        return metadata;
    }

    // 解析frametime（默认1）
    metadata.frametime = json.value("frametime", 1);
    if (metadata.frametime <= 0) {
        metadata.frametime = 1;
    }

    // 解析width和height（-1表示自动检测）
    metadata.width = json.value("width", -1);
    metadata.height = json.value("height", -1);

    // 解析interpolate标志
    metadata.interpolate = json.value("interpolate", false);

    // 解析frames数组
    if (json.contains("frames") && json["frames"].is_array()) {
        const auto& framesArray = json["frames"];
        metadata.frames.reserve(framesArray.size());

        for (const auto& frame : framesArray) {
            if (frame.is_number_integer()) {
                // 简单形式：直接是帧索引数字
                metadata.frames.emplace_back(frame.get<i32>(), -1);
            } else if (frame.is_object()) {
                // 对象形式：{ "index": N, "time": M }
                i32 index = frame.value("index", 0);
                i32 time = frame.value("time", -1);
                metadata.frames.emplace_back(index, time);
            }
        }
    }

    return metadata;
}

AnimationMetadata AnimationMetadata::fromMcmeta(
    const std::vector<u8>& mcmetaData,
    u32 imageWidth,
    u32 imageHeight)
{
    if (mcmetaData.empty() || imageWidth == 0 || imageHeight == 0) {
        return AnimationMetadata();
    }

    try {
        const String jsonText(mcmetaData.begin(), mcmetaData.end());
        const auto json = nlohmann::json::parse(jsonText);

        if (!json.is_object() || !json.contains("animation")) {
            return AnimationMetadata();
        }

        AnimationMetadata metadata = fromJson(json["animation"]);

        // 自动检测帧尺寸
        if (metadata.width <= 0 && metadata.height <= 0) {
            // 默认使用方形帧（宽度等于图像宽度）
            metadata.width = static_cast<i32>(imageWidth);
            metadata.height = static_cast<i32>(imageWidth);
        } else if (metadata.width <= 0) {
            metadata.width = metadata.height;
        } else if (metadata.height <= 0) {
            metadata.height = metadata.width;
        }

        // 验证帧尺寸
        if (metadata.width <= 0 || metadata.height <= 0) {
            return AnimationMetadata();
        }

        if (static_cast<u32>(metadata.width) > imageWidth ||
            static_cast<u32>(metadata.height) > imageHeight) {
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
    } catch (const nlohmann::json::exception& e) {
        // JSON解析失败
        return AnimationMetadata();
    }
}

} // namespace mc::resource::metadata
