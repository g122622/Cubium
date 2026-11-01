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

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.core.Holder;
import net.minecraft.world.level.saveddata.maps.MapDecorationType;
import net.minecraft.world.level.saveddata.maps.MapDecorationTypes;

/**
 * 地图装饰类型注册名的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MapDecorationTypeOracle list &lt;units.txt&gt;
 *       | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版 1.21.11 的地图装饰已从枚举改为**注册表对象**。序列化名是**注册表键**，
 * 由 {@code register(资源名, 纹理名, ...)} 的第一个参数决定：
 *
 * <pre>
 * register("mansion", "woodland_mansion", true, 5393476, false, true)
 * //        ^^^^^^^^   ^^^^^^^^^^^^^^^^
 * //        序列化名    纹理 id
 * </pre>
 *
 * <p>村庄的序列化名是 <b>{@code village_desert}</b>（{@code village_} 前缀），
 * 纹理名才是 {@code desert_village}。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>
 * （{@code world::map::decorationTypeToString} / {@code decorationTypeFromString}）。
 *
 * <p>本能力<b>需要 bootstrap</b>：{@link MapDecorationTypes} 的静态初始化会触碰
 * {@link net.minecraft.core.registries.BuiltInRegistries}，必须先调用
 * {@code Bootstrap.bootStrap()}，且注册表映射须惰性初始化（不能放在 static 块里）。
 *
 * <p>单元表必须与 probes/map_decoration_type.cpp 逐项一致。
 */
