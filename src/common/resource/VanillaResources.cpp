/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "VanillaResources.hpp"
#include "common/resource/pack/InMemoryResourcePack.hpp"
#include <cstring>
#include <memory>
#include <string>

namespace mc::resource {

// ============================================================================
// 模型模板定义
// ============================================================================

// 单面纹理方块模型（所有面使用同一纹理）
const char* VanillaResources::MODEL_CUBE_ALL = R"({
    "parent": "block/cube",
    "textures": {
        "particle": "#all",
        "down": "#all",
        "up": "#all",
        "north": "#all",
        "east": "#all",
        "south": "#all",
        "west": "#all"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#down", "cullface": "down"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#up", "cullface": "up"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#north", "cullface": "north"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#south", "cullface": "south"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#west", "cullface": "west"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#east", "cullface": "east"}
            }
        }
    ]
})";

// 柱状方块模型（上下和侧面不同纹理）
const char* VanillaResources::MODEL_CUBE_COLUMN = R"({
    "parent": "block/cube",
    "textures": {
        "particle": "#side",
        "down": "#end",
        "up": "#end",
        "north": "#side",
        "east": "#side",
        "south": "#side",
        "west": "#side"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#down", "cullface": "down"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#up", "cullface": "up"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#north", "cullface": "north"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#south", "cullface": "south"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#west", "cullface": "west"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#east", "cullface": "east"}
            }
        }
    ]
})";

// 六面不同纹理的基础方块模型
const char* VanillaResources::MODEL_CUBE = R"({
    "textures": {
        "particle": "#north"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#down", "cullface": "down"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#up", "cullface": "up"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#north", "cullface": "north"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#south", "cullface": "south"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#west", "cullface": "west"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#east", "cullface": "east"}
            }
        }
    ]
})";

// 树叶模型
const char* VanillaResources::MODEL_LEAVES = R"({
    "textures": {
        "particle": "#all"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#all", "tintindex": 0},
                "up": {"uv": [0, 0, 16, 16], "texture": "#all", "tintindex": 0},
                "north": {"uv": [0, 0, 16, 16], "texture": "#all", "tintindex": 0},
                "south": {"uv": [0, 0, 16, 16], "texture": "#all", "tintindex": 0},
                "west": {"uv": [0, 0, 16, 16], "texture": "#all", "tintindex": 0},
                "east": {"uv": [0, 0, 16, 16], "texture": "#all", "tintindex": 0}
            }
        }
    ]
})";

// 交叉纹理模型
const char* VanillaResources::MODEL_CROSS = R"({
    "textures": {
        "particle": "#cross"
    },
    "elements": [
        {
            "from": [0.8, 0, 8],
            "to": [15.2, 16, 8],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "north": {"uv": [0, 0, 16, 16], "texture": "#cross"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#cross"}
            }
        },
        {
            "from": [8, 0, 0.8],
            "to": [8, 16, 15.2],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "west": {"uv": [0, 0, 16, 16], "texture": "#cross"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#cross"}
            }
        }
    ]
})";

// 染色交叉纹理模型
const char* VanillaResources::MODEL_TINTED_CROSS = R"({
    "textures": {
        "particle": "#cross"
    },
    "elements": [
        {
            "from": [0.8, 0, 8],
            "to": [15.2, 16, 8],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "north": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0},
                "south": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0}
            }
        },
        {
            "from": [8, 0, 0.8],
            "to": [8, 16, 15.2],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "west": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0},
                "east": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0}
            }
        }
    ]
})";

// 空气模型（不可见）
const char* VanillaResources::MODEL_AIR = R"({
    "textures": {},
    "elements": []
})";

// ============================================================================
// 创建资源包
// ============================================================================

std::unique_ptr<InMemoryResourcePack> VanillaResources::createResourcePack()
{
    auto pack = std::make_unique<InMemoryResourcePack>("vanilla");

    _registerBaseModels(*pack);
    _registerBlockStates(*pack);

    return pack;
}

// ============================================================================
// 注册基础模型
// ============================================================================

