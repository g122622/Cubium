# Minecraft Reborn

现代 Minecraft 克隆，使用 C++17 和 Vulkan 渲染，采用客户端-服务端架构。

## 构建命令

### 环境配置

```powershell
# 设置 vcpkg 环境变量
$env:VCPKG_ROOT = "D:\tools\vcpkg"

# 配置项目
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

# 若想启用perfetto性能分析：
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DMC_ENABLE_TRACING=ON

# 编译
chcp 65001 # 务必记得先执行这一行，避免中文乱码

# 注：即使在开发过程中，也要尽量使用RelWithDebInfo构建，因为Debug运行非常慢，除非必要否则不要用。
# 这行命令除了编译cpp代码之外，还会编译着色器
cmake --build build --config RelWithDebInfo
# 构建过程可能出现“cl : 命令行  error D8040: 创建子进程或与子进程通讯时出错”这种错误，此时只需要重新跑一遍构建命令就行，不用清理构建目录、不用重新生成构建脚本。

# 运行测试
# 强烈建议只运行特定测试并设置brief，运行全部测试会更慢（测试用例有几千个），且很快就会耗尽上下文，导致你无法有效地分析测试结果。
# 建议只在全部编码工作完成之后运行回归测试的时候才运行全部测试，且也要启用brief。
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter=ChunkWorkerPoolTest.* --gtest_brief=1

```

# 运行服务端
./build/bin/RelWithDebInfo/minecraft-server.exe --help

# 运行客户端
./build/bin/RelWithDebInfo/minecraft-client.exe
```

增加新的着色器之后要在shaders\CMakeLists.txt中新增文件

## 着色器编译

项目使用 Vulkan SPIR-V 着色器。如果系统未找到着色器编译器，需要手动编译：

### 方法1: 使用 Vulkan SDK 中的 glslc

确保已安装 [Vulkan SDK](https://vulkan.lunarg.com/)，然后：

```powershell
cd D:\MiscProjects\minecraft-reborn
# 编译所有着色器
glslc shaders/block.vert -o build/shaders/block.vert.spv
glslc shaders/block.frag -o build/shaders/block.frag.spv
glslc shaders/debug.vert -o build/shaders/debug.vert.spv
glslc shaders/debug.frag -o build/shaders/debug.frag.spv
```

### 方法2: 使用 CMake 自动编译

如果 Vulkan SDK 已安装并在 PATH 中，CMake 会自动检测 `glslc` 或 `glslangValidator` 并编译着色器：

```powershell
# 重新配置并编译
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config RelWithDebInfo
```

## 依赖

通过 vcpkg 管理：
- **glm** - 数学库
- **spdlog** - 日志
- **nlohmann-json** - JSON 解析
- **glfw3** - 窗口/输入
- **Vulkan** - 图形 API
- **VulkanMemoryAllocator** - GPU 内存管理
- **asio** - 网络 (异步 I/O)
- **GTest** - 测试框架
- **stb** - 图像加载