public final class MapDecorationTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count",
                    "toString|PLAYER",
                    "toString|FRAME",
                    "toString|RED_MARKER",
                    "toString|BLUE_MARKER",
                    "toString|TARGET_X",
                    "toString|TARGET_POINT",
                    "toString|PLAYER_OFF_MAP",
                    "toString|PLAYER_OFF_LIMITS",
                    "toString|WOODLAND_MANSION",
                    "toString|OCEAN_MONUMENT",
                    "toString|WHITE_BANNER",
                    "toString|ORANGE_BANNER",
                    "toString|MAGENTA_BANNER",
                    "toString|LIGHT_BLUE_BANNER",
                    "toString|YELLOW_BANNER",
                    "toString|LIME_BANNER",
                    "toString|PINK_BANNER",
                    "toString|GRAY_BANNER",
                    "toString|LIGHT_GRAY_BANNER",
                    "toString|CYAN_BANNER",
                    "toString|PURPLE_BANNER",
                    "toString|BLUE_BANNER",
                    "toString|BROWN_BANNER",
                    "toString|GREEN_BANNER",
                    "toString|RED_BANNER",
                    "toString|BLACK_BANNER",
                    "toString|RED_X",
                    "toString|DESERT_VILLAGE",
                    "toString|PLAINS_VILLAGE",
                    "toString|SAVANNA_VILLAGE",
                    "toString|SNOWY_VILLAGE",
                    "toString|TAIGA_VILLAGE",
                    "toString|JUNGLE_TEMPLE",
                    "toString|SWAMP_HUT",
                    "toString|TRIAL_CHAMBERS",
                    "fromString|mansion",
                    "fromString|woodland_mansion",
                    "fromString|village_desert",
                    "fromString|desert_village",
                    "fromString|village_taiga",
                    "fromString|dunno",
                    "fromString|");

    /** 常量名 → Holder，顺序与探针一致。 */
    private static final Map<String, Holder<MapDecorationType>> BY_NAME = new LinkedHashMap<>();

    /// 惰性初始化：MapDecorationTypes 的静态初始化会触碰 BuiltInRegistries，
    /// 必须在 Bootstrap.bootStrap() 之后才能访问。
    private static void initRegistry() {
        if (!BY_NAME.isEmpty()) {
            return;
        }
        BY_NAME.put("PLAYER", MapDecorationTypes.PLAYER);
        BY_NAME.put("FRAME", MapDecorationTypes.FRAME);
        BY_NAME.put("RED_MARKER", MapDecorationTypes.RED_MARKER);
        BY_NAME.put("BLUE_MARKER", MapDecorationTypes.BLUE_MARKER);
        BY_NAME.put("TARGET_X", MapDecorationTypes.TARGET_X);
        BY_NAME.put("TARGET_POINT", MapDecorationTypes.TARGET_POINT);
        BY_NAME.put("PLAYER_OFF_MAP", MapDecorationTypes.PLAYER_OFF_MAP);
        BY_NAME.put("PLAYER_OFF_LIMITS", MapDecorationTypes.PLAYER_OFF_LIMITS);
        BY_NAME.put("WOODLAND_MANSION", MapDecorationTypes.WOODLAND_MANSION);
        BY_NAME.put("OCEAN_MONUMENT", MapDecorationTypes.OCEAN_MONUMENT);
        BY_NAME.put("WHITE_BANNER", MapDecorationTypes.WHITE_BANNER);
        BY_NAME.put("ORANGE_BANNER", MapDecorationTypes.ORANGE_BANNER);
        BY_NAME.put("MAGENTA_BANNER", MapDecorationTypes.MAGENTA_BANNER);
        BY_NAME.put("LIGHT_BLUE_BANNER", MapDecorationTypes.LIGHT_BLUE_BANNER);
        BY_NAME.put("YELLOW_BANNER", MapDecorationTypes.YELLOW_BANNER);
        BY_NAME.put("LIME_BANNER", MapDecorationTypes.LIME_BANNER);
        BY_NAME.put("PINK_BANNER", MapDecorationTypes.PINK_BANNER);
        BY_NAME.put("GRAY_BANNER", MapDecorationTypes.GRAY_BANNER);
        BY_NAME.put("LIGHT_GRAY_BANNER", MapDecorationTypes.LIGHT_GRAY_BANNER);
        BY_NAME.put("CYAN_BANNER", MapDecorationTypes.CYAN_BANNER);
        BY_NAME.put("PURPLE_BANNER", MapDecorationTypes.PURPLE_BANNER);
        BY_NAME.put("BLUE_BANNER", MapDecorationTypes.BLUE_BANNER);
        BY_NAME.put("BROWN_BANNER", MapDecorationTypes.BROWN_BANNER);
        BY_NAME.put("GREEN_BANNER", MapDecorationTypes.GREEN_BANNER);
        BY_NAME.put("RED_BANNER", MapDecorationTypes.RED_BANNER);
        BY_NAME.put("BLACK_BANNER", MapDecorationTypes.BLACK_BANNER);
        BY_NAME.put("RED_X", MapDecorationTypes.RED_X);
        BY_NAME.put("DESERT_VILLAGE", MapDecorationTypes.DESERT_VILLAGE);
        BY_NAME.put("PLAINS_VILLAGE", MapDecorationTypes.PLAINS_VILLAGE);
        BY_NAME.put("SAVANNA_VILLAGE", MapDecorationTypes.SAVANNA_VILLAGE);
        BY_NAME.put("SNOWY_VILLAGE", MapDecorationTypes.SNOWY_VILLAGE);
        BY_NAME.put("TAIGA_VILLAGE", MapDecorationTypes.TAIGA_VILLAGE);
        BY_NAME.put("JUNGLE_TEMPLE", MapDecorationTypes.JUNGLE_TEMPLE);
        BY_NAME.put("SWAMP_HUT", MapDecorationTypes.SWAMP_HUT);
        BY_NAME.put("TRIAL_CHAMBERS", MapDecorationTypes.TRIAL_CHAMBERS);
    }

    private MapDecorationTypeOracle() {}

    public static void main(String[] args) throws IOException {
        // MapDecorationTypes 的静态初始化会触碰 BuiltInRegistries，必须先 bootstrap。
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        initRegistry();
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: MapDecorationTypeOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    /** 取注册表键的路径部分（去掉 {@code minecraft:} 前缀）。 */
    private static String registryName(Holder<MapDecorationType> holder) {
        String path = holder.unwrapKey().orElseThrow().identifier().getPath();
        return path;
    }

    /** 反查：注册名 → 常量名。 */
    private static String byRegistryName(String name) {
        for (Map.Entry<String, Holder<MapDecorationType>> entry : BY_NAME.entrySet()) {
            if (registryName(entry.getValue()).equals(name)) {
                return entry.getKey();
            }
        }
        return null;
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("count")) {
            out.addProperty("value", String.valueOf(BY_NAME.size()));
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("map_decoration_type: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "toString" -> {
                Holder<MapDecorationType> holder = BY_NAME.get(parts[1]);
                if (holder == null) {
                    throw new IllegalArgumentException(
                            "map_decoration_type: 未知常量: " + parts[1]);
                }
                out.addProperty("value", registryName(holder));
            }
            case "fromString" -> {
                String constant = byRegistryName(parts[1]);
                out.addProperty("value", constant == null ? "null" : constant);
            }
            default -> throw new IllegalArgumentException(
                    "map_decoration_type: 无法识别的单元: " + unit);
        }

        return out;
    }
}