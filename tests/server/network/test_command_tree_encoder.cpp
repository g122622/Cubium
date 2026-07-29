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

// CommandTreeEncoder 专项测试。
//
// encodeCommandTree 把 CommandTreeSnapshot 编码为 1.21.11 ClientboundCommandsPacket 包体字节
// （VarInt(nodeCount) + nodes + VarInt(rootIndex)）。本测试手搓 snapshot（不经
// CommandDispatcher），逐字节断言编码输出，覆盖：
//   - 字段顺序 flags→children→redirect→stub（children 在 name 之前）；
//   - root 节点无 stub（不写 name）；
//   - 包尾单 VarInt(rootIndex=0)；
//   - ArgumentType 数值 id（非 Identifier 字符串）+ properties：integer(min/max numFlags)、
//     entity(flags byte)、string(mode VarInt)、time(i32 min)；
//   - redirect（flags bit3 + VarInt index）；
//   - customSuggestions（flags bit4 + argument stub 内 writeIdentifier）；
//   - 未知 typeName 返回 Error。

#include "common/network/backend/java/codecs/CommandTreeEncoder.hpp"

#include "common/command/CommandTreeSnapshot.hpp"
#include "common/core/Types.hpp"
#include "server/command/CommandRegistry.hpp"

#include <gtest/gtest.h>

#include <climits>
#include <limits>
#include <vector>

using namespace mc;
using namespace mc::command;
using namespace mc::network::java::codecs;

namespace {

// 构造一个 root 节点（type=Root, 无 name, 无 children）。
CommandTreeNodeSnapshot makeRoot(std::vector<u32> children = {})
{
    CommandTreeNodeSnapshot n;
    n.type = NodeType::Root;
    n.children = std::move(children);
    return n;
}

// 构造一个 literal 节点。
CommandTreeNodeSnapshot makeLiteral(std::string name, std::vector<u32> children = {}, bool executable = false)
{
    CommandTreeNodeSnapshot n;
    n.type = NodeType::Literal;
    n.name = std::move(name);
    n.children = std::move(children);
    n.executable = executable;
    return n;
}

// 构造一个 argument 节点（properties 由调用方填 argumentProperties）。
CommandTreeNodeSnapshot makeArgument(std::string name,
    std::string typeName,
    i32 networkId,
    nlohmann::json props = nlohmann::json::object(),
    std::vector<u32> children = {},
    bool executable = false)
{
    CommandTreeNodeSnapshot n;
    n.type = NodeType::Argument;
    n.name = std::move(name);
    n.typeName = std::move(typeName);
    n.argumentNetworkId = networkId;
    n.argumentProperties = std::move(props);
    n.children = std::move(children);
    n.executable = executable;
    return n;
}

// 把字节向量格式化为 hex 串（断言失败时便于肉眼对比）。
std::string toHex(const std::vector<u8>& bytes)
{
    static const char* kHex = "0123456789ABCDEF";
    std::string s;
    s.reserve(bytes.size() * 3);
    for (u8 b : bytes) {
        s += kHex[(b >> 4) & 0x0F];
        s += kHex[b & 0x0F];
        s += ' ';
    }
    return s;
}

} // namespace

