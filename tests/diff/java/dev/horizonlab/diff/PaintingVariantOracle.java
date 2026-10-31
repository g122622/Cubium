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
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.contents.TranslatableContents;
import net.minecraft.resources.ResourceKey;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.decoration.painting.PaintingVariant;

/**
 * 画变体的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PaintingVariantOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/painting_variant.cpp 同构：比较宽高、贴图 ID、标题与作者的翻译键。
 * 变体来自数据包 {@code data/<ns>/painting_variant/}，必须走 {@link DataPackRegistries}
 * 加载之后才非空。
 */
public final class PaintingVariantOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private PaintingVariantOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var variants = registries.lookup.lookupOrThrow(Registries.PAINTING_VARIANT);
            List<String> names = new ArrayList<>();
            variants.listElements().forEach(holder -> {
                names.add(holder.key().identifier().toString());
            });
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
        System.err.println("usage: PaintingVariantOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var variants = registries.lookup.lookupOrThrow(Registries.PAINTING_VARIANT);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("variant").getAsString();
            text.append(GSON.toJson(summarise(variants, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<PaintingVariant> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("variant", name);

        ResourceKey<PaintingVariant> key =
                ResourceKey.create(Registries.PAINTING_VARIANT, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        PaintingVariant variant = holder.value();
        result.addProperty("width", variant.width());
        result.addProperty("height", variant.height());
        result.addProperty("assetId", variant.assetId().toString());
        result.addProperty("titleKey", keyOf(variant.title().orElse(null)));
        result.addProperty("authorKey", keyOf(variant.author().orElse(null)));
        return result;
    }

    /**
     * 取组件的**翻译键**而非渲染文本。
     *
     * <p>画作标题不参与玩法，翻译键才稳定可比；用 `getString()` 会拿到当前语言下的
     * 显示文本（唱片机歌曲能力里踩过这个坑）。
     */
    private static String keyOf(Component component) {
        if (component == null) {
            return "";
        }
        if (component.getContents() instanceof TranslatableContents translatable) {
            return translatable.getKey();
        }
        return component.getString();
    }
}