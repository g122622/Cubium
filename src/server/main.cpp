#include "application/StandaloneServer.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <csignal>

namespace {
std::atomic<bool> g_shouldExit{false};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        spdlog::info("Received shutdown signal");
        g_shouldExit = true;
    }
}

void printBanner()
{
    std::cout << R"(
  __  __      _____          _____ ______
 |  \/  |/\  / ____|   /\   |  __ \___  /
 | \  / /  \ | |       /  \  | |__) | / /
 | |\/| / /\ \| |      / /\ \ |  ___/ / /
 | |  |/ ____ \ |____ / ____ \| |   / /__
 |_|  _/_/    \_\_____/_/    \_\_|  /_____/
    | |         | |
    | |__   __ _| | _____ _ __
    | '_ \ / _` | |/ / _ \ '__|
    | |_) | (_| |   <  __/ |
    |_.__/ \__,_|_|\_\___|_|

)" << std::endl;

    std::cout << "  Minecraft Reborn Server v"
              << MC_VERSION_MAJOR << "."
              << MC_VERSION_MINOR << "."
              << MC_VERSION_PATCH << std::endl;
    std::cout << "  ========================================\n" << std::endl;
}

void printHelp()
{
    std::cout << "Usage: minecraft-server [options]\n\n"
              << "Options:\n"
              << "  -h, --help          Show this help message\n"
              << "  -p, --port <port>   Set server port (default: 19132)\n"
              << "  -n, --name <name>   Set server/world name (default: world)\n"
              << "  -s, --seed <seed>   Set world seed (default: random)\n"
              << "  --settings <path>   Use custom server.json path\n"
              << "  -m, --max <count>   Set max players (default: 20)\n"
              << "  -v, --verbose       Enable verbose logging (debug level)\n"
              << std::endl;
}
} // namespace

int main(int argc, char* argv[])
{
    // 设置信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 打印Banner
    printBanner();

    // 启动参数
    mc::server::StandaloneServerParams params;
    bool verboseLogging = false;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            params.port = static_cast<mc::u16>(std::stoi(argv[++i]));
        }
        if ((arg == "-n" || arg == "--name") && i + 1 < argc) {
            params.worldName = argv[++i];
        }
        if ((arg == "-s" || arg == "--seed") && i + 1 < argc) {
            params.seed = std::stoll(argv[++i]);
        }
        if ((arg == "-m" || arg == "--max") && i + 1 < argc) {
            params.maxPlayers = static_cast<mc::u32>(std::stoi(argv[++i]));
        }
        if (arg == "--settings" && i + 1 < argc) {
            params.settingsPath = argv[++i];
        }
        if (arg == "-v" || arg == "--verbose") {
            verboseLogging = true;
        }
    }

    // 设置日志级别
    if (verboseLogging) {
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Verbose logging enabled");
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    try {
        // 创建服务端实例
        mc::server::StandaloneServer server;

        // 初始化
        auto initResult = server.initialize(params);
        if (initResult.failed()) {
            spdlog::error("Failed to initialize server: {}", initResult.error().toString());
            return 1;
        }

        // 在单独线程中运行服务端
        std::thread serverThread([&server]() {
            auto runResult = server.run();
            if (runResult.failed()) {
                spdlog::error("Server error: {}", runResult.error().toString());
            }
        });

        // 等待退出信号
        while (!g_shouldExit && server.isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 停止服务端
        server.stop();

        // 等待服务端线程结束
        if (serverThread.joinable()) {
            serverThread.join();
        }

        spdlog::info("Server exited successfully");
        return 0;

    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }
}
