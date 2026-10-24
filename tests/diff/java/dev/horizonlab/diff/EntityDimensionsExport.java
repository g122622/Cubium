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

/**
 * 把原版全部实体类型的尺寸导出成 JSON，供 generator 生成 Cubium 的数据表。
 *
 * <pre>
 *   java dev.horizonlab.diff.EntityDimensionsExport &lt;output.json&gt;
 * </pre>
 *
 * 输出字段：width / height / eyeHeight / fixed。用字符串保留原始文本，避免
 * 十进制往返转换引入偏差；Cubium 侧按 f32 字面量解析。
 */
public final class EntityDimensionsExport {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EntityDimensionsExport() {}

    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("usage: EntityDimensionsExport <output.json>");
            System.exit(2);
        }

        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        List<String> names = new ArrayList<>();
        for (Identifier id : BuiltInRegistries.ENTITY_TYPE.keySet()) {
            names.add(id.toString());
        }
        names.sort(Comparator.naturalOrder());

        JsonArray entries = new JsonArray();
        for (String name : names) {
            EntityType<?> type = BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(name)).orElseThrow();
            EntityDimensions dimensions = type.getDimensions();
            JsonObject entry = new JsonObject();
            entry.addProperty("entity", name);
            entry.addProperty("width", Float.toString(dimensions.width()));
            entry.addProperty("height", Float.toString(dimensions.height()));
            entry.addProperty("eyeHeight", Float.toString(dimensions.eyeHeight()));
            entry.addProperty("fixed", dimensions.fixed());
            entries.add(entry);
        }

        Files.writeString(Path.of(args[0]), GSON.toJson(entries), StandardCharsets.UTF_8);
        System.out.println("exported " + entries.size() + " entity dimensions");
    }
}