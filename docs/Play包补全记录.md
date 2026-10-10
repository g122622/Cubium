# Play 包补全记录

分支：`codex/play-packet-completion`。基线：`1dcbd7e87`。范围为本次请求列出的 24 项；每项修复单独提交。项目以真 Java 1.21.11 客户端连接服务端为主。

| 项目 | 状态 | 验证 |
| --- | --- | --- |
| container_close（cb:17） | 已补下行登记、客户端消费、死亡前关闭；远程关闭核对菜单 ID | Windows 构建成功；11 项容器协议测试通过，含下行黄金字节测试 |
| change_game_mode（sb:4） | 已补协议、权限校验及路由；所有模式切换均更新真实实体 | Windows 构建成功；3 项协议与业务测试通过 |
| player_loaded（sb:43） | 已接加载计时器，保留等待重生守卫 | Windows 构建成功；加载状态与路由测试通过 |
| swing（sb:60） | 已接真实实体挥手与既有动画广播，重置主动操作时间 | Windows 构建成功；双手、非法枚举及实体广播测试通过 |
| player_abilities（sb:39） | 独立的单字节上行 codec，仅更新服务端授权的飞行状态 | Windows 构建成功；线格式、截断输入及飞行越权测试通过 |
| client_information（sb:13） | 复用配置负载校验；应用惯用手、皮肤、聊天及玩家列表偏好；每玩家区块追踪视距；初次配置、重配置和跨维度保留 | Windows 构建成功；协议、元数据、列表及视距独立性测试通过 |
| teleport_to_entity（sb:61） | 旁观者按 UUID 查找目标；同维度移动与跨维度迁移复用已有链路，并启动传送确认 | Windows 构建成功；UUID 黄金字节、全部截断长度、旁观者权限及传送确认测试通过 |
| tab_list（cb:120） | 完整 Component NBT codec 与 ServerPlayer 富文本发送/清空接口 | 已补黄金字节、截断输入和连接发送测试；随下行簇集中构建 |
| cooldown（cb:22） | 按资源标识符追踪冷却组，设置/到期/移除自动通知客户端；原生客户端消费已接 | 已补黄金字节、连接发送、组共享、到期和最大持续时间溢出回归；随下行簇集中构建 |
| damage_event（cb:25） | 接入有效新伤害广播；追踪者与受害玩家本人收到类型、责任/直接来源和显式位置；类型 ID 与配置注册序共用 | 已补黄金字节、截断、注册序、无敌帧及本人未被追踪时的发送回归；随下行簇集中构建 |
| award_stats（cb:3） | 九类统计 ID 映射及 77 项自定义统计；完整/增量发送、重置零值与请求回复 | 已补黄金字节、计数边界、注册表类型、脏标记独立性、发送失败与真实请求路由测试；随下行簇集中构建 |
| set_equipment（cb:100） | 修复卸装空槽未同步与新追踪者缺初始快照；补鞍槽及输入校验 | 已补黄金字节、新追踪装备与卸装清槽回归；随下行簇集中构建 |
| update_mob_effect | 基线已有 IR、codec、登记与广播，待核查链路 | |
| rename_item、edit_book、select_trade、set_beacon、container_button_click、bundle_item_selected | 待处理 | |
| show_dialog、clear_dialog、waypoint、server_links、entity_position_sync | 待处理 | |

统计采用 `assets/data/packet_ids.json` 与 `JavaProtocolTables.cpp` 逐项比对；完整实现要求实际收发与业务逻辑，单纯补登记或 TODO 不视为完成。

2026-10-11：按请求将 ServerPlayHandler::route 改为 switch。case 标签从包类型编译期推导，禁止硬编码变体下标；类型缺失或重复触发编译错误。Windows 构建成功，10 项玩家状态协议与路由测试全部通过。

2026-10-11：玩家状态簇及 UUID 修复重新构建成功；237 项 Play 协议、配置设置、玩家状态路由、区块视距和 UUID 测试全部通过。六个请求的玩家状态上行包均完成；后续仍按包独立提交。

统计窗口前置链路：额外登记 client_command（sb:11）并实现 REQUEST_STATS。PERFORM_RESPAWN 的完整重生链路不在本次列出的 24 项中，仍明确标为 TODO，不计入完整实现。
