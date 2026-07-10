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

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/effect/fire/FireTextureLoader.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::test {
namespace {

// 与 test_resource_manager_cloud_texture.cpp 中的 InMemoryResourcePack 保持一致的内存资源包实现
class InMemoryResourcePack final : public IResourcePack {
public:
    InMemoryResourcePack() = default;

    Result<void> initialize() override { return Result<void>::ok(); }

    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }

    [[nodiscard]] bool hasResource(resource::PackType type, std::string_view resourcePath) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        std::string full;
        if (hasTypePrefix) {
            full = path;
        } else {
            full = typeDir + "/" + path;
        }
        return m_resources.find(full) != m_resources.end();
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(
        resource::PackType type, std::string_view resourcePath) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        std::string full;
        if (hasTypePrefix) {
            full = path;
        } else {
            full = typeDir + "/" + path;
        }
        auto it = m_resources.find(full);
        if (it == m_resources.end()) {
            return Error(ErrorCode::NotFound, "Resource not found");
        }
        return it->second;
    }

    [[nodiscard]] Result<std::vector<std::string>> listResources(
        resource::PackType type, std::string_view directory, std::string_view extension) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string fullDirectory = typeDir + "/" + std::string(directory);
        std::vector<std::string> result;
        const std::string ext(extension);
        for (const auto& [path, _] : m_resources) {
            const bool inDir = fullDirectory.empty() || path.rfind(fullDirectory, 0) == 0;
            const bool extMatch =
                ext.empty() || (path.size() >= ext.size() && path.substr(path.size() - ext.size()) == ext);
            if (inDir && extMatch) {
                result.push_back(path);
            }
        }
        return result;
    }

    [[nodiscard]] Result<std::vector<std::string>> getResourceNamespaces(resource::PackType type) const override
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string prefix = typeDir + "/";
        std::unordered_set<std::string> namespaces;
        for (const auto& [path, _] : m_resources) {
            if (path.size() > prefix.size() && path.substr(0, prefix.size()) == prefix) {
                std::string rest = path.substr(prefix.size());
                size_t slashPos = rest.find('/');
                if (slashPos != std::string::npos) {
                    namespaces.insert(rest.substr(0, slashPos));
                }
            }
        }
        std::vector<std::string> result(namespaces.begin(), namespaces.end());
        std::sort(result.begin(), result.end());
        return result;
    }

    [[nodiscard]] std::string name() const override { return "InMemoryResourcePack"; }

    void add(std::string path, std::vector<u8> bytes) { m_resources.emplace(std::move(path), std::move(bytes)); }

private:
    PackMetadata m_metadata{6, "test-pack"};
    std::unordered_map<std::string, std::vector<u8>> m_resources;
};

// 1x1 PNG（来自 test_resource_manager_cloud_texture.cpp）
std::vector<u8> makeValid1x1Png()
{
    return {137,
        80,
        78,
        71,
        13,
        10,
        26,
        10,
        0,
        0,
        0,
        13,
        73,
        72,
        68,
        82,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        1,
        8,
        4,
        0,
        0,
        0,
        181,
        28,
        12,
        2,
        0,
        0,
        0,
        11,
        73,
        68,
        65,
        84,
        120,
        218,
        99,
        252,
        255,
        31,
        0,
        3,
        3,
        2,
        0,
        239,
        156,
        7,
        219,
        0,
        0,
        0,
        0,
        73,
        69,
        78,
        68,
        174,
        66,
        96,
        130};
}

