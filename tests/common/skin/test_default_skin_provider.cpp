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

#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/pack/PackMetadata.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/manager/DefaultSkinProvider.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::skin;

namespace {

/// 内存资源包，仿照 tests/client/renderer/test_renderer.cpp 中的同名实现
class InMemoryResourcePack final : public resource::IResourcePack {
public:
    Result<void> initialize() override { return Result<void>::ok(); }

    [[nodiscard]] const resource::PackMetadata& metadata() const override { return m_metadata; }

    [[nodiscard]] bool hasResource(resource::PackType type, std::string_view resourcePath) const override
    {
        return m_resources.find(makeFullKey(type, resourcePath)) != m_resources.end();
    }

    [[nodiscard]] Result<std::vector<u8>> readResource(
        resource::PackType type, std::string_view resourcePath) const override
    {
        const auto it = m_resources.find(makeFullKey(type, resourcePath));
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
    /// readResource 接受的路径已剥离 "assets/" 前缀；这里还原完整键以便查表
    [[nodiscard]] static std::string makeFullKey(resource::PackType type, std::string_view resourcePath)
    {
        std::string typeDir(resource::packTypeDirectoryName(type));
        std::string path(resourcePath);
        bool hasTypePrefix = path.size() > typeDir.size() && path.substr(0, typeDir.size() + 1) == typeDir + "/";
        if (hasTypePrefix) {
            return path;
        }
        return typeDir + "/" + path;
    }

    resource::PackMetadata m_metadata{6, "test-pack"};
    std::unordered_map<std::string, std::vector<u8>> m_resources;
};

/// 生成 64x64 指定纯色 RGBA 像素的 PNG 字节流
/// 使用最简 PNG 结构：IHDR + IDAT（zlib 1 块，stored 块无压缩）+ IEND
std::vector<u8> makeSolidColor64x64Png(u8 r, u8 g, u8 b, u8 a)
{
    // PNG 签名
    static const u8 kSignature[] = {137, 80, 78, 71, 13, 10, 26, 10};

    // IHDR chunk: width=64, height=64, bitdepth=8, colortype=6 (RGBA), compression=0, filter=0, interlace=0
    static const u8 kIhdrType[] = {'I', 'H', 'D', 'R'};
    static const u8 kIhdrData[] = {
        0,
        0,
        0,
        64, // width
        0,
        0,
        0,
        64, // height
        8,  // bit depth
        6,  // color type (RGBA)
        0,  // compression
        0,  // filter
        0   // interlace
    };

    // IDAT chunk: stored zlib block, 每行前置 filter byte (0 = None)，每行 64*4+1 = 257 字节，共 64 行
    // 总数据 = 2 字节 zlib header + 4 字节 stored block header (len+nlen) + 257*64 字节 + 4 字节 adler32
    // 注意：stored block 单块最大 65535 字节，257*64 = 16448 字节，一个 stored block 足够
    constexpr size_t kRowBytes = 64 * 4;
    constexpr size_t kRowSizeWithFilter = kRowBytes + 1; // 每行前置 filter byte
    constexpr size_t kRawSize = kRowSizeWithFilter * 64;

    std::vector<u8> idatData;
    idatData.reserve(2 + 5 + kRawSize + 4);
    // zlib header: CMF=0x78, FLG=0x01 (no preset dictionary, fastest)
    idatData.push_back(0x78);
    idatData.push_back(0x01);
    // stored block: BFINAL=1 (last block), BTYPE=00 (stored)
    // 字节对齐后跟 LEN (2 字节 LE) 和 NLEN (2 字节 LE)
    idatData.push_back(0x01); // BFINAL=1, BTYPE=00
    const u16 len = static_cast<u16>(kRawSize);
    idatData.push_back(static_cast<u8>(len & 0xFF));
    idatData.push_back(static_cast<u8>((len >> 8) & 0xFF));
    const u16 nlen = static_cast<u16>(~len);
    idatData.push_back(static_cast<u8>(nlen & 0xFF));
    idatData.push_back(static_cast<u8>((nlen >> 8) & 0xFF));
    // raw data: 64 行
    for (size_t y = 0; y < 64; ++y) {
        idatData.push_back(0); // filter byte: None
        for (size_t x = 0; x < 64; ++x) {
            idatData.push_back(r); // R
            idatData.push_back(g); // G
            idatData.push_back(b); // B
            idatData.push_back(a); // A
        }
    }
    // Adler-32 校验和（按字节计算）。变量名加 adler 前缀避免与函数参数 r/g/b/a 重名遮蔽
    u32 adlerA = 1, adlerB = 0;
    for (size_t i = 2; i < idatData.size(); ++i) { // 跳过 zlib header 2 字节
        adlerA = (adlerA + idatData[i]) % 65521;
        adlerB = (adlerB + adlerA) % 65521;
    }
    u32 adler32 = (adlerB << 16) | adlerA;
    idatData.push_back(static_cast<u8>((adler32 >> 24) & 0xFF));
    idatData.push_back(static_cast<u8>((adler32 >> 16) & 0xFF));
    idatData.push_back(static_cast<u8>((adler32 >> 8) & 0xFF));
    idatData.push_back(static_cast<u8>(adler32 & 0xFF));

    auto makeChunk = [](const u8* type, const u8* data, size_t dataSize) -> std::vector<u8> {
        std::vector<u8> chunk;
        chunk.reserve(8 + dataSize + 4);
        // length (big-endian)
        chunk.push_back(static_cast<u8>((dataSize >> 24) & 0xFF));
        chunk.push_back(static_cast<u8>((dataSize >> 16) & 0xFF));
        chunk.push_back(static_cast<u8>((dataSize >> 8) & 0xFF));
        chunk.push_back(static_cast<u8>(dataSize & 0xFF));
        // type
        chunk.insert(chunk.end(), type, type + 4);
        // data
        if (dataSize > 0) {
            chunk.insert(chunk.end(), data, data + dataSize);
        }
        // CRC32 over type + data
        static const u32 kCrcTable[256] = {
            0x00000000,
            0x77073096,
            0xee0e612c,
            0x990951ba,
            0x076dc419,
            0x706af48f,
            0xe963a535,
            0x9e6495a3,
            0x0edb8832,
            0x79dcb8a4,
            0xe0d5e91e,
            0x97d2d988,
            0x09b64c2b,
            0x7eb17cbd,
            0xe7b82d07,
            0x90bf1d91,
            0x1db71064,
            0x6ab020f2,
            0xf3b97148,
            0x84be41de,
            0x1adad47d,
            0x6ddde4eb,
            0xf4d4b551,
            0x83d385c7,
            0x136c9856,
            0x646ba8c0,
            0xfd62f97a,
            0x8a65c9ec,
            0x14015c4f,
            0x63066cd9,
            0xfa0f3d63,
            0x8d080df5,
            0x3b6e20c8,
            0x4c69105e,
            0xd56041e4,
            0xa2677172,
            0x3c03e4d1,
            0x4b04d447,
            0xd20d85fd,
            0xa50ab56b,
            0x35b5a8fa,
            0x42b2986c,
            0xdbbbc9d6,
            0xacbcf940,
            0x32d86ce3,
            0x45df5c75,
            0xdcd60dcf,
            0xabd13d59,
            0x26d930ac,
            0x51de003a,
            0xc8d75180,
            0xbfd06116,
            0x21b4f4b5,
            0x56b3c423,
            0xcfba9599,
            0xb8bda50f,
            0x2802b89e,
            0x5f058808,
            0xc60cd9b2,
            0xb10be924,
            0x2f6f7c87,
            0x58684c11,
            0xc1611dab,
            0xb6662d3d,
            0x76dc4190,
            0x01db7106,
            0x98d220bc,
            0xefd5102a,
            0x71b18589,
            0x06b6b51f,
            0x9fbfe4a5,
            0xe8b8d433,
            0x7807c9a2,
            0x0f00f934,
            0x9609a88e,
            0xe10e9818,
            0x7f6a0dbb,
            0x086d3d2d,
            0x91646c97,
            0xe6635c01,
            0x6b6b51f4,
            0x1c6c6162,
            0x856530d8,
            0xf262004e,
            0x6c0695ed,
            0x1b01a57b,
            0x8208f4c1,
            0xf50fc457,
            0x65b0d9c6,
            0x12b7e950,
            0x8bbeb8ea,
            0xfcb9887c,
            0x62dd1ddf,
            0x15da2d49,
            0x8cd37cf3,
            0xfbd44c65,
            0x4db26158,
            0x3ab551ce,
            0xa3bc0074,
            0xd4bb30e2,
            0x4adfa541,
            0x3dd895d7,
            0xa4d1c46d,
            0xd3d6f4fb,
            0x4369e96a,
            0x346ed9fc,
            0xad678846,
            0xda60b8d0,
            0x44042d73,
            0x33031de5,
            0xaa0a4c5f,
            0xdd0d7cc9,
            0x5005713c,
            0x270241aa,
            0xbe0b1010,
            0xc90c2086,
            0x5768b525,
            0x206f85b3,
            0xb966d409,
            0xce61e49f,
            0x5edef90e,
            0x29d9c998,
            0xb0d09822,
            0xc7d7a8b4,
            0x59b33d17,
            0x2eb40d81,
            0xb7bd5c3b,
            0xc0ba6cad,
            0xedb88320,
            0x9abfb3b6,
            0x03b6e20c,
            0x74b1d29a,
            0xead54739,
            0x9dd277af,
            0x04db2615,
            0x73dc1683,
            0xe3630b12,
            0x94643b84,
            0x0d6d6a3e,
            0x7a6a5aa8,
            0xe40ecf0b,
            0x9309ff9d,
            0x0a00ae27,
            0x7d079eb1,
            0xf00f9344,
            0x8708a3d2,
            0x1e01f268,
            0x6906c2fe,
            0xf762575d,
            0x806567cb,
            0x196c3671,
            0x6e6b06e7,
            0xfed41b76,
            0x89d32be0,
            0x10da7a5a,
            0x67dd4acc,
            0xf9b9df6f,
            0x8ebeeff9,
            0x17b7be43,
            0x60b08ed5,
            0xd6d6a3e8,
            0xa1d1937e,
            0x38d8c2c4,
            0x4fdff252,
            0xd1bb67f1,
            0xa6bc5767,
            0x3fb506dd,
            0x48b2364b,
            0xd80d2bda,
            0xaf0a1b4c,
            0x36034af6,
            0x41047a60,
            0xdf60efc3,
            0xa867df55,
            0x316e8eef,
            0x4669be79,
            0xcb61b38c,
            0xbc66831a,
            0x256fd2a0,
            0x5268e236,
            0xcc0c7795,
            0xbb0b4703,
            0x220216b9,
            0x5505262f,
            0xc5ba3bbe,
            0xb2bd0b28,
            0x2bb45a92,
            0x5cb36a04,
            0xc2d7ffa7,
            0xb5d0cf31,
            0x2cd99e8b,
            0x5bdeae1d,
            0x9b64c2b0,
            0xec63f226,
            0x756aa39c,
            0x026d930a,
            0x9c0906a9,
            0xeb0e363f,
            0x72076785,
            0x05005713,
            0x95bf4a82,
            0xe2b87a14,
            0x7bb12bae,
            0x0cb61b38,
            0x92d28e9b,
            0xe5d5be0d,
            0x7cdcefb7,
            0x0bdbdf21,
            0x86d3d2d4,
            0xf1d4e242,
            0x68ddb3f8,
            0x1fda836e,
            0x81be16cd,
            0xf6b9265b,
            0x6fb077e1,
            0x18b74777,
            0x88085ae6,
            0xff0f6a70,
            0x66063bca,
            0x11010b5c,
            0x8f659eff,
            0xf862ae69,
            0x616bffd3,
            0x166ccf45,
            0xa00ae278,
            0xd70dd2ee,
            0x4e048354,
            0x3903b3c2,
            0xa7672661,
            0xd06016f7,
            0x4969474d,
            0x3e6e77db,
            0xaed16a4a,
            0xd9d65adc,
            0x40df0b66,
            0x37d83bf0,
            0xa9bcae53,
            0xdebb9ec5,
            0x47b2cf7f,
            0x30b5ffe9,
            0xbdbdf21c,
            0xcabac28a,
            0x53b39330,
            0x24b4a3a6,
            0xbad03605,
            0xcdd70693,
            0x54de5729,
            0x23d967bf,
            0xb3667a2e,
            0xc4614ab8,
            0x5d681b02,
            0x2a6f2b94,
            0xb40bbe37,
            0xc30c8ea1,
            0x5a05df1b,
            0x2d02ef8d,
        };
        u32 crc = 0xFFFFFFFFu;
        auto update = [&](u8 byte) { crc = kCrcTable[(crc ^ byte) & 0xFF] ^ (crc >> 8); };
        for (int i = 0; i < 4; ++i) {
            update(type[i]);
        }
        for (size_t i = 0; i < dataSize; ++i) {
            update(data[i]);
        }
        crc ^= 0xFFFFFFFFu;
        chunk.push_back(static_cast<u8>((crc >> 24) & 0xFF));
        chunk.push_back(static_cast<u8>((crc >> 16) & 0xFF));
        chunk.push_back(static_cast<u8>((crc >> 8) & 0xFF));
        chunk.push_back(static_cast<u8>(crc & 0xFF));
        return chunk;
    };

    std::vector<u8> png;
    png.insert(png.end(), kSignature, kSignature + sizeof(kSignature));
    auto ihdr = makeChunk(kIhdrType, kIhdrData, sizeof(kIhdrData));
    png.insert(png.end(), ihdr.begin(), ihdr.end());
    static const u8 kIdatType[] = {'I', 'D', 'A', 'T'};
    auto idat = makeChunk(kIdatType, idatData.data(), idatData.size());
    png.insert(png.end(), idat.begin(), idat.end());
    static const u8 kIendType[] = {'I', 'E', 'N', 'D'};
    auto iend = makeChunk(kIendType, nullptr, 0);
    png.insert(png.end(), iend.begin(), iend.end());
    return png;
}

/// 生成 64x64 全不透明红色 RGBA 像素的 PNG 字节流（makeSolidColor64x64Png 的便捷包装）
std::vector<u8> makeSolidRed64x64Png()
{
    return makeSolidColor64x64Png(255, 0, 0, 255);
}

/// 为 18 个默认皮肤变体在内存资源包中注入纯红色 64x64 PNG
void populateAllDefaultSkinPngs(InMemoryResourcePack& pack)
{
    const auto& variants = getDefaultSkinVariants();
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        std::string resourcePath = variants[i].textureLocation().toFilePath(resource::PackType::ClientResources);
        // toFilePath 返回 "assets/minecraft/textures/entity/player/{slim|wide}/{name}.png"
        // IResourcePack::readResource 接受 "assets/" 前缀的完整路径（InMemoryResourcePack 兼容两种格式）
        pack.add(resourcePath, makeSolidRed64x64Png());
    }
}

} // namespace

