#include <gtest/gtest.h>

#include "client/command/ClientCommandManager.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "common/network/packet/CommandTreePacket.hpp"

using namespace mc;
using namespace mc::command;
using namespace mc::client::command;
using namespace mc::network;

namespace {

CommandTreeSnapshot buildSnapshot()
{
    CommandTreeSnapshot snapshot;
    snapshot.nodes.resize(5);

    snapshot.nodes[0].id = 0;
    snapshot.nodes[0].type = NodeType::Root;
    snapshot.nodes[0].typeName = "root";
    snapshot.nodes[0].children = {1, 3};

    snapshot.nodes[1].id = 1;
    snapshot.nodes[1].type = NodeType::Literal;
    snapshot.nodes[1].name = "say";
    snapshot.nodes[1].typeName = "say";
    snapshot.nodes[1].children = {2};

    snapshot.nodes[2].id = 2;
    snapshot.nodes[2].type = NodeType::Argument;
    snapshot.nodes[2].name = "message";
    snapshot.nodes[2].typeName = "message";
    snapshot.nodes[2].executable = true;
    snapshot.nodes[2].suggestionKind = CommandTreeSuggestionKind::Fixed;
    snapshot.nodes[2].suggestionCandidates = {"hello", "world"};

    snapshot.nodes[3].id = 3;
    snapshot.nodes[3].type = NodeType::Literal;
    snapshot.nodes[3].name = "tell";
    snapshot.nodes[3].typeName = "tell";
    snapshot.nodes[3].children = {4};

    snapshot.nodes[4].id = 4;
    snapshot.nodes[4].type = NodeType::Argument;
    snapshot.nodes[4].name = "target";
    snapshot.nodes[4].typeName = "player";
    snapshot.nodes[4].executable = true;
    snapshot.nodes[4].suggestionKind = CommandTreeSuggestionKind::PlayerNames;

    return snapshot;
}

} // namespace

TEST(ClientCommandManagerTest, CommandTreePacketRoundTrip)
{
    const auto snapshot = buildSnapshot();
    const auto json = snapshot.toJsonString();

    CommandTreePacket packet(json);
    auto serializedResult = packet.serialize();
    ASSERT_TRUE(serializedResult.success());

    const auto& serialized = serializedResult.value();
    ASSERT_EQ(serialized.size(), json.size() + sizeof(u16));

    const u16 encodedLength =
        static_cast<u16>((static_cast<u16>(serialized[0]) << 8) | static_cast<u16>(serialized[1]));
    EXPECT_EQ(encodedLength, json.size());

    const std::string encodedJson(reinterpret_cast<const char*>(serialized.data() + sizeof(u16)), encodedLength);
    EXPECT_EQ(encodedJson, json);

    CommandTreePacket decodedPacket;
    auto deserializeResult = decodedPacket.deserialize(serialized.data(), serialized.size());
    ASSERT_TRUE(deserializeResult.success());
    EXPECT_EQ(decodedPacket.treeJson(), json);
}

TEST(ClientCommandManagerTest, AppliesTreeAndBuildsSuggestions)
{
    ClientCommandManager manager;
    manager.setPlayerNameProvider([]() { return std::vector<std::string>{"Steve", "Alex"}; });

    const auto snapshot = buildSnapshot();
    auto applyResult = manager.applyCommandTreeJson(snapshot.toJsonString());
    ASSERT_TRUE(applyResult.success());
    ASSERT_TRUE(manager.hasCommandTree());

    const auto commandNames = manager.getCommandNames();
    ASSERT_EQ(commandNames.size(), 2u);
    EXPECT_EQ(commandNames[0], "say");
    EXPECT_EQ(commandNames[1], "tell");

    const auto commandSuggestions = manager.getSuggestions("/sa", 3);
    ASSERT_FALSE(commandSuggestions.isEmpty());
    ASSERT_GE(commandSuggestions.getList().size(), 1u);
    EXPECT_EQ(commandSuggestions.getList().front().getText(), "say");

    const auto fixedSuggestions = manager.getSuggestions("/say h", 6);
    ASSERT_FALSE(fixedSuggestions.isEmpty());
    ASSERT_GE(fixedSuggestions.getList().size(), 1u);
    EXPECT_EQ(fixedSuggestions.getList().front().getText(), "hello");

    const auto playerSuggestions = manager.getSuggestions("/tell A", 7);
    ASSERT_FALSE(playerSuggestions.isEmpty());
    ASSERT_GE(playerSuggestions.getList().size(), 1u);
    EXPECT_EQ(playerSuggestions.getList().front().getText(), "Alex");
}
