如果你的任务涉及代码修改、文档更新、编译等，必须完整阅读下面三个文件的内容：

/README.md
/docs/CODE_CONVENTIONS.md
/docs/PROJECT_CONVENTIONS.md

【重要】在windows平台上，构建只能使用 cmake --build --preset windows-clang-relwithdebinfo 这个命令，其他构建方式都不允许使用。由于本项目代码量已达百万级，构建时间很长，任务的超时等待时间必须30分钟以上！

提交代码之前，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

如果遇到了 vcpkg 构建失败，按照下面命令解决：

手动执行 CMake configure，**关闭 vcpkg manifest install**：

```powershell
cmake .. -DVCPKG_MANIFEST_INSTALL=OFF -G "Ninja Multi-Config"
```

这跳过了 vcpkg install 步骤，CMake 成功完成配置并重新生成了 ninja 构建文件。之后正常构建即可：

```powershell
cmake --build --preset windows-clang-relwithdebinfo
```
