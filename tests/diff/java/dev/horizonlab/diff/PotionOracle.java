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
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.item.alchemy.Potion;

/**
 * 药水定义的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PotionOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/potion.cpp 同构：比较基础名、效果列表（按效果 ID 排序后的集合）、是否含瞬时
 * 效果，以及三种前缀下的翻译键。
 */
public final class PotionOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private PotionOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (var key : BuiltInRegistries.POTION.keySet()) {
                names.add(key.toString());
            }
            names.sort(Comparator.naturalOrder());
            Files.writeString(Path.of(args[1]), String.join("\n", names) + "\n", StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: PotionOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("potion").getAsString();
            text.append(GSON.toJson(summarise(name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String name) {
        JsonObject result = new JsonObject();
        result.addProperty("potion", name);

        var key = net.minecraft.resources.ResourceKey.create(
                net.minecraft.core.registries.Registries.POTION,
                net.minecraft.resources.Identifier.parse(name));
        var optional = BuiltInRegistries.POTION.get(key);
        if (optional.isEmpty()) {
            result.addProperty("missing", true);
            return result;
        }
        Potion potion = (Potion) optional.get().value();

        // `Potion#name` 是 private，用 getter 不可得；原版显示名的三段前缀由
        // `PotionContents#getName` 拼装，因此这里同样取它对外暴露的两个入口：
        // 效果列表与瞬时效果判定，另用反射读 private name 与 baseName 对齐。
        result.addProperty("baseName", readName(potion));

        List<JsonObject> effects = new ArrayList<>();
        for (MobEffectInstance effect : potion.getEffects()) {
            effects.add(effectJson(effect));
        }
        effects.sort(Comparator.comparing(e -> e.get("id").getAsString()));
        JsonArray array = new JsonArray();
        effects.forEach(array::add);
        result.add("effects", array);

        result.addProperty("hasInstant", potion.hasInstantEffects());

        JsonArray names = new JsonArray();
        names.add(prefixed(potion, ""));
        names.add(prefixed(potion, "Splash "));
        names.add(prefixed(potion, "Lingering "));
        result.add("names", names);
        return result;
    }

    /**
     * 原版 `Potion` 没有公开的 name getter，但翻译键由 `PotionContents#getName` 用
     * `potion.value().name()` 拼出。这里按同一规则复现：读 private 字段 `name`。
     */
    private static String readName(Potion potion) {
        try {
            var field = Potion.class.getDeclaredField("name");
            field.setAccessible(true);
            return (String) field.get(potion);
        } catch (ReflectiveOperationException error) {
            throw new AssertionError("无法读取 Potion.name", error);
        }
    }

    /** 对应 Cubium 的 `Potion::getNamePrefixed`：`<prefix>.effect.minecraft.<name>`。 */
    private static String prefixed(Potion potion, String prefix) {
        String base = readName(potion);
        if (base.isEmpty()) {
            return prefix;
        }
        return prefix + ".effect.minecraft." + base;
    }

    private static JsonObject effectJson(MobEffectInstance effect) {
        JsonObject result = new JsonObject();
        var effectKey = BuiltInRegistries.MOB_EFFECT.getKey(effect.getEffect().value());
        result.addProperty("id", effectKey == null ? "minecraft:" : effectKey.toString());
        result.addProperty("duration", effect.getDuration());
        result.addProperty("amplifier", effect.getAmplifier());
        result.addProperty("ambient", effect.isAmbient());
        result.addProperty("visible", effect.isVisible());
        result.addProperty("showIcon", effect.showIcon());
        return result;
    }
}