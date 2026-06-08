如果你的任务涉及代码修改、文档更新、编译等，必须完整阅读下面三个文件的内容：

/README.md
/docs/CODE_CONVENTIONS.md
/docs/PROJECT_CONVENTIONS.md

【重要】由于本项目代码量已达百万级，构建时间很长，任务的超时等待时间必须30分钟以上！

只允许执行下面唯一构建命令：
```bash
./scripts/configure.sh build
```

提交代码之前，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

可能出现找不到clang-format的情况，此时需要手动指定路径。（我的vs安装在D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64/bin/clang-format.exe）

【重要】不允许让子代理执行编译命令，因为多个子代理执行编译命令会导致构建系统出现大量严重问题甚至锁死，必须由你来执行编译命令！

注：当前资源包路径：C:\Users\Administrator\minecraft_reborn\resourcepacks\Vanilla
注：当前数据包路径：C:\Users\Administrator\minecraft_reborn\datapacks\Vanilla