void VanillaResources::_registerBaseModels(InMemoryResourcePack& pack)
{
    // 基础方块模型（无父模型）
    pack.addClientResource("minecraft/models/block/cube.json", MODEL_CUBE);

    // 单面纹理方块
    pack.addClientResource("minecraft/models/block/cube_all.json", MODEL_CUBE_ALL);

    // 柱状方块
    pack.addClientResource("minecraft/models/block/cube_column.json", MODEL_CUBE_COLUMN);

    // 树叶
    pack.addClientResource("minecraft/models/block/leaves.json", MODEL_LEAVES);

    // 交叉纹理
    pack.addClientResource("minecraft/models/block/cross.json", MODEL_CROSS);
    pack.addClientResource("minecraft/models/block/tinted_cross.json", MODEL_TINTED_CROSS);

    // 空气
    pack.addClientResource("minecraft/models/block/air.json", MODEL_AIR);

    // 原版方块的完整模型定义
    // 石头
    pack.addClientResource("minecraft/models/block/stone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/stone"
        }
    })");

    // 泥土
    pack.addClientResource("minecraft/models/block/dirt.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/dirt"
        }
    })");

    // 草方块
    pack.addClientResource("minecraft/models/block/grass_block.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/grass_block_top",
            "bottom": "block/dirt",
            "top": "block/grass_block_top",
            "side": "block/grass_block_side",
            "overlay": "block/grass_block_side_overlay"
        },
        "elements": [
            {
                "from": [0, 0, 0],
                "to": [16, 16, 16],
                "faces": {
                    "down": {"uv": [0, 0, 16, 16], "texture": "#bottom", "cullface": "down"},
                    "up": {"uv": [0, 0, 16, 16], "texture": "#top", "cullface": "up", "tintindex": 0},
                    "north": {"uv": [0, 0, 16, 16], "texture": "#side", "cullface": "north"},
                    "south": {"uv": [0, 0, 16, 16], "texture": "#side", "cullface": "south"},
                    "west": {"uv": [0, 0, 16, 16], "texture": "#side", "cullface": "west"},
                    "east": {"uv": [0, 0, 16, 16], "texture": "#side", "cullface": "east"}
                }
            },
            {
                "from": [0, 0, 0],
                "to": [16, 16, 16],
                "faces": {
                    "north": {"uv": [0, 0, 16, 16], "texture": "#overlay", "cullface": "north", "tintindex": 0},
                    "south": {"uv": [0, 0, 16, 16], "texture": "#overlay", "cullface": "south", "tintindex": 0},
                    "west": {"uv": [0, 0, 16, 16], "texture": "#overlay", "cullface": "west", "tintindex": 0},
                    "east": {"uv": [0, 0, 16, 16], "texture": "#overlay", "cullface": "east", "tintindex": 0}
                }
            }
        ]
    })");

    // 基岩
    pack.addClientResource("minecraft/models/block/bedrock.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/bedrock"
        }
    })");

    // 沙子
    pack.addClientResource("minecraft/models/block/sand.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/sand"
        }
    })");

    // 砾石
    pack.addClientResource("minecraft/models/block/gravel.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/gravel"
        }
    })");

    // 橡木木板
    pack.addClientResource("minecraft/models/block/oak_planks.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/oak_planks"
        }
    })");

    // 橡木原木
    pack.addClientResource("minecraft/models/block/oak_log.json", R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/oak_log_top",
            "side": "block/oak_log"
        }
    })");

    // 橡木树叶
    pack.addClientResource("minecraft/models/block/oak_leaves.json", R"({
        "parent": "block/leaves",
        "textures": {
            "all": "block/oak_leaves"
        }
    })");

    // 圆石
    pack.addClientResource("minecraft/models/block/cobblestone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/cobblestone"
        }
    })");

    // 苔石圆石
    pack.addClientResource("minecraft/models/block/mossy_cobblestone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/mossy_cobblestone"
        }
    })");

    // 各类矿石
    pack.addClientResource("minecraft/models/block/coal_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/coal_ore"
        }
    })");

    pack.addClientResource("minecraft/models/block/iron_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/iron_ore"
        }
    })");

    pack.addClientResource("minecraft/models/block/gold_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/gold_ore"
        }
    })");

    pack.addClientResource("minecraft/models/block/diamond_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/diamond_ore"
        }
    })");

    pack.addClientResource("minecraft/models/block/emerald_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/emerald_ore"
        }
    })");

    pack.addClientResource("minecraft/models/block/lapis_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/lapis_ore"
        }
    })");

    pack.addClientResource("minecraft/models/block/redstone_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/redstone_ore"
        }
    })");

    // 矿物方块
    pack.addClientResource("minecraft/models/block/diamond_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/diamond_block"
        }
    })");

    pack.addClientResource("minecraft/models/block/gold_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/gold_block"
        }
    })");

    pack.addClientResource("minecraft/models/block/iron_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/iron_block"
        }
    })");

    pack.addClientResource("minecraft/models/block/emerald_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/emerald_block"
        }
    })");

    pack.addClientResource("minecraft/models/block/lapis_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/lapis_block"
        }
    })");

    pack.addClientResource("minecraft/models/block/redstone_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/redstone_block"
        }
    })");

    // 石头变种
    pack.addClientResource("minecraft/models/block/granite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/granite"
        }
    })");

    pack.addClientResource("minecraft/models/block/polished_granite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_granite"
        }
    })");

    pack.addClientResource("minecraft/models/block/diorite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/diorite"
        }
    })");

    pack.addClientResource("minecraft/models/block/polished_diorite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_diorite"
        }
    })");

    pack.addClientResource("minecraft/models/block/andesite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/andesite"
        }
    })");

    pack.addClientResource("minecraft/models/block/polished_andesite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_andesite"
        }
    })");

    // 砂岩
    pack.addClientResource("minecraft/models/block/sandstone.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/sandstone_side",
            "down": "block/sandstone_bottom",
            "up": "block/sandstone_top",
            "north": "block/sandstone_side",
            "east": "block/sandstone_side",
            "south": "block/sandstone_side",
            "west": "block/sandstone_side"
        }
    })");

    // 其他方块
    pack.addClientResource("minecraft/models/block/bricks.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/bricks"
        }
    })");

    pack.addClientResource("minecraft/models/block/bookshelf.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/oak_planks",
            "down": "block/oak_planks",
            "up": "block/oak_planks",
            "north": "block/bookshelf",
            "east": "block/bookshelf",
            "south": "block/bookshelf",
            "west": "block/bookshelf"
        }
    })");

    pack.addClientResource("minecraft/models/block/tnt.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/tnt_side",
            "down": "block/tnt_bottom",
            "up": "block/tnt_top",
            "north": "block/tnt_side",
            "east": "block/tnt_side",
            "south": "block/tnt_side",
            "west": "block/tnt_side"
        }
    })");

    pack.addClientResource("minecraft/models/block/sponge.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/sponge"
        }
    })");

    pack.addClientResource("minecraft/models/block/wet_sponge.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/wet_sponge"
        }
    })");

    // 下界方块
    pack.addClientResource("minecraft/models/block/netherrack.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/netherrack"
        }
    })");

    pack.addClientResource("minecraft/models/block/glowstone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/glowstone"
        }
    })");

    pack.addClientResource("minecraft/models/block/end_stone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/end_stone"
        }
    })");

    pack.addClientResource("minecraft/models/block/obsidian.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/obsidian"
        }
    })");

    pack.addClientResource("minecraft/models/block/soul_sand.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/soul_sand"
        }
    })");

    pack.addClientResource("minecraft/models/block/soul_soil.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/soul_soil"
        }
    })");

    pack.addClientResource("minecraft/models/block/basalt.json", R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/basalt_top",
            "side": "block/basalt_side"
        }
    })");

    pack.addClientResource("minecraft/models/block/polished_basalt.json", R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/polished_basalt_top",
            "side": "block/polished_basalt_side"
        }
    })");

    pack.addClientResource("minecraft/models/block/blackstone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/blackstone"
        }
    })");

    pack.addClientResource("minecraft/models/block/polished_blackstone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_blackstone"
        }
    })");

    pack.addClientResource("minecraft/models/block/crying_obsidian.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/crying_obsidian"
        }
    })");

    // 冰和雪
    pack.addClientResource("minecraft/models/block/ice.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/ice"
        }
    })");

    pack.addClientResource("minecraft/models/block/snow.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/snow"
        }
    })");

    // 水（透明方块）
    pack.addClientResource("minecraft/models/block/water.json", R"({
        "textures": {
            "particle": "block/water_still"
        },
        "elements": [
            {
                "from": [0, 0, 0],
                "to": [16, 16, 16],
                "faces": {
                    "down": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "up": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "north": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "south": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "west": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "east": {"uv": [0, 0, 16, 16], "texture": "#particle"}
                }
            }
        ]
    })");

    // 岩浆
    pack.addClientResource("minecraft/models/block/lava.json", R"({
        "textures": {
            "particle": "block/lava_still"
        },
        "elements": [
            {
                "from": [0, 0, 0],
                "to": [16, 16, 16],
                "faces": {
                    "down": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "up": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "north": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "south": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "west": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "east": {"uv": [0, 0, 16, 16], "texture": "#particle"}
                }
            }
        ]
    })");

    // 各种木板
    const char* planksTemplate = R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/%s_planks"
        }
    })";

    const char* planksTypes[] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : planksTypes) {
        std::string model = std::string(planksTemplate);
        size_t pos = model.find("%s");
        if (pos != std::string::npos) {
            model.replace(pos, 2, type);
        }
        pack.addClientResource("minecraft/models/block/" + std::string(type) + "_planks.json", model);
    }

    // 羊毛
    const char* woolTemplate = R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/%s_wool"
        }
    })";

    const char* woolColors[] = {"white",
        "orange",
        "magenta",
        "light_blue",
        "yellow",
        "lime",
        "pink",
        "gray",
        "light_gray",
        "cyan",
        "purple",
        "blue",
        "brown",
        "green",
        "red",
        "black"};
    for (const auto& color : woolColors) {
        std::string model = std::string(woolTemplate);
        size_t pos = model.find("%s");
        if (pos != std::string::npos) {
            model.replace(pos, 2, color);
        }
        pack.addClientResource("minecraft/models/block/" + std::string(color) + "_wool.json", model);
    }

    // 泥土变种
    pack.addClientResource("minecraft/models/block/coarse_dirt.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/coarse_dirt"
        }
    })");

    pack.addClientResource("minecraft/models/block/podzol.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/podzol_side",
            "down": "block/dirt",
            "up": "block/podzol_top",
            "north": "block/podzol_side",
            "east": "block/podzol_side",
            "south": "block/podzol_side",
            "west": "block/podzol_side"
        }
    })");

    // 原木变种（使用 cube_column 模板）
    const char* logTemplate = R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/%s_log_top",
            "side": "block/%s_log"
        }
    })";

    const char* logTypes[] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : logTypes) {
        std::string model = std::string(logTemplate);
        size_t pos = model.find("%s");
        while (pos != std::string::npos) {
            model.replace(pos, 2, type);
            pos = model.find("%s", pos + strlen(type));
        }
        pack.addClientResource("minecraft/models/block/" + std::string(type) + "_log.json", model);
    }

    // Leaves variants
    const char* leavesTemplate = R"({
        "parent": "block/leaves",
        "textures": {
            "all": "block/%s_leaves"
        }
    })";

    const char* leavesTypes[] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : leavesTypes) {
        std::string model = std::string(leavesTemplate);
        size_t pos = model.find("%s");
        if (pos != std::string::npos) {
            model.replace(pos, 2, type);
        }
        pack.addClientResource("minecraft/models/block/" + std::string(type) + "_leaves.json", model);
    }

    // Saplings
    const char* saplingTemplate = R"({
        "parent": "block/cross",
        "textures": {
            "cross": "block/%s_sapling"
        }
    })";

    const char* saplingTypes[] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : saplingTypes) {
        std::string model = std::string(saplingTemplate);
        size_t pos = model.find("%s");
        if (pos != std::string::npos) {
            model.replace(pos, 2, type);
        }
        pack.addClientResource("minecraft/models/block/" + std::string(type) + "_sapling.json", model);
    }

    // Flowers
    const char* flowerTemplate = R"({
        "parent": "block/cross",
        "textures": {
            "cross": "block/%s"
        }
    })";

    const char* flowers[] = {"dandelion",
        "poppy",
        "blue_orchid",
        "allium",
        "azure_bluet",
        "red_tulip",
        "orange_tulip",
        "white_tulip",
        "pink_tulip",
        "oxeye_daisy",
        "brown_mushroom",
        "red_mushroom"};
    for (const auto& flower : flowers) {
        std::string model = std::string(flowerTemplate);
        size_t pos = model.find("%s");
        if (pos != std::string::npos) {
            model.replace(pos, 2, flower);
        }
        pack.addClientResource("minecraft/models/block/" + std::string(flower) + ".json", model);
    }

    // 草与蕨类（需要生物群系着色）
    const char* tintedCrossTemplate = R"({
        "parent": "block/tinted_cross",
        "textures": {
            "cross": "block/%s"
        }
    })";

    const char* tintedCrossPlants[] = {"short_grass", "fern"};
    for (const auto& plant : tintedCrossPlants) {
        std::string model = std::string(tintedCrossTemplate);
        size_t pos = model.find("%s");
        if (pos != std::string::npos) {
            model.replace(pos, 2, plant);
        }
        pack.addClientResource("minecraft/models/block/" + std::string(plant) + ".json", model);
    }

    // Tall grass
    pack.addClientResource("minecraft/models/block/tall_grass.json", R"({
        "parent": "block/tinted_cross",
        "textures": {
            "cross": "block/tall_grass_top"
        }
    })");

    // Crafting table
    pack.addClientResource("minecraft/models/block/crafting_table.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/crafting_table_front",
            "down": "block/crafting_table_bottom",
            "up": "block/crafting_table_top",
            "north": "block/crafting_table_front",
            "east": "block/crafting_table_side",
            "south": "block/crafting_table_side",
            "west": "block/crafting_table_front"
        }
    })");
}

