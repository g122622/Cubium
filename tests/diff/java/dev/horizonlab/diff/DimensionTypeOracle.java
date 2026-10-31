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
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.level.dimension.DimensionType;

/**
 * 维度类型的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DimensionTypeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/dimension_type.cpp 同构：比较高度三项、坐标缩放、环境光、三个布尔、
 * 天空盒与方位光类型、无限燃烧标签、刷怪光照取值、以及原版 codec 的三条校验结果。
 */
public final class DimensionTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private DimensionTypeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            List<String> names = new ArrayList<>();
            registries
                    .lookup
                    .lookupOrThrow(Registries.DIMENSION_TYPE)
                    .listElements()
                    .forEach(holder -> names.add(holder.key().identifier().toString()));
            Collections.sort(names);
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: DimensionTypeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var types = registries.lookup.lookupOrThrow(Registries.DIMENSION_TYPE);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("dimensionType").getAsString();
            text.append(GSON.toJson(summarise(types, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<DimensionType> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("dimensionType", name);

        var holder =
                registry.get(ResourceKey.create(Registries.DIMENSION_TYPE, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        DimensionType type = holder.value();
        result.addProperty("minY", type.minY());
        result.addProperty("height", type.height());
        result.addProperty("logicalHeight", type.logicalHeight());
        result.addProperty("coordinateScale", formatFloat(type.coordinateScale()));
        result.addProperty("ambientLight", formatFloat(type.ambientLight()));
        result.addProperty("hasSkylight", type.hasSkyLight());
        result.addProperty("hasCeiling", type.hasCeiling());
        result.addProperty("hasFixedTime", type.hasFixedTime());
        result.addProperty("skybox", type.skybox().getSerializedName());
        result.addProperty("cardinalLight", type.cardinalLightType().getSerializedName());
        result.addProperty("infiniburn", type.infiniburn().location().toString());
        result.addProperty("monsterSpawnLightLevel", intProviderToString(type.monsterSpawnLightTest()));
        result.addProperty("monsterSpawnBlockLightLimit", type.monsterSpawnBlockLightLimit());
        result.addProperty("attributeCount", type.attributes().keySet().size());
        var timelineEither = type.timelines().unwrap();
        int timelineCount =
                timelineEither.left().isPresent()
                        ? 1
                        : timelineEither.right().orElse(List.of()).size();
        result.addProperty("timelineCount", timelineCount);
        // 原版 createDirectCodec 的三条校验：合法的数据必然全部为 true。
        result.addProperty("validHeightMultiple", type.height() % 16 == 0);
        result.addProperty("validMinYMultiple", type.minY() % 16 == 0);
        result.addProperty("validLogicalHeight", type.logicalHeight() <= type.height());
        return result;
    }

    /**
     * 把 {@code monster_spawn_light_level} 归一成字符串：常量输出整数，其他形态输出
     * 其 {@code type} 名加关键字段的紧凑形式。
     */
    private static String intProviderToString(net.minecraft.util.valueproviders.IntProvider provider) {
        if (provider instanceof net.minecraft.util.valueproviders.ConstantInt constant) {
            return Integer.toString(constant.getValue());
        }
        // 非恒定形态：原版 IntProvider 没有统一编码入口，这里按类名 + 采样下界描述。
        // 数据包 4 项里只有 uniform 一种，min=max=7 之外均为常量。
        if (provider instanceof net.minecraft.util.valueproviders.UniformInt uniform) {
            return "minecraft:uniform[" + uniform.getMinValue() + "," + uniform.getMaxValue() + "]";
        }
        JsonArray array = new JsonArray();
        array.add(provider.getClass().getSimpleName());
        return array.toString();
    }

    private static String formatFloat(double value) {
        String text = String.format("%.6g", value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            while (text.endsWith("0")) {
                text = text.substring(0, text.length() - 1);
            }
            if (text.endsWith(".")) {
                text = text.substring(0, text.length() - 1);
            }
        }
        return text;
    }
}