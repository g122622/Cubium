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
import java.util.List;
import net.minecraft.core.Holder;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.effect.MobEffect;
import net.minecraft.world.effect.MobEffectInstance;

/**
 * 药水效果叠加合并的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MobEffectMergeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接构造两个 {@code MobEffectInstance} 并调用原版 {@code update}，不重写分支——
 * 本能力要钉住的正是那三层分支与 {@code hiddenEffect} 的降级回退语义。
 *
 * <p>场景表必须与 probes/mob_effect_merge.cpp 逐项一致。
 */
public final class MobEffectMergeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 场景表：`原等级|原时长|原环境|新等级|新时长|新环境`。 */
    private static final List<String> SCENES =
            List.of(
                    "0|200|false|1|300|false",
                    "1|300|false|2|100|false",
                    "0|200|false|1|-1|false",
                    "1|-1|false|2|100|false",
                    "1|200|false|1|300|false",
                    "1|300|false|1|200|false",
                    "1|200|false|1|-1|false",
                    "1|-1|false|1|300|false",
                    "1|-1|false|1|-1|false",
                    "2|300|false|1|500|false",
                    "2|300|false|1|100|false",
                    "1|200|true|2|300|false",
                    "1|200|true|1|300|true",
                    "1|200|true|1|100|false",
                    "0|0|false|0|-1|false",
                    "0|-1|false|0|0|false");

    private MobEffectMergeOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String scene : SCENES) {
                text.append(scene).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: MobEffectMergeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var effects = registries.lookup.lookupOrThrow(Registries.MOB_EFFECT);
        // 合并语义与具体效果无关，取一个固定效果即可。
        Holder<MobEffect> effect =
                effects
                        .get(ResourceKey.create(Registries.MOB_EFFECT, Identifier.parse("minecraft:speed")))
                        .orElseThrow();

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(effect, unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(Holder<MobEffect> effect, String unit) {
        String[] parts = unit.split("\\|");
        if (parts.length != 6) {
            throw new IllegalArgumentException("mob_effect_merge: 单元应为 6 段: " + unit);
        }

        int selfAmplifier = Integer.parseInt(parts[0]);
        int selfDuration = Integer.parseInt(parts[1]);
        boolean selfAmbient = Boolean.parseBoolean(parts[2]);
        int incomingAmplifier = Integer.parseInt(parts[3]);
        int incomingDuration = Integer.parseInt(parts[4]);
        boolean incomingAmbient = Boolean.parseBoolean(parts[5]);

        // 构造当前效果与传入效果。ambient 通过构造器参数传入。
        MobEffectInstance current = new MobEffectInstance(
                effect, selfDuration, selfAmplifier, selfAmbient, true, true);
        MobEffectInstance incoming = new MobEffectInstance(
                effect, incomingDuration, incomingAmplifier, incomingAmbient, true, true);

        boolean changed = current.update(incoming);

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        result.addProperty("resultAmplifier", current.getAmplifier());
        result.addProperty("resultDuration", current.getDuration());
        result.addProperty("resultAmbient", current.isAmbient());
        result.addProperty("changed", changed);

        // hiddenEffect 是包内可见字段，这里通过公开访问器取；没有则记为 -1 / 0。
        MobEffectInstance hidden = hiddenEffectOf(current);
        if (hidden == null) {
            result.addProperty("hiddenAmplifier", -1);
            result.addProperty("hiddenDuration", 0);
        } else {
            result.addProperty("hiddenAmplifier", hidden.getAmplifier());
            result.addProperty("hiddenDuration", hidden.getDuration());
        }
        return result;
    }

    /**
     * 读取 {@code MobEffectInstance.hiddenEffect}。
     *
     * 该字段是包内可见的私有字段，没有公开访问器，只能用反射读。合并语义里
     * 「旧效果降级回退」完全靠这个字段承载，不读它就验证不到最关键的那一支。
     */
    private static MobEffectInstance hiddenEffectOf(MobEffectInstance instance) {
        try {
            var field = MobEffectInstance.class.getDeclaredField("hiddenEffect");
            field.setAccessible(true);
            return (MobEffectInstance) field.get(instance);
        } catch (ReflectiveOperationException e) {
            throw new IllegalStateException("无法读取 MobEffectInstance.hiddenEffect", e);
        }
    }
}