// ============================================================================
// 注册 Blockstates
// ============================================================================

void VanillaResources::_registerBlockStates(InMemoryResourcePack& pack)
{
    // Most simple blocks use "normal" variant
    const char* simpleBlockstate = R"({
        "variants": {
            "normal": { "model": "%s" }
        }
    })";

    // 注册所有简单方块的 blockstates
    const char* simpleBlocks[] = {"stone",
        "dirt",
        "grass_block",
        "bedrock",
        "sand",
        "gravel",
        "cobblestone",
        "mossy_cobblestone",
        "oak_planks",
        "coal_ore",
        "iron_ore",
        "gold_ore",
        "diamond_ore",
        "emerald_ore",
        "lapis_ore",
        "redstone_ore",
        "diamond_block",
        "gold_block",
        "iron_block",
        "emerald_block",
        "lapis_block",
        "redstone_block",
        "granite",
        "polished_granite",
        "diorite",
        "polished_diorite",
        "andesite",
        "polished_andesite",
        "sandstone",
        "chiseled_sandstone",
        "cut_sandstone",
        "red_sandstone",
        "bricks",
        "bookshelf",
        "tnt",
        "sponge",
        "wet_sponge",
        "netherrack",
        "glowstone",
        "end_stone",
        "obsidian",
        "soul_sand",
        "soul_soil",
        "basalt",
        "polished_basalt",
        "blackstone",
        "polished_blackstone",
        "crying_obsidian",
        "ice",
        "snow",
        "water",
        "lava",
        "coarse_dirt",
        "podzol",
        "spruce_planks",
        "birch_planks",
        "jungle_planks",
        "acacia_planks",
        "dark_oak_planks",
        "white_wool",
        "orange_wool",
        "magenta_wool",
        "light_blue_wool",
        "yellow_wool",
        "lime_wool",
        "pink_wool",
        "gray_wool",
        "light_gray_wool",
        "cyan_wool",
        "purple_wool",
        "blue_wool",
        "brown_wool",
        "green_wool",
        "red_wool",
        "black_wool"};

    for (const auto& block : simpleBlocks) {
        std::string json = std::string(simpleBlockstate);
        size_t pos = json.find("%s");
        if (pos != std::string::npos) {
            json.replace(pos, 2, block);
        }
        pack.addClientResource("minecraft/blockstates/" + std::string(block) + ".json", json);
    }

    // Oak log (has axis property)
    pack.addClientResource("minecraft/blockstates/oak_log.json", R"({
        "variants": {
            "axis=y": { "model": "oak_log" },
            "axis=z": { "model": "oak_log", "x": 90 },
            "axis=x": { "model": "oak_log", "x": 90, "y": 90 }
        }
    })");

    // Air
    pack.addClientResource("minecraft/blockstates/air.json", R"({
        "variants": {
            "normal": { "model": "air" }
        }
    })");

    // Oak leaves
    pack.addClientResource("minecraft/blockstates/oak_leaves.json", R"({
        "variants": {
            "normal": { "model": "oak_leaves" }
        }
    })");

    // Wood log blockstates (with axis property)
    const char* logBlockstateTemplate = R"({
        "variants": {
            "axis=y": { "model": "%s_log" },
            "axis=z": { "model": "%s_log", "x": 90 },
            "axis=x": { "model": "%s_log", "x": 90, "y": 90 }
        }
    })";

    const char* logBlockstateTypes[] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : logBlockstateTypes) {
        std::string json = std::string(logBlockstateTemplate);
        size_t pos = json.find("%s");
        while (pos != std::string::npos) {
            json.replace(pos, 2, type);
            pos = json.find("%s", pos + strlen(type));
        }
        pack.addClientResource("minecraft/blockstates/" + std::string(type) + "_log.json", json);
    }

    // Leaves blockstates
    const char* leavesBlockstateTemplate = R"({
        "variants": {
            "normal": { "model": "%s_leaves" }
        }
    })";

    const char* leavesBlockstateTypes[] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : leavesBlockstateTypes) {
        std::string json = std::string(leavesBlockstateTemplate);
        size_t pos = json.find("%s");
        if (pos != std::string::npos) {
            json.replace(pos, 2, type);
        }
        pack.addClientResource("minecraft/blockstates/" + std::string(type) + "_leaves.json", json);
    }

    // Sapling blockstates
    const char* saplingBlockstateTemplate = R"({
        "variants": {
            "normal": { "model": "%s_sapling" }
        }
    })";

    const char* saplingBlockstateTypes[] = {"oak", "spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : saplingBlockstateTypes) {
        std::string json = std::string(saplingBlockstateTemplate);
        size_t pos = json.find("%s");
        if (pos != std::string::npos) {
            json.replace(pos, 2, type);
        }
        pack.addClientResource("minecraft/blockstates/" + std::string(type) + "_sapling.json", json);
    }

    // Flower and vegetation blockstates
    const char* flowerBlockstateTemplate = R"({
        "variants": {
            "normal": { "model": "%s" }
        }
    })";

    const char* flowerBlockstates[] = {"dandelion",
        "poppy",
        "blue_orchid",
        "allium",
        "azure_bluet",
        "red_tulip",
        "orange_tulip",
        "white_tulip",
        "pink_tulip",
        "oxeye_daisy",
        "brown_mushroom",
        "red_mushroom",
        "short_grass",
        "fern",
        "tall_grass"};
    for (const auto& flower : flowerBlockstates) {
        std::string json = std::string(flowerBlockstateTemplate);
        size_t pos = json.find("%s");
        if (pos != std::string::npos) {
            json.replace(pos, 2, flower);
        }
        pack.addClientResource("minecraft/blockstates/" + std::string(flower) + ".json", json);
    }

    // Crafting table
    pack.addClientResource("minecraft/blockstates/crafting_table.json", R"({
        "variants": {
            "normal": { "model": "crafting_table" }
        }
    })");
}

} // namespace mc::resource
