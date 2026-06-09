如果你的任务涉及代码修改、文档更新、编译等，必须完整阅读下面三个文件的内容：

/README.md
/docs/CODE_CONVENTIONS.md
/docs/PROJECT_CONVENTIONS.md

【重要】构建命令只能使用 `cmake --build --preset windows-clang-relwithdebinfo`。由于本项目代码量已达百万级，构建时间很长，任务的超时等待时间必须30分钟以上！

如果构建过程中遇到工具链问题、头文件找不到等问题，按照下面指引尝试先进行CMake configure，然后再重新构建：

在windows平台上，CMake configure 必须使用 `configure.bat`（或 `configure.sh` / `configure.ps1`），这些脚本会自动注入 Visual Studio 开发环境变量，解决 vcpkg 找不到 VS 的问题。

首次 configure 或清理后重新 configure（脚本位于 `scripts/` 目录下）：
```bash
# Git Bash / Claude Code 中：
./scripts/configure.sh
# 或直接调用 bat：
cmd //c scripts\configure.bat

# PowerShell 中：
.\scripts\configure.ps1

# CMD 中：
scripts\configure.bat
```

其实上述方法可能也不行，此时可以试试 configure + build 一步完成，大概率能成功：
```bash
./scripts/configure.sh build
# 或
cmd //c scripts\configure.bat build
```

提交代码之前，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

可能出现找不到clang-format的情况，此时需要手动指定路径。（我的vs安装在D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\）

【重要】不允许让子代理执行编译命令，因为多个子代理执行编译命令会导致构建系统出现大量严重问题甚至锁死，必须由你来执行编译命令！

注：当前资源包路径：C:\Users\Administrator\minecraft_reborn\resourcepacks\Vanilla
注：当前数据包路径：C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla
