#游戏事件系统 - 服务端实现

本目录包含游戏事件系统中依赖服务端（`server`）模块的实现文件。

        ##目录说明

            由于 `src /
        common /` 不得依赖 `src / server /`（架构分层约束），所有头文件（`.hpp`）位于
`src / common / world /
        gameevent /`，而需要引用 `ServerWorld`、`ServerChunkManager` 等服务端类型的 实现文件（`.cpp`）则放置在本目录中。

                   ##文件列表

    | 文件 | 说明 | | -- -- --| -- -- --|
    | `PositionSource.cpp` | `EntityPositionSource::getPosition()` 实现（需查询服务端实体管理器） |
    | `GameEventListenerRegistry.cpp` | `EuclideanGameEventListenerRegistry` 实现（需查询服务端世界坐标） |
    | `GameEventDispatcher.cpp` | `GameEventDispatcher` 实现（需访问服务端区块管理器） |
    | `DynamicGameEventListener.cpp` | `DynamicGameEventListener` 实现（需访问服务端区块管理器） |
    | `VibrationSystem.cpp` | `VibrationSystem::Ticker / Listener / User` 实现（需访问服务端世界和玩家状态） |

    ##依赖关系

``` common / world / gameevent/*.hpp  (接口与类型定义，仅前向声明 ServerWorld)
        ↑
server/world/gameevent/*.cpp  (实现，include ServerWorld 等服务端头文件)
```
