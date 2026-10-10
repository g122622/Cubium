# Play 包补全记录

分支：`codex/play-packet-completion`。基线：`1dcbd7e87`。范围为本次请求列出的 24 项；每项修复单独提交。项目以真 Java 1.21.11 客户端连接服务端为主。

| 项目 | 状态 | 验证 |
| --- | --- | --- |
| container_close（cb:17） | 已补下行登记、客户端消费、死亡前关闭；远程关闭核对菜单 ID | Windows 构建成功；11 项容器协议测试通过，含下行黄金字节测试 |
| change_game_mode、swing、teleport_to_entity、client_information、player_abilities、player_loaded | 待处理 | |
| award_stats、tab_list、damage_event、cooldown | 待处理 | |
| set_equipment、update_mob_effect | 基线已有 IR、codec、登记与广播，待核查链路 | |
| rename_item、edit_book、select_trade、set_beacon、container_button_click、bundle_item_selected | 待处理 | |
| show_dialog、clear_dialog、waypoint、server_links、entity_position_sync | 待处理 | |

统计采用 `assets/data/packet_ids.json` 与 `JavaProtocolTables.cpp` 逐项比对；完整实现要求实际收发与业务逻辑，单纯补登记或 TODO 不视为完成。
