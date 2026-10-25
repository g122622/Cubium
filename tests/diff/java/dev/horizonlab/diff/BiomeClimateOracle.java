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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.attribute.EnvironmentAttributeMap;
import net.minecraft.world.attribute.EnvironmentAttributes;
import net.minecraft.world.level.biome.Biome;

/**
 * 生物群系气候与视觉参数的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BiomeClimateOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/biome_climate.cpp 同构：
 *
 * <pre>
 *   { "biome": "minecraft:plains", "temperature": 0.8, "downfall": 0.4,
 *     "hasPrecipitation": true, "temperatureModifier": "none",
 *     "waterColor": 4159204, "waterFogColor": 329011, "fogColor": 12638463,
 *     "skyColor": 7907327, "grassColorModifier": "none" }
 * </pre>
 *
 * <p>1.21.11 把雾色与天空色挪进了 {@code EnvironmentAttributeMap}（数据包 JSON 的
 * {@code attributes} 段），没写时分别回落到 {@code FOG_COLOR} / {@code SKY_COLOR}
 * 的默认值；这里导出的是「同一个位置、同一个群系下客户端实际会拿到的值」。
 */
public final class BiomeClimateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();
    /** EnvironmentAttributes 里 FOG_COLOR / SKY_COLOR 的 defaultValue（源码为 0）。 */
    private static final int FOG_COLOR_DEFAULT = 0x000000;
    private static final int SKY_COLOR_DEFAULT = 0x000000;
    /** WATER_FOG_COLOR 属性的 defaultValue（源码 -16448205 = 0x050533）。 */
    private static final int WATER_FOG_COLOR_DEFAULT = 0x050533;

    /** 数据包没写 water_color 时原版回落的水色（vanilla 每个群系 JSON 都写全了）。 */
    private static final int WATER_COLOR_DEFAULT = 0x3F76E4;

    private BiomeClimateOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var biomes = registries.lookup.lookupOrThrow(Registries.BIOME);
            List<String> names = new ArrayList<>();
            biomes.listElements().map(holder -> holder.key().identifier().toString()).forEach(names::add);
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
        System.err.println("usage: BiomeClimateOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var biomes = registries.lookup.lookupOrThrow(Registries.BIOME);

        List<JsonObject> results = new ArrayList<>();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("biome").getAsString();
            results.add(summarise(biomes, name));
        }
        results.sort((left, right) -> left.get("biome").getAsString().compareTo(right.get("biome").getAsString()));

        StringBuilder text = new StringBuilder();
        for (JsonObject result : results) {
            text.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(net.minecraft.core.HolderLookup.RegistryLookup<Biome> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("biome", name);
        var key = ResourceKey.create(Registries.BIOME, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }
        Biome biome = holder.value();
        // 1.21.11 的 Biome 不再公开 ClimateSettings：温度与降水量挪进了数据包 JSON，
        // 客户端/服务端读到的是 getBaseTemperature()（temperature）与 hasPrecipitation()。
        result.addProperty("temperature", roundToFloat5(biome.getBaseTemperature()));
        result.addProperty("hasPrecipitation", biome.hasPrecipitation());
        // 三组颜色统一导出成 0xRRGGBB：原版内部是有符号 int，Cubium 侧是 u32，直接比 int 会一致。
        result.addProperty("waterColor", waterColor(biome));
        result.addProperty("waterFogColor",
                attributeColor(biome, EnvironmentAttributes.WATER_FOG_COLOR, WATER_FOG_COLOR_DEFAULT));
        result.addProperty("fogColor", attributeColor(biome, EnvironmentAttributes.FOG_COLOR, FOG_COLOR_DEFAULT));
        result.addProperty("skyColor", attributeColor(biome, EnvironmentAttributes.SKY_COLOR, SKY_COLOR_DEFAULT));
        result.addProperty("grassColorModifier", biome.getSpecialEffects().grassColorModifier().getSerializedName());
        return result;
    }

    /** 水色：特殊效果里的值；数据包没写时用原版回落值。 */
    private static int waterColor(Biome biome) {
        int value = biome.getSpecialEffects().waterColor();
        return value == 0 ? WATER_COLOR_DEFAULT : value & 0xFFFFFF;
    }

    /**
     * 温度按「十进制 5 位有效数字」输出成 double。
     *
     * <p>数据包用 JSON 数字写温度（0.949999988079071 这种 f32 展开），原版读成 f32
     * 后按 f32 参与计算；Cubium 侧同样是 f32。两侧都按同一格式输出，比较的就是
     * 「同一个 f32」，而不是 JSON 文本的写法。
     */
    private static double roundToFloat5(float value) {
        return Double.parseDouble(String.format(java.util.Locale.ROOT, "%.5f", value));
    }

    /**
     * 读环境属性里的颜色；没注册该属性时用原版默认值。
     *
     * <p>{@code applyModifier} 在属性缺失时原样返回传入值，所以这里直接传默认色：
     * 有属性就是群系覆盖色，没有就是默认色，和客户端计算的路径一致。
     */
    private static int attributeColor(Biome biome, net.minecraft.world.attribute.EnvironmentAttribute<Integer> attribute, int fallback) {
        EnvironmentAttributeMap attributes = biome.getAttributes();
        Integer value = attributes.applyModifier(attribute, fallback);
        return (value != null ? value : fallback) & 0xFFFFFF;
    }
}