// 空树：VarInt(0) nodeCount + VarInt(0) rootIndex。
TEST(CommandTreeEncoder, EmptyTree)
{
    CommandTreeSnapshot snap;
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {0x00, 0x00};
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// 单 root：count=1, node0(flags=0,childCount=0), rootIndex=0。
TEST(CommandTreeEncoder, RootOnly)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot()};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    // 01(nodeCount=1) 00(flags=0) 00(childCount=0) 00(rootIndex=0)
    const std::vector<u8> expected = {0x01, 0x00, 0x00, 0x00};
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// root + 1 literal child("foo", executable)。
// node0: root flags=00, children=[1]
// node1: literal flags=05(0x01 literal | 0x04 executable), children=[], name "foo"
// 包尾 rootIndex=0
TEST(CommandTreeEncoder, RootWithLiteralChild)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}), makeLiteral("foo", {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x02, // nodeCount=2
        0x00,
        0x01,
        0x01, // node0: flags=0, childCount=1, child[0]=1
        0x05,
        0x00, // node1: flags=0x05, childCount=0
        0x03,
        0x66,
        0x6F,
        0x6F, // name "foo": VarInt(3) + "foo"
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// argument integer min=0 max=64：numFlags=0x03 + i32(0) + i32(64)，大端。
TEST(CommandTreeEncoder, ArgumentIntegerMinMax)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}), makeArgument("value", "integer", 3, {{"min", 0}, {"max", 64}}, {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x02, // nodeCount=2
        0x00,
        0x01,
        0x01, // node0 root: flags=0, childCount=1, child=1
        0x06,
        0x00, // node1: flags=0x06(0x02 arg|0x04 exec), childCount=0
        0x05,
        0x76,
        0x61,
        0x6C,
        0x75,
        0x65, // name "value": VarInt(5)+"value"
        0x03, // VarInt(numericId=3 integer)
        0x03, // numFlags=0x03 (hasMin|hasMax)
        0x00,
        0x00,
        0x00,
        0x00, // i32 min=0 (big-endian)
        0x00,
        0x00,
        0x00,
        0x40, // i32 max=64 (big-endian)
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// argument integer 无 min/max：numFlags=0x00，无后续。
TEST(CommandTreeEncoder, ArgumentIntegerNoBounds)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}), makeArgument("n", "integer", 3, {}, {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x02,
        0x00,
        0x01,
        0x01, // node0 root
        0x06,
        0x00, // node1: flags=0x06, childCount=0
        0x01,
        0x6E, // name "n": VarInt(1)+"n"
        0x03, // numericId=3 integer
        0x00, // numFlags=0x00
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// argument integer 带默认哨兵值（min=INT32_MIN, max=INT32_MAX）：
// 复现生产路径——IntegerArgumentType::serializeMetadata 无条件写 min/max，但 vanilla
// IntegerArgumentInfo 仅在非默认值时写。编码器须按哨兵 gating，输出与无界一致（numFlags=0x00）。
// 回归保护：此前此处误写 numFlags=0x03 + 8 字节，致真 Java 客户端游标错位崩溃
// （disconnect readerIndex(10043)+1>writerIndex(10043)）。
TEST(CommandTreeEncoder, ArgumentIntegerDefaultSentinelTreatedAsUnbounded)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}), makeArgument("n", "integer", 3, {{"min", INT32_MIN}, {"max", INT32_MAX}}, {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x02,
        0x00,
        0x01,
        0x01, // node0 root
        0x06,
        0x00, // node1: flags=0x06, childCount=0
        0x01,
        0x6E, // name "n": VarInt(1)+"n"
        0x03, // numericId=3 integer
        0x00, // numFlags=0x00（哨兵 gating，不写 min/max）
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// argument integer 仅设下界（min=0, max=INT32_MAX）：numFlags=0x01 + i32(0)，不写 max。
TEST(CommandTreeEncoder, ArgumentIntegerMinOnly)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}), makeArgument("n", "integer", 3, {{"min", 0}, {"max", INT32_MAX}}, {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x02,
        0x00,
        0x01,
        0x01, // node0 root
        0x06,
        0x00, // node1: flags=0x06, childCount=0
        0x01,
        0x6E, // name "n": VarInt(1)+"n"
        0x03, // numericId=3 integer
        0x01, // numFlags=0x01 (hasMin only)
        0x00,
        0x00,
        0x00,
        0x00, // i32 min=0 (big-endian)
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// argument float 带默认哨兵值（min=-FLT_MAX, max=FLT_MAX）：numFlags=0x00，无后续。
// 同 integer 哨兵 gating 回归保护。
TEST(CommandTreeEncoder, ArgumentFloatDefaultSentinelTreatedAsUnbounded)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}),
        makeArgument("f",
            "float",
            1,
            {{"min", -std::numeric_limits<float>::max()}, {"max", std::numeric_limits<float>::max()}},
            {},
            true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x02,
        0x00,
        0x01,
        0x01, // node0 root
        0x06,
        0x00, // node1: flags=0x06, childCount=0
        0x01,
        0x66, // name "f": VarInt(1)+"f"
        0x01, // numericId=1 float
        0x00, // numFlags=0x00（哨兵 gating，不写 min/max）
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// entity flags byte：player→0x03, players→0x02, entity→0x01, entities→0x00。
// 用完整字节向量断言（避免手算偏移出错）。
TEST(CommandTreeEncoder, ArgumentEntityFlags)
{
    const struct Case {
        std::string typeName;
        bool single;
        bool playersOnly;
        u8 expectedEntityFlags;
    } cases[] = {
        {"player", true, true, 0x03},
        {"players", false, true, 0x02},
        {"entity", true, false, 0x01},
        {"entities", false, false, 0x00},
    };

    for (const auto& c : cases) {
        CommandTreeSnapshot snap;
        snap.nodes = {makeRoot({1}),
            makeArgument("t", c.typeName, 6, {{"single", c.single}, {"playersOnly", c.playersOnly}}, {}, true)};
        auto r = encodeCommandTree(snap);
        ASSERT_TRUE(r.success()) << c.typeName << ": " << r.error().toString();
        // node1: flags=0x06(arg|exec), childCount=0, name "t"(VarInt(1)+"t"), numericId=6, entityFlags
        const std::vector<u8> expected = {
            0x02, // nodeCount=2
            0x00,
            0x01,
            0x01, // node0 root: flags=0, childCount=1, child=1
            0x06,
            0x00, // node1: flags=0x06, childCount=0
            0x01,
            0x74,                  // name "t": VarInt(1)+"t"
            0x06,                  // numericId=6 entity
            c.expectedEntityFlags, // entity flags byte
            0x00                   // rootIndex=0
        };
        EXPECT_EQ(r.value(), expected) << c.typeName << " actual: " << toHex(r.value());
    }
}

// string mode：word→VarInt(0), phrase→VarInt(1), greedy_string→VarInt(2)。
TEST(CommandTreeEncoder, ArgumentStringMode)
{
    const struct Case {
        std::string typeName;
        i32 expectedMode;
    } cases[] = {
        {"word", 0},
        {"phrase", 1},
        {"greedy_string", 2},
    };

    for (const auto& c : cases) {
        CommandTreeSnapshot snap;
        snap.nodes = {makeRoot({1}), makeArgument("n", c.typeName, 5, {}, {}, true)};
        auto r = encodeCommandTree(snap);
        ASSERT_TRUE(r.success()) << c.typeName << ": " << r.error().toString();
        // node1: flags=0x06, childCount=0, name "n"(VarInt(1)+"n"), numericId=5, modeVarInt
        const std::vector<u8> expected = {
            0x02, // nodeCount=2
            0x00,
            0x01,
            0x01, // node0 root
            0x06,
            0x00, // node1: flags=0x06, childCount=0
            0x01,
            0x6E,                            // name "n": VarInt(1)+"n"
            0x05,                            // numericId=5 string
            static_cast<u8>(c.expectedMode), // mode VarInt
            0x00                             // rootIndex=0
        };
        EXPECT_EQ(r.value(), expected) << c.typeName << " actual: " << toHex(r.value());
    }
}

// time：i32 min（min=0 也要写）。
TEST(CommandTreeEncoder, ArgumentTime)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1}), makeArgument("t", "time", 43, {{"min", 0}}, {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    // node1: flags=0x06, childCount=0, name "t"(VarInt(1)+"t"), numericId=43(0x2B), i32 min=0(大端 4 字节)
    const std::vector<u8> expected = {
        0x02, // nodeCount=2
        0x00,
        0x01,
        0x01, // node0 root
        0x06,
        0x00, // node1: flags=0x06, childCount=0
        0x01,
        0x74, // name "t": VarInt(1)+"t"
        0x2B, // numericId=43 time
        0x00,
        0x00,
        0x00,
        0x00, // i32 min=0 (big-endian)
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// redirect：root + literal "tp" + literal "teleport"(redirect→tp, index=1)。
// "teleport" 节点 flags 含 0x08，stub 在 children 之后写 VarInt(redirect=1)。
TEST(CommandTreeEncoder, Redirect)
{
    CommandTreeSnapshot snap;
    auto tp = makeLiteral("tp", {}, true);
    auto teleport = makeLiteral("teleport", {}, true);
    teleport.redirect = 1u; // 重定向到 tp（node index 1）
    snap.nodes = {makeRoot({1, 2}), std::move(tp), std::move(teleport)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x03, // nodeCount=3
        0x00,
        0x02,
        0x01,
        0x02, // node0 root: flags=0, childCount=2, child=1, child=2
        0x05,
        0x00, // node1 "tp": flags=0x05(literal|exec), childCount=0
        0x02,
        0x74,
        0x70, // name "tp": VarInt(2)+"tp"
        0x0D,
        0x00, // node2 "teleport": flags=0x0D(literal|exec|redirect), childCount=0
        0x01, // redirect index=1
        0x08,
        0x74,
        0x65,
        0x6C,
        0x65,
        0x70,
        0x6F,
        0x72,
        0x74, // name "teleport": VarInt(8)+"teleport"
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// customSuggestions：argument "dim"(dimension, hasCustomSuggestions) → flags 含 0x10，
// stub 在 serializeCap 后写 writeIdentifier("minecraft:ask_server")。
TEST(CommandTreeEncoder, CustomSuggestions)
{
    CommandTreeSnapshot snap;
    auto dim = makeArgument("dim", "dimension", 41, {}, {}, true);
    dim.suggestionProviderId = "minecraft:ask_server";
    snap.nodes = {makeRoot({1}), std::move(dim)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    // node1: flags=0x16(arg|exec|suggestions), childCount=0, name "dim"(VarInt(3)+"dim"),
    //        numericId=41(0x29), suggestionId writeString("minecraft:ask_server")=VarInt(20)+20bytes
    const std::string idStr = "minecraft:ask_server";
    std::vector<u8> expected = {
        0x02, // nodeCount=2
        0x00,
        0x01,
        0x01, // node0 root
        0x16,
        0x00, // node1: flags=0x16, childCount=0
        0x03,
        0x64,
        0x69,
        0x6D,                          // name "dim": VarInt(3)+"dim"
        0x29,                          // numericId=41 dimension
        static_cast<u8>(idStr.size()), // suggestionId length=20 (0x14)
    };
    for (char ch : idStr) {
        expected.push_back(static_cast<u8>(ch));
    }
    expected.push_back(0x00); // rootIndex=0
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// children 顺序：root 有 2 literal children，children 列表顺序与 snapshot.children 一致。
TEST(CommandTreeEncoder, ChildrenOrder)
{
    CommandTreeSnapshot snap;
    snap.nodes = {makeRoot({1, 2}), makeLiteral("a", {}, true), makeLiteral("b", {}, true)};
    auto r = encodeCommandTree(snap);
    ASSERT_TRUE(r.success()) << r.error().toString();
    const std::vector<u8> expected = {
        0x03, // nodeCount=3
        0x00,
        0x02,
        0x01,
        0x02, // node0 root: flags=0, childCount=2, child=1, child=2
        0x05,
        0x00,
        0x01,
        0x61, // node1 "a": flags=0x05, childCount=0, name "a"
        0x05,
        0x00,
        0x01,
        0x62, // node2 "b": flags=0x05, childCount=0, name "b"
        0x00  // rootIndex=0
    };
    EXPECT_EQ(r.value(), expected) << "actual: " << toHex(r.value());
}

// 未知 typeName（如 "enum"）→ argumentNetworkId 为 nullopt → encodeCommandTree 返回 Error。
TEST(CommandTreeEncoder, UnknownTypeNameReturnsError)
{
    CommandTreeSnapshot snap;
    // argumentNetworkId 不设置（模拟 typeName="enum" 未映射）。
    CommandTreeNodeSnapshot n;
    n.type = NodeType::Argument;
    n.name = "x";
    n.typeName = "enum";
    // argumentNetworkId 故意留 nullopt
    n.executable = true;
    snap.nodes = {makeRoot({1}), std::move(n)};
    auto r = encodeCommandTree(snap);
    EXPECT_FALSE(r.success()) << "未知 typeName 应返回 Error, actual: " << toHex(r.value());
}

// 端到端：用真实 CommandRegistry（构造即 registerDefaults 注册全部 72 条命令）生成快照并编码。
// 验证生产路径 buildCommandTreeSnapshot → encodeCommandTree 全程无未映射 typeName 的 Error，
// 且编码产物非空。这是真 1.21.11 Java 客户端 clientbound/minecraft:commands 包的实际来源。
// 回归保护：此前 integer/float 边界默认值未 gating，每个无界 integer 多写 8 字节，致客户端
// IndexOutOfBoundsException 崩溃（disconnect readerIndex(10043)+1>writerIndex(10043)）。
TEST(CommandTreeEncoder, RealRegistryEncodesWithoutError)
{
    CommandRegistry registry; // 构造即 registerDefaults()，注册全部默认命令
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 真实命令树应含数十个节点（root + 72 命令各自的 literal/argument 子树）。
    ASSERT_GT(snapshot.nodes.size(), 50u) << "真实命令树节点数异常少";

    auto encoded = encodeCommandTree(snapshot);
    ASSERT_TRUE(encoded.success()) << "真实命令树编码失败（存在未映射 typeName）: " << encoded.error().toString();

    const auto& bytes = encoded.value();
    EXPECT_FALSE(bytes.empty()) << "编码产物不应为空";

    // 线格式自洽性：首字节是 VarInt nodeCount（与快照节点数一致），末字节是 rootIndex VarInt(0)。
    // nodeCount>127 时 VarInt 占多字节，故仅断言末字节 rootIndex=0（snapshot.nodes[0] 即 root）。
    EXPECT_EQ(bytes.back(), 0x00) << "包尾 rootIndex 应为 0";

    // 至少应有一个 argument 节点（如 time/gamemode/entity 等），证明 ArgumentType 编码路径被覆盖。
    bool hasArgument = false;
    for (const auto& node : snapshot.nodes) {
        if (node.type == NodeType::Argument) {
            hasArgument = true;
            // 每个 argument 节点都必须有 argumentNetworkId（否则真实树就会报 Error，已被上面断言）。
            EXPECT_TRUE(node.argumentNetworkId.has_value())
                << "argument 节点 '" << node.name << "' (typeName='" << node.typeName << "') 缺少 networkId";
        }
    }
    EXPECT_TRUE(hasArgument) << "真实命令树应至少含一个 argument 节点";
}
