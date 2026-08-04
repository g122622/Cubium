使用java版1.21.11客户端连接到本项目服务端时，观察到大量bug：

1. 时间不更新，昼夜不更替，且进入世界的初始时间是0，不符合mc逻辑
2. 放置床的时候，只能放半张，另一半床不会放置出来
3. 使用燧石点击tnt没反应
4. 玩家快速移动导致区块卸载的时候，服务端有大量下面日志：
```
[2026-08-04 13:24:19.942] [error] Attempted to remove non-existent entity with ID 3892
[2026-08-04 13:24:19.942] [error] Attempted to remove non-existent entity with ID 3894
[2026-08-04 13:24:19.942] [error] Attempted to remove non-existent entity with ID 3901
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3902
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3903
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3906
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3970
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 4364
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 4235
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 4050
```
5. 客户端输入命令的时候能看到命令树的智能提示，但提交命令之后没反应
6. 红石不工作（红石火把无法激活红石粉末）
7. 右键无法放置矿车
8. 方块更新异常（例如海里有个海带柱子，我破坏了海带柱子底部的海带，正常情况下应该是上面海带替换为水，然而现实变成了替换为草方块、木板！错乱了。）
9. 流体无法流动