// 极简 PNG 编码器：生成指定宽高的 RGBA PNG（无压缩优化，使用 stored blocks）
// 仅为测试需要而实现，避免依赖外部测试资源文件。
std::vector<u8> makePng(u32 width, u32 height, std::function<u8(u32 x, u32 y, u32 channel)> pixelAt)
{
    // 构建 RGBA 像素缓冲区
    std::vector<u8> raw;
    raw.reserve((static_cast<size_t>(width) * 4 + 1) * height);
    for (u32 y = 0; y < height; ++y) {
        raw.push_back(0); // filter byte: None
        for (u32 x = 0; x < width; ++x) {
            raw.push_back(pixelAt(x, y, 0)); // R
            raw.push_back(pixelAt(x, y, 1)); // G
            raw.push_back(pixelAt(x, y, 2)); // B
            raw.push_back(pixelAt(x, y, 3)); // A
        }
    }

    // PNG 使用 zlib 包装：2 字节 header + stored blocks + 4 字节 Adler32
    std::vector<u8> zlib;
    zlib.push_back(0x78); // CMF
    zlib.push_back(0x01); // FLG (no preset dict, fastest)

    size_t pos = 0;
    while (pos < raw.size()) {
        size_t chunk = std::min<size_t>(raw.size() - pos, 65535);
        zlib.push_back(pos + chunk >= raw.size() ? 0x01 : 0x00); // BFINAL
        zlib.push_back(static_cast<u8>(chunk & 0xFF));           // BLEN low
        zlib.push_back(static_cast<u8>((chunk >> 8) & 0xFF));    // BLEN high
        zlib.push_back(static_cast<u8>(~chunk & 0xFF));          // NLEN low
        zlib.push_back(static_cast<u8>((~chunk >> 8) & 0xFF));   // NLEN high
        zlib.insert(zlib.end(), raw.begin() + pos, raw.begin() + pos + chunk);
        pos += chunk;
    }

    // Adler32
    auto adler32 = [](const std::vector<u8>& data) -> u32 {
        u32 a = 1;
        u32 b = 0;
        for (auto byte : data) {
            a = (a + byte) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    };
    u32 adler = adler32(raw);
    zlib.push_back(static_cast<u8>((adler >> 24) & 0xFF));
    zlib.push_back(static_cast<u8>((adler >> 16) & 0xFF));
    zlib.push_back(static_cast<u8>((adler >> 8) & 0xFF));
    zlib.push_back(static_cast<u8>(adler & 0xFF));

    // CRC32 表
    const std::array<u32, 256> crcTable = [] {
        std::array<u32, 256> t{};
        for (u32 i = 0; i < 256; ++i) {
            u32 c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    auto crc32 = [&crcTable](const u8* data, size_t len) -> u32 {
        u32 c = 0xFFFFFFFFu;
        for (size_t i = 0; i < len; ++i) {
            c = crcTable[(c ^ data[i]) & 0xFF] ^ (c >> 8);
        }
        return c ^ 0xFFFFFFFFu;
    };

    auto writeU32 = [](std::vector<u8>& out, u32 v) {
        out.push_back(static_cast<u8>((v >> 24) & 0xFF));
        out.push_back(static_cast<u8>((v >> 16) & 0xFF));
        out.push_back(static_cast<u8>((v >> 8) & 0xFF));
        out.push_back(static_cast<u8>(v & 0xFF));
    };

    std::vector<u8> png;
    png.insert(png.end(), {137, 80, 78, 71, 13, 10, 26, 10}); // PNG signature

    // IHDR
    std::vector<u8> ihdr;
    writeU32(ihdr, width);
    writeU32(ihdr, height);
    ihdr.push_back(8); // bit depth
    ihdr.push_back(6); // color type: RGBA
    ihdr.push_back(0); // compression
    ihdr.push_back(0); // filter
    ihdr.push_back(0); // interlace
    png.push_back(0);
    png.push_back(0);
    png.push_back(0);
    png.push_back(static_cast<u8>(ihdr.size()));
    std::vector<u8> ihdrChunk = {'I', 'H', 'D', 'R'};
    ihdrChunk.insert(ihdrChunk.end(), ihdr.begin(), ihdr.end());
    u32 ihdrCrc = crc32(ihdrChunk.data(), ihdrChunk.size());
    png.insert(png.end(), ihdrChunk.begin(), ihdrChunk.end());
    writeU32(png, ihdrCrc);

    // IDAT
    writeU32(png, static_cast<u32>(zlib.size()));
    std::vector<u8> idatChunk = {'I', 'D', 'A', 'T'};
    idatChunk.insert(idatChunk.end(), zlib.begin(), zlib.end());
    u32 idatCrc = crc32(idatChunk.data(), idatChunk.size());
    png.insert(png.end(), idatChunk.begin(), idatChunk.end());
    writeU32(png, idatCrc);

    // IEND
    std::vector<u8> iendChunk = {'I', 'E', 'N', 'D'};
    writeU32(png, 0u);
    u32 iendCrc = crc32(iendChunk.data(), iendChunk.size());
    png.insert(png.end(), iendChunk.begin(), iendChunk.end());
    writeU32(png, iendCrc);

    return png;
}

// 生成空的 mcmeta JSON（默认动画：顺序播放、frametime=1）
std::vector<u8> makeEmptyMcmeta()
{
    const std::string json = R"({"animation":{}})";
    return std::vector<u8>(json.begin(), json.end());
}

// 生成自定义帧序列的 mcmeta JSON
std::vector<u8> makeCustomFrameMcmeta(const std::vector<i32>& frameIndices)
{
    std::string json = R"({"animation":{"frames":[)";
    for (size_t i = 0; i < frameIndices.size(); ++i) {
        if (i > 0) {
            json += ",";
        }
        json += std::to_string(frameIndices[i]);
    }
    json += R"(]}})";
    return std::vector<u8>(json.begin(), json.end());
}

using namespace client::renderer::entity::effect::fire;

TEST(FireTextureLoaderTest, ReturnsProceduralFallbackWhenNoPacks)
{
    FireTextureData data = loadFireTextureData({});
    EXPECT_EQ(data.frameWidth, 16u);
    EXPECT_EQ(data.frameHeight, 16u);
    // 程序化占位：fire_0 和 fire_1 各 1 帧
    EXPECT_EQ(data.fire0FrameCount, 1u);
    EXPECT_EQ(data.fire1FrameCount, 1u);
    EXPECT_EQ(data.frameCount, 2u);
    EXPECT_EQ(data.pixels.size(), static_cast<size_t>(16) * 16 * 2 * 4);
}

TEST(FireTextureLoaderTest, LoadsSingleFramePngAndDuplicatesToTwoFrames)
{
    auto pack = std::make_shared<InMemoryResourcePack>();
    // 仅提供 fire_0.png（1x1），缺失 fire_1.png
    pack->add("assets/minecraft/textures/block/fire_0.png", makeValid1x1Png());

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);
    EXPECT_EQ(data.frameWidth, 1u);
    EXPECT_EQ(data.frameHeight, 1u);
    // fire_1 复制自 fire_0，各 1 帧
    EXPECT_EQ(data.fire0FrameCount, 1u);
    EXPECT_EQ(data.fire1FrameCount, 1u);
    EXPECT_EQ(data.frameCount, 2u);
    EXPECT_EQ(data.pixels.size(), static_cast<size_t>(1) * 1 * 2 * 4);
}

TEST(FireTextureLoaderTest, LoadsBothFramesFromResourcePack)
{
    auto pack = std::make_shared<InMemoryResourcePack>();
    // 提供 2x2 的 fire_0.png 和 fire_1.png，像素不同
    auto fire0 = makePng(2, 2, [](u32 x, u32 y, u32 c) -> u8 { return static_cast<u8>((x + y * 2 + c) & 0xFF); });
    auto fire1 = makePng(2, 2, [](u32 x, u32 y, u32 c) -> u8 { return static_cast<u8>((x + y * 2 + c + 100) & 0xFF); });
    pack->add("assets/minecraft/textures/block/fire_0.png", fire0);
    pack->add("assets/minecraft/textures/block/fire_1.png", fire1);

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);
    ASSERT_EQ(data.frameWidth, 2u);
    ASSERT_EQ(data.frameHeight, 2u);
    ASSERT_EQ(data.fire0FrameCount, 1u);
    ASSERT_EQ(data.fire1FrameCount, 1u);
    ASSERT_EQ(data.frameCount, 2u);
    ASSERT_EQ(data.pixels.size(), static_cast<size_t>(2) * 2 * 2 * 4);

    // 校验第一帧（fire_0）像素
    for (u32 y = 0; y < 2; ++y) {
        for (u32 x = 0; x < 2; ++x) {
            for (u32 c = 0; c < 4; ++c) {
                size_t idx = (y * 2 + x) * 4 + c;
                u8 expected = static_cast<u8>((x + y * 2 + c) & 0xFF);
                EXPECT_EQ(data.pixels[idx], expected) << "frame0 mismatch at (" << x << "," << y << ",c=" << c;
            }
        }
    }
    // 校验第二帧（fire_1）像素
    for (u32 y = 0; y < 2; ++y) {
        for (u32 x = 0; x < 2; ++x) {
            for (u32 c = 0; c < 4; ++c) {
                size_t idx = (4 + y * 2 + x) * 4 + c;
                u8 expected = static_cast<u8>((x + y * 2 + c + 100) & 0xFF);
                EXPECT_EQ(data.pixels[idx], expected) << "frame1 mismatch at (" << x << "," << y << ",c=" << c;
            }
        }
    }
}

TEST(FireTextureLoaderTest, ExtractsAllFramesFromAnimationStrip)
{
    // 模拟原版资源包提供的 16x512 动画条带（32 帧）
    // 每帧 16x16，每帧的所有像素设为帧索引值
    auto strip = makePng(16, 512, [](u32 x, u32 y, u32 c) -> u8 {
        const u32 frameIdx = y / 16;
        return static_cast<u8>(frameIdx & 0xFF);
    });

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/textures/block/fire_0.png", strip);

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);
    EXPECT_EQ(data.frameWidth, 16u);
    EXPECT_EQ(data.frameHeight, 16u);
    // fire_0 有 32 帧，fire_1 复制自 fire_0 也有 32 帧
    EXPECT_EQ(data.fire0FrameCount, 32u);
    EXPECT_EQ(data.fire1FrameCount, 32u);
    EXPECT_EQ(data.frameCount, 64u);
    EXPECT_EQ(data.pixels.size(), static_cast<size_t>(16) * 16 * 64 * 4);

    // 校验 fire_0 的每一帧像素
    for (u32 frame = 0; frame < 32; ++frame) {
        for (u32 y = 0; y < 16; ++y) {
            for (u32 x = 0; x < 16; ++x) {
                size_t idx = (static_cast<size_t>(frame) * 16 * 16 + y * 16 + x) * 4;
                u8 expected = static_cast<u8>(frame & 0xFF);
                for (u32 c = 0; c < 4; ++c) {
                    EXPECT_EQ(data.pixels[idx + c], expected)
                        << "fire_0 frame " << frame << " pixel (" << x << "," << y << ",c=" << c << ")";
                }
            }
        }
    }
}

