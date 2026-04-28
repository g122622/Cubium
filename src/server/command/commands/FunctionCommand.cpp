#include "FunctionCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>

namespace mc {
namespace command {

void FunctionCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto functionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("function");
    functionNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        functionNode,
        support::makeMetadata(
            "Runs a function from a data pack.",
            "/function <name>",
            2,
            {},
            true));

    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "name",
        StringArgumentType::string());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return runFunction(ctx);
    });
    functionNode->addChild(nameArg);

    dispatcher.registerCommand(functionNode);
}

i32 FunctionCommand::runFunction(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const String name = context.getArgument<String>("name");

    // 解析函数命名空间
    size_t colonPos = name.find(':');
    String namespaceName = colonPos != String::npos ? name.substr(0, colonPos) : "minecraft";
    String functionName = colonPos != String::npos ? name.substr(colonPos + 1) : name;

    std::ostringstream ss;
    ss << "Running function " << namespaceName << ":" << functionName;
    source.sendMessage(ss.str());

    // TODO: 实现数据包函数系统
    // 1. 从数据包加载函数文件
    // 2. 解析函数中的命令
    // 3. 按顺序执行命令
    // 4. 处理递归调用限制

    return 1;
}

} // namespace command
} // namespace mc
