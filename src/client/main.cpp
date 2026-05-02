#include "application/ClientApplication.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <string>

namespace {

void printBanner()
{
    std::cout << R"(
  __  __      _____           _____ ______
 |  \/  | /\  / ____|    /\   |  __ \___  /
 | \  /  /  \ | |       /  \  | |__) | / /
 | |\/| / /\ \| |      / /\ \ |  ___/ / /
 | |\/| |/ ____ \ |____ / ____ \| |   / /__
 |_|  _/_/    \_\_____/_/    \_\_|  /_____/
    | |         | |
    | |__   __ _| | _____ _ __
    | '_ \ / _` | |/ / _ \ '__|
    | |_) | (_| |   <  __/ |
    |_.__/ \__,_|_|\_\___|_|

)" << std::endl;

    std::cout << "  Minecraft Reborn Client v"
              << MC_VERSION_MAJOR << "."
              << MC_VERSION_MINOR << "."
              << MC_VERSION_PATCH << std::endl;
    std::cout << "  ========================================\n" << std::endl;
}

void printHelp()
{
    std::cout << "Usage: minecraft-client [options]\n\n"
              << "Options:\n"
              << "  -h, --help          Show this help message\n"
              << "  -w, --width <px>    Set window width (default: 1280)\n"
              << "  -H, --height <px>   Set window height (default: 720)\n"
              << "  -f, --fullscreen    Start in fullscreen mode\n"
              << "  -s, --server <addr> Server address to connect to\n"
              << "  -p, --port <port>   Server port (default: 19132)\n"
              << "  -u, --username <n>  Player username\n"
              << "  --settings <path>   Use custom options.json path\n"
              << "  -v, --verbose       Enable verbose logging\n"
              << "  --skip-integrated   Skip integrated server (for external server)\n"
              << "  --quick-play <id>   Skip main menu and load world with given level ID\n"
              << "  --quick-play-new    Skip main menu and create a new world\n"
              << std::endl;
}

} // namespace

int main(int argc, char* argv[])
{
    // 打印Banner
    printBanner();

    // 启动参数
    mc::client::ClientLaunchParams params;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            params.windowWidth = std::stoi(argv[++i]);
        }
        if ((arg == "-H" || arg == "--height") && i + 1 < argc) {
            params.windowHeight = std::stoi(argv[++i]);
        }
        if (arg == "-f" || arg == "--fullscreen") {
            params.fullscreen = true;
        }
        if ((arg == "-s" || arg == "--server") && i + 1 < argc) {
            params.serverAddress = argv[++i];
        }
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            params.serverPort = static_cast<mc::u16>(std::stoi(argv[++i]));
        }
        if ((arg == "-u" || arg == "--username") && i + 1 < argc) {
            params.username = argv[++i];
        }
        if (arg == "--settings" && i + 1 < argc) {
            params.settingsPath = argv[++i];
        }
        if (arg == "-v" || arg == "--verbose") {
            // 通过设置日志级别来启用详细日志
        }
        if (arg == "--skip-integrated") {
            params.skipIntegratedServer = true;
        }
        if (arg == "--quick-play" && i + 1 < argc) {
            params.quickPlayLevelId = argv[++i];
        }
        if (arg == "--quick-play-new") {
            params.quickPlayNew = true;
        }
    }

    try {
        // 创建客户端实例
        mc::client::ClientApplication client;

        // 初始化
        auto initResult = client.initialize(params);
        if (initResult.failed()) {
            spdlog::error("Failed to initialize client: {}", initResult.error().toString());
            return 1;
        }

        // 运行主循环
        auto runResult = client.run();
        if (runResult.failed()) {
            spdlog::error("Client error: {}", runResult.error().toString());
            return 1;
        }

        spdlog::info("Client exited successfully");
        return 0;

    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }
}