TEST(FireTextureLoaderTest, HigherPriorityPackOverridesLowerPriority)
{
    // 两个包：低优先级包提供 fire_0.png（像素值为 10），高优先级包提供 fire_0.png（像素值为 200）
    auto lowPriority = std::make_shared<InMemoryResourcePack>();
    lowPriority->add(
        "assets/minecraft/textures/block/fire_0.png", makePng(1, 1, [](u32, u32, u32) -> u8 { return 10; }));

    auto highPriority = std::make_shared<InMemoryResourcePack>();
    highPriority->add(
        "assets/minecraft/textures/block/fire_0.png", makePng(1, 1, [](u32, u32, u32) -> u8 { return 200; }));

    // packs 按优先级从低到高排列：lowPriority 在前，highPriority 在后
    std::vector<IResourcePack*> packs{lowPriority.get(), highPriority.get()};
    FireTextureData data = loadFireTextureData(packs);
    ASSERT_EQ(data.frameWidth, 1u);
    ASSERT_EQ(data.frameHeight, 1u);
    ASSERT_EQ(data.pixels.size(), static_cast<size_t>(1) * 1 * 2 * 4);
    // 高优先级应当覆盖，R 通道为 200
    EXPECT_EQ(data.pixels[0], 200u);
}

TEST(FireTextureLoaderTest, ProceduralFallbackHasNonZeroPixels)
{
    FireTextureData data = loadFireTextureData({});
    // 程序化纹理至少有一个非零像素
    bool hasNonZero = false;
    for (auto byte : data.pixels) {
        if (byte != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

// ========== mcmeta 动画元数据测试 ==========

TEST(FireTextureLoaderTest, ReadsEmptyMcmetaForAnimationStrip)
{
    // 提供 fire_0.png 16x512 + fire_0.png.mcmeta（空动画配置）
    auto strip = makePng(16, 512, [](u32 x, u32 y, u32 c) -> u8 {
        const u32 frameIdx = y / 16;
        return static_cast<u8>(frameIdx & 0xFF);
    });

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/textures/block/fire_0.png", strip);
    pack->add("assets/minecraft/textures/block/fire_0.png.mcmeta", makeEmptyMcmeta());

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);

    // mcmeta 为空动画配置，默认 frametime=1，帧尺寸自动检测为 16x16
    ASSERT_EQ(data.fire0FrameCount, 32u);
    EXPECT_TRUE(data.fire0Metadata.isValidAnimation());
    EXPECT_EQ(data.fire0Metadata.frametime, 1);
    EXPECT_EQ(data.fire0Metadata.width, 16);
    EXPECT_EQ(data.fire0Metadata.height, 16);
    // 空帧序列表示顺序播放 0..31
    EXPECT_TRUE(data.fire0Metadata.frames.empty());
}

TEST(FireTextureLoaderTest, ReadsCustomFrameSequenceFromMcmeta)
{
    // 提供 fire_0.png 16x32（2 帧）+ 自定义帧序列 mcmeta
    // fire_0.png.mcmeta 帧序列为 [1, 0]（先播放第 1 帧再播放第 0 帧）
    auto strip = makePng(16, 32, [](u32 x, u32 y, u32 c) -> u8 {
        const u32 frameIdx = y / 16;
        return static_cast<u8>(frameIdx & 0xFF);
    });

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/textures/block/fire_0.png", strip);
    pack->add("assets/minecraft/textures/block/fire_0.png.mcmeta", makeCustomFrameMcmeta({1, 0}));

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);

    ASSERT_EQ(data.fire0FrameCount, 2u);
    EXPECT_TRUE(data.fire0Metadata.isValidAnimation());
    ASSERT_EQ(data.fire0Metadata.frames.size(), 2u);
    EXPECT_EQ(data.fire0Metadata.frames[0].index, 1);
    EXPECT_EQ(data.fire0Metadata.frames[1].index, 0);
}

TEST(FireTextureLoaderTest, ReadsVanillaFire0McmetaFrameSequence)
{
    // 模拟原版 fire_0.png.mcmeta 的帧序列：[16..31, 0..15]
    auto strip = makePng(16, 512, [](u32 x, u32 y, u32 c) -> u8 {
        const u32 frameIdx = y / 16;
        return static_cast<u8>(frameIdx & 0xFF);
    });

    // 构造原版 fire_0.png.mcmeta 内容
    std::vector<i32> vanillaFrames;
    for (i32 i = 16; i < 32; ++i) {
        vanillaFrames.push_back(i);
    }
    for (i32 i = 0; i < 16; ++i) {
        vanillaFrames.push_back(i);
    }

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/textures/block/fire_0.png", strip);
    pack->add("assets/minecraft/textures/block/fire_0.png.mcmeta", makeCustomFrameMcmeta(vanillaFrames));

    // fire_1 也提供，避免被复制
    auto fire1 = makePng(16, 512, [](u32 x, u32 y, u32 c) -> u8 {
        const u32 frameIdx = y / 16;
        return static_cast<u8>((frameIdx + 50) & 0xFF);
    });
    pack->add("assets/minecraft/textures/block/fire_1.png", fire1);
    pack->add("assets/minecraft/textures/block/fire_1.png.mcmeta", makeEmptyMcmeta());

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);

    // fire_0 有 32 帧，帧序列长度 32
    ASSERT_EQ(data.fire0FrameCount, 32u);
    ASSERT_EQ(data.fire0Metadata.frames.size(), 32u);

    // 校验帧序列：前 16 帧索引 16..31，后 16 帧索引 0..15
    for (i32 i = 0; i < 16; ++i) {
        EXPECT_EQ(data.fire0Metadata.frames[i].index, 16 + i) << "frame " << i;
    }
    for (i32 i = 0; i < 16; ++i) {
        EXPECT_EQ(data.fire0Metadata.frames[16 + i].index, i) << "frame " << (16 + i);
    }

    // fire_1 有 32 帧，空帧序列（顺序播放）
    ASSERT_EQ(data.fire1FrameCount, 32u);
    EXPECT_TRUE(data.fire1Metadata.frames.empty());
}

TEST(FireTextureLoaderTest, BothTexturesShareSameFrameDimensions)
{
    // fire_0 和 fire_1 帧尺寸必须一致
    auto fire0 = makePng(16, 32, [](u32, u32, u32) -> u8 { return 0; });   // 2 帧
    auto fire1 = makePng(16, 64, [](u32, u32, u32) -> u8 { return 128; }); // 4 帧

    auto pack = std::make_shared<InMemoryResourcePack>();
    pack->add("assets/minecraft/textures/block/fire_0.png", fire0);
    pack->add("assets/minecraft/textures/block/fire_1.png", fire1);

    std::vector<IResourcePack*> packs{pack.get()};
    FireTextureData data = loadFireTextureData(packs);

    EXPECT_EQ(data.frameWidth, 16u);
    EXPECT_EQ(data.frameHeight, 16u);
    EXPECT_EQ(data.fire0FrameCount, 2u);
    EXPECT_EQ(data.fire1FrameCount, 4u);
    EXPECT_EQ(data.frameCount, 6u);
    EXPECT_EQ(data.pixels.size(), static_cast<size_t>(16) * 16 * 6 * 4);
}

} // namespace
} // namespace mc::test