TEST(DefaultSkinProviderTest, InitializeWithoutResourcePackFallsBackToZeroPixels)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(provider.isInitialized());

    // 没有注入资源包，所有 18 个变体应该回退到零像素数据
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        const auto& data = provider.getSkinData(i);
        EXPECT_EQ(64u * 64u * 4u, data.size());
        bool allZero = std::all_of(data.begin(), data.end(), [](u8 b) { return b == 0; });
        EXPECT_TRUE(allZero) << "Variant " << i << " should be zero-pixel fallback";
    }
}

TEST(DefaultSkinProviderTest, InitializeWithResourcePackLoadsRealPixels)
{
    InMemoryResourcePack pack;
    populateAllDefaultSkinPngs(pack);

    DefaultSkinProvider provider;
    provider.setResourcePacks({&pack});
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    // 资源包中所有 18 个变体均为 64x64 纯红 RGBA（R=255, G=0, B=0, A=255）
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        const auto& data = provider.getSkinData(i);
        EXPECT_EQ(64u * 64u * 4u, data.size());
        // 检查首像素与末像素均为红色
        EXPECT_EQ(255, data[0]); // R
        EXPECT_EQ(0, data[1]);   // G
        EXPECT_EQ(0, data[2]);   // B
        EXPECT_EQ(255, data[3]); // A

        const size_t last = data.size() - 4;
        EXPECT_EQ(255, data[last + 0]);
        EXPECT_EQ(0, data[last + 1]);
        EXPECT_EQ(0, data[last + 2]);
        EXPECT_EQ(255, data[last + 3]);
    }
}

