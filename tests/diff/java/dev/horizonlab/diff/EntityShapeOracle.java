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
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.EntityDimensions;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.MobCategory;

/**
 * 实体碰撞箱尺寸的原版 oracle。
 *
 * <p>与 probes/entity_shape.cpp 同构：宽度、高度、眼睛高度、是否固定尺寸、分类。
 * 尺寸按位输出，避免十进制打印的舍入歧义。
 */
public final class EntityShapeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityShapeOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            writeList(Path.of(args[1]));
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: EntityShapeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    /** 单元写文件而不是 stdout：原版 log4j 的配置会把 stdout 拿去记日志。 */
    private static void writeList(Path output) throws IOException {
        List<String> names = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.ENTITY_TYPE.keySet()) {
            names.add(id.toString());
        }
        names.sort(Comparator.naturalOrder());
        StringBuilder text = new StringBuilder();
        for (String name : names) {
            text.append(name).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static void run(Path input, Path output) throws IOException {
        try (BufferedReader reader = Files.newBufferedReader(input, StandardCharsets.UTF_8);
                BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.isBlank()) {
                    continue;
                }
                JsonObject request = JsonParser.parseString(line).getAsJsonObject();
                writer.write(GSON.toJson(summarise(request.get("entity").getAsString())));
                writer.write('\n');
            }
        }
    }

    private static JsonObject summarise(String entityId) {
        JsonObject result = new JsonObject();
        result.addProperty("entity", entityId);

        EntityType<?> type =
                BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(entityId)).orElse(null);
        if (type == null) {
            result.addProperty("missing", true);
            return result;
        }

        EntityDimensions dimensions = type.getDimensions();
        result.addProperty("width", floatBits(dimensions.width()));
        result.addProperty("height", floatBits(dimensions.height()));
        result.addProperty("eyeHeight", floatBits(dimensions.eyeHeight()));
        result.addProperty("fixed", dimensions.fixed());
        result.addProperty("classification", categoryName(type.getCategory()));
        return result;
    }

    /** MobCategory 名字：与 Cubium 的 EntityClassification 一一对应。 */
    private static String categoryName(MobCategory category) {
        return switch (category) {
            case MONSTER -> "monster";
            case CREATURE -> "creature";
            case AMBIENT -> "ambient";
            case AXOLOTLS -> "axolotls";
            case UNDERGROUND_WATER_CREATURE -> "underground_water_creature";
            case WATER_CREATURE -> "water_creature";
            case WATER_AMBIENT -> "water_ambient";
            case MISC -> "misc";
        };
    }

    private static String floatBits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}