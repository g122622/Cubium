# common 模块

## 目录结构树

```text
common/
├── advancement/  # 该领域的子模块
├── application/  # 进程引导与日志
├── command/  # 命令语法树、解析与建议
├── core/  # 共享基础类型、常量与错误处理
├── entity/  # 实体状态与行为
├── item/  # 物品、组件、配方和掉落
├── mod/  # 该领域的子模块
├── network/  # IR、编解码与传输管线
├── particle/  # 该领域的子模块
├── pch/  # 该领域的子模块
├── physics/  # 碰撞与几何运算
├── profiler/  # 该领域的子模块
├── registry/  # 共享注册设施
├── resource/  # 资源包与数据包读取
├── scoreboard/  # 该领域的子模块
├── screen/  # 该领域的子模块
├── skin/  # 该领域的子模块
├── sound/  # 声音标识、类别与唱片机
├── stats/  # 该领域的子模块
├── util/  # 数学、并发、序列化与通用工具
├── world/  # 世界接口、方块、流体与共享规则
├── CMakeLists.txt  # 模块说明与构建配置
├── Makefile  # 模块说明与构建配置
└── cmake_install.cmake  # 模块说明与构建配置
```

## 内部模块关系

各文件承接目录树中标明的单一职责，通过同领域接口组合。独占的辅助实现位于使用者目录下。

## 上下游外部依赖关系

公共层提供领域类型与接口；服务端通过应用装配连接玩家、网络和世界实现。源文件的构建归属与领域目录一致。

## 容易踩的坑

- 目录按领域职责组织；跨领域协作通过明确入口和基础类型。
- 新增或迁移源文件时同步更新所有消费目标的 CMake 清单。

1. **命名空间**: 所有类型在 `mc` 命名空间下
   ```cpp
   mc::BlockPos pos;      // 正确
   BlockPos pos;          // 错误
   ```

2. **智能指针**: 实体和方块使用原始指针或引用，所有权由管理器控制
   ```cpp
   Block* block = VanillaBlocks::STONE;  // 静态指针，无需释放
   ```

3. **错误处理**: 使用 `Result<T>` 而非异常
   ```cpp
   Result<int> divide(int a, int b) {
       if (b == 0) return Error(ErrorCode::InvalidArgument, "Division by zero");
       return a / b;
   }
   ```

4. **区块坐标 vs 方块坐标**:
   ```cpp
   ChunkCoord cx = pos.x >> 4;  // 方块坐标转区块坐标
   BlockCoord bx = pos.x & 15;  // 区块内方块坐标
   ```

5. **日志级别**: 项目使用 `spdlog`，默认只输出 `info` 及以上级别，`debug` 日志不可见

6. **资源定位符**: 使用 `minecraft:` 命名空间
   ```cpp
   ResourceLocation loc("minecraft:stone");  // 正确
   ResourceLocation loc("stone");             // 也可以，自动补全
   ```

7. **流体系统**: 参考 `world/fluid/FLUID_TODO.md`

8. **性能追踪**: Perfetto + Tracy 双轨，默认均启用。可用 `-DMC_ENABLE_TRACING=OFF` / `-DMC_ENABLE_TRACY=OFF` 独立关闭。详见 `profiler/README.md`。
