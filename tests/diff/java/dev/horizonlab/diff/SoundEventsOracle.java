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
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import net.minecraft.SharedConstants;
import net.minecraft.server.Bootstrap;
import net.minecraft.sounds.SoundEvent;
import net.minecraft.sounds.SoundEvents;

/**
 * {@code SoundEvents} 音效 ID 常量表的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.SoundEventsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>不硬编码音效名，而是**反射扫描 {@link SoundEvents} 里所有 {@link SoundEvent} 类型的
 * public static final 字段**，取其 {@code location()} 的 path。原版新增音效时 oracle
 * 自动跟上。
 *
 * <p>{@code list} 输出的是 oracle 自己枚举到的全集，因此探针的单元表必须来自同一份
 * 原版数据——见 probes/sound_events.cpp 的 VANILLA_SOUNDS。
 */
public final class SoundEventsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private SoundEventsOracle() {}

    /** 反射收集原版全部音效 ID（path 部分）。 */
    private static List<String> collectSorted() throws IllegalAccessException {
        List<String> list = new ArrayList<>(collect());
        // 排序保证 list 输出确定（反射的字段顺序不保证跨 JVM 一致）。
        Collections.sort(list);
        return list;
    }

    private static Set<String> collect() throws IllegalAccessException {
        Set<String> out = new LinkedHashSet<>();
        for (Field field : SoundEvents.class.getDeclaredFields()) {
            if (!Modifier.isStatic(field.getModifiers()) || !Modifier.isFinal(field.getModifiers())) {
                continue;
            }
            if (field.getType() != SoundEvent.class) {
                continue;
            }
            SoundEvent event = (SoundEvent) field.get(null);
            out.add(event.location().getPath());
        }
        return out;
    }

    public static void main(String[] args) throws Exception {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            List<String> all = collectSorted();
            StringBuilder text = new StringBuilder();
            for (String id : all) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: SoundEventsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws Exception {
        Set<String> known = collect();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            JsonObject row = new JsonObject();
            row.addProperty("result", known.contains(unit) ? "OK" : "MISSING");
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }
}