TEST(DefaultSkinProviderTest, InitializeWithPartialResourcePackLoadsAvailablePixels)
{
    InMemoryResourcePack pack;
    // 只注入 slim/steve（索引 6）一个变体
    const auto& variants = getDefaultSkinVariants();
    std::string path = variants[6].textureLocation().toFilePath(resource::PackType::ClientResources);
    pack.add(path, makeSolidRed64x64Png());

    DefaultSkinProvider provider;
    provider.setResourcePacks({&pack});
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    // 索引 6 应该是真实像素
    const auto& steveData = provider.getSkinData(6);
    EXPECT_EQ(255, steveData[0]);
    EXPECT_EQ(0, steveData[1]);
    EXPECT_EQ(0, steveData[2]);
    EXPECT_EQ(255, steveData[3]);

    // 其他索引应该是零像素占位
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        if (i == 6) {
            continue;
        }
        const auto& data = provider.getSkinData(i);
        EXPECT_EQ(64u * 64u * 4u, data.size());
        bool allZero = std::all_of(data.begin(), data.end(), [](u8 b) { return b == 0; });
        EXPECT_TRUE(allZero) << "Variant " << i << " should be zero-pixel fallback when not in pack";
    }
}

TEST(DefaultSkinProviderTest, SetResourcePacksReturnsInjectedList)
{
    InMemoryResourcePack pack;
    DefaultSkinProvider provider;
    provider.setResourcePacks({&pack});
    const auto& packs = provider.resourcePacks();
    ASSERT_EQ(1u, packs.size());
    EXPECT_EQ(&pack, packs[0]);
}

TEST(DefaultSkinProviderTest, MultiplePacksLaterAddedWinsByPriority)
{
    // 模拟生产场景：index 0 内存包不含 player 皮肤 PNG（查询全部 not-found），
    // index 1 磁盘包含全部 18 个变体。注入完整列表后应命中 index 1 的真实像素，
    // 而不是回退零像素。这正是本次修复的核心回归保障。
    InMemoryResourcePack lowPriorityPack;  // 模拟内存包：不注册任何皮肤
    InMemoryResourcePack highPriorityPack; // 模拟磁盘包：注册全部 18 个变体
    populateAllDefaultSkinPngs(highPriorityPack);

    DefaultSkinProvider provider;
    provider.setResourcePacks({&lowPriorityPack, &highPriorityPack});
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    // 全部 18 个变体都应来自高优先级包的红色像素，而非零像素回退
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        const auto& data = provider.getSkinData(i);
        EXPECT_EQ(64u * 64u * 4u, data.size());
        EXPECT_EQ(255, data[0]) << "Variant " << i << " R";
        EXPECT_EQ(0, data[1]) << "Variant " << i << " G";
        EXPECT_EQ(0, data[2]) << "Variant " << i << " B";
        EXPECT_EQ(255, data[3]) << "Variant " << i << " A";
    }
}

TEST(DefaultSkinProviderTest, MultiplePacksOverridePrecedenceWithinSameVariant)
{
    // 两个包都提供 slim/steve（索引 6），但颜色不同。
    // 后添加的包（高优先级）应胜出：绿色覆盖红色。
    InMemoryResourcePack redPack;
    InMemoryResourcePack greenPack;
    const auto& variants = getDefaultSkinVariants();
    std::string path = variants[6].textureLocation().toFilePath(resource::PackType::ClientResources);
    redPack.add(path, makeSolidColor64x64Png(255, 0, 0, 255));   // 红色，低优先级
    greenPack.add(path, makeSolidColor64x64Png(0, 255, 0, 255)); // 绿色，高优先级

    DefaultSkinProvider provider;
    provider.setResourcePacks({&redPack, &greenPack});
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    const auto& steveData = provider.getSkinData(6);
    EXPECT_EQ(0, steveData[0]) << "R should be overridden to green";
    EXPECT_EQ(255, steveData[1]) << "G from high-priority pack";
    EXPECT_EQ(0, steveData[2]);
    EXPECT_EQ(255, steveData[3]);
}

TEST(DefaultSkinProviderTest, GetDefaultSkinReturnsValidLocation)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    std::array<u8, 16> uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto location = provider.getDefaultSkin(uuid);
    EXPECT_EQ("minecraft", location.namespace_());
    EXPECT_TRUE(location.path().find("textures/entity/player/") == 0);
    EXPECT_TRUE(location.path().find(".png") != std::string::npos);
}

TEST(DefaultSkinProviderTest, GetDefaultSkinTypeConsistentWithVariant)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    // 验证所有 18 个 UUID 哈希值都能映射到合法的 SkinType
    for (u8 v = 0; v < DEFAULT_SKIN_COUNT; ++v) {
        std::array<u8, 16> uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, v};
        SkinType type = provider.getDefaultSkinType(uuid);
        EXPECT_TRUE(type == SkinType::Default || type == SkinType::Slim);
    }
}

TEST(DefaultSkinProviderTest, IsDefaultSkinRecognizesAllVariants)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    const auto& variants = getDefaultSkinVariants();
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        EXPECT_TRUE(provider.isDefaultSkin(variants[i].textureLocation()))
            << "Variant " << i << " should be recognized as default skin";
    }

    // 非默认皮肤路径
    ResourceLocation custom("minecraft:textures/entity/player/custom_skin.png");
    EXPECT_FALSE(provider.isDefaultSkin(custom));
}

TEST(DefaultSkinProviderTest, GetSkinLocationIndexBounds)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    // 越界索引应回退到 slim/steve（索引 6）
    auto location = provider.getSkinLocation(999);
    const auto& variants = getDefaultSkinVariants();
    EXPECT_EQ(variants[6].textureLocation(), location);
}

TEST(DefaultSkinProviderTest, GetCanonicalDefaultSkinLocation)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    auto location = provider.getCanonicalDefaultSkinLocation();
    const auto& variants = getDefaultSkinVariants();
    EXPECT_EQ(variants[6].textureLocation(), location);
}

TEST(DefaultSkinProviderTest, BackwardCompatSteveAlexAccessors)
{
    DefaultSkinProvider provider;
    auto result = provider.initialize();
    ASSERT_TRUE(result.success());

    // getSteveSkin 应该等于索引 15 (wide/steve)
    auto steveLocation = provider.getSteveSkin();
    const auto& variants = getDefaultSkinVariants();
    EXPECT_EQ(variants[15].textureLocation(), steveLocation);

    // getAlexSkin 应该等于索引 0 (slim/alex)
    auto alexLocation = provider.getAlexSkin();
    EXPECT_EQ(variants[0].textureLocation(), alexLocation);

    // getSteveSkinData / getAlexSkinData 应该返回与 getSkinData 等价的数据
    const auto& steveData = provider.getSteveSkinData();
    const auto& steveDataByIndex = provider.getSkinData(15);
    EXPECT_EQ(steveData.size(), steveDataByIndex.size());
    EXPECT_TRUE(std::equal(steveData.begin(), steveData.end(), steveDataByIndex.begin()));

    const auto& alexData = provider.getAlexSkinData();
    const auto& alexDataByIndex = provider.getSkinData(0);
    EXPECT_EQ(alexData.size(), alexDataByIndex.size());
    EXPECT_TRUE(std::equal(alexData.begin(), alexData.end(), alexDataByIndex.begin()));
}
