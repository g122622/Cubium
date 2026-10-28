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
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.IdentityHashMap;
import java.util.List;
import net.minecraft.core.Holder;
import net.minecraft.world.effect.MobEffect;
import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.effect.MobEffects;

/** 直接运行原版实例操作；反射仅用于观察隐藏链及调用服务端共享的纯时长转换。 */
public final class StatusEffectOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().serializeNulls().create();
    private static final Field HIDDEN = hiddenField();
    private static final Method DECREMENT = durationMethod("tickDownDuration");
    private static final Method DOWNGRADE = durationMethod("downgradeToHiddenEffect");
    private static List<Holder<MobEffect>> effects;

    private static Field hiddenField() {
        try {
            Field field = MobEffectInstance.class.getDeclaredField("hiddenEffect");
            field.setAccessible(true);
            return field;
        } catch (ReflectiveOperationException error) {
            throw new ExceptionInInitializerError(error);
        }
    }

    private static Method durationMethod(String name) {
        try {
            Method method = MobEffectInstance.class.getDeclaredMethod(name);
            method.setAccessible(true);
            return method;
        } catch (ReflectiveOperationException error) {
            throw new ExceptionInInitializerError(error);
        }
    }

    private static int number(JsonObject value, String key) {
        return value.get(key).getAsInt();
    }

    private static int id(IdentityHashMap<MobEffectInstance, Integer> ids, MobEffectInstance instance) {
        return ids.computeIfAbsent(instance, unused -> ids.size());
    }

    private static JsonObject snapshot(MobEffectInstance[] slots, IdentityHashMap<MobEffectInstance, Integer> ids)
            throws IllegalAccessException {
        JsonObject result = new JsonObject();
        JsonArray roots = new JsonArray();
        JsonArray nodes = new JsonArray();
        List<MobEffectInstance> visit = new ArrayList<>();
        IdentityHashMap<MobEffectInstance, Boolean> seen = new IdentityHashMap<>();
        for (MobEffectInstance instance : slots) {
            roots.add(id(ids, instance));
            if (seen.put(instance, true) == null) {
                visit.add(instance);
            }
        }
        for (int i = 0; i < visit.size(); i++) {
            MobEffectInstance instance = visit.get(i);
            MobEffectInstance hidden = (MobEffectInstance) HIDDEN.get(instance);
            JsonObject node = new JsonObject();
            node.addProperty("id", id(ids, instance));
            node.addProperty("effect", effects.indexOf(instance.getEffect()));
            node.addProperty("duration", instance.getDuration());
            node.addProperty("amplifier", instance.getAmplifier());
            node.addProperty("ambient", instance.isAmbient());
            node.addProperty("visible", instance.isVisible());
            node.addProperty("icon", instance.showIcon());
            if (hidden == null) {
                node.add("hidden", JsonNull.INSTANCE);
            } else {
                node.addProperty("hidden", id(ids, hidden));
                if (seen.put(hidden, true) == null) {
                    visit.add(hidden);
                }
            }
            nodes.add(node);
        }
        result.add("slots", roots);
        result.add("nodes", nodes);
        return result;
    }

    private static final class CallbackFailure extends RuntimeException {}

    private static JsonObject run(JsonObject input) throws ReflectiveOperationException {
        JsonArray instances = input.getAsJsonArray("instances");
        MobEffectInstance[] slots = new MobEffectInstance[instances.size()];
        for (int i = 0; i < slots.length; i++) {
            JsonObject value = instances.get(i).getAsJsonObject();
            int hidden = number(value, "hidden");
            slots[i] = new MobEffectInstance(effects.get(number(value, "effect")), number(value, "duration"),
                    number(value, "amplifier"), value.get("ambient").getAsBoolean(), value.get("visible").getAsBoolean(),
                    value.get("icon").getAsBoolean(), hidden < 0 ? null : slots[hidden]);
        }
        IdentityHashMap<MobEffectInstance, Integer> ids = new IdentityHashMap<>();
        JsonObject result = new JsonObject();
        result.addProperty("unit", input.get("unit").getAsString());
        result.add("initial", snapshot(slots, ids));
        JsonArray trace = new JsonArray();
        for (var value : input.getAsJsonArray("ops")) {
            JsonObject op = value.getAsJsonObject();
            int a = number(op, "a");
            int b = number(op, "b");
            JsonObject step = new JsonObject();
            step.addProperty("result", "ok");
            switch (op.get("op").getAsString()) {
                case "alias" -> slots[b] = slots[a];
                case "copy" -> slots[b] = new MobEffectInstance(slots[a]);
                case "scale" -> slots[b] = slots[a].withScaledDuration(Float.intBitsToFloat(number(op, "factor")));
                case "update" -> step.addProperty("result", slots[a].update(slots[b]));
                case "eq" -> step.addProperty("result", slots[a].equals(slots[b]));
                case "same" -> step.addProperty("result", slots[a] == slots[b]);
                case "ends" -> step.addProperty("result", slots[a].endsWithin(number(op, "value")));
                case "infinite" -> step.addProperty("result", slots[a].isInfiniteDuration());
                case "tick" -> {
                    MobEffectInstance instance = slots[a];
                    boolean active = instance.isInfiniteDuration() || instance.getDuration() > 0;
                    boolean downgraded = false;
                    if (active) {
                        DECREMENT.invoke(instance);
                        downgraded = (boolean) DOWNGRADE.invoke(instance);
                    }
                    JsonObject tick = new JsonObject();
                    tick.addProperty("active", instance.isInfiniteDuration() || instance.getDuration() > 0);
                    tick.addProperty("downgraded", downgraded);
                    step.add("result", tick);
                }
                case "map" -> {
                    JsonArray calls = new JsonArray();
                    step.add("calls", calls);
                    try {
                        int mapped = slots[a].mapDuration(duration -> {
                            calls.add(duration);
                            if (op.get("mutate").getAsBoolean()) {
                                slots[b].update(slots[number(op, "c")]);
                            }
                            if (op.get("fail").getAsBoolean()) {
                                throw new CallbackFailure();
                            }
                            return duration * number(op, "value");
                        });
                        step.addProperty("result", mapped);
                    } catch (CallbackFailure error) {
                        step.addProperty("result", "callback");
                    }
                }
                default -> throw new IllegalArgumentException("Unknown operation: " + op);
            }
            step.add("state", snapshot(slots, ids));
            trace.add(step);
        }
        result.add("trace", trace);
        return result;
    }

    public static void main(String[] args) throws Exception {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
        effects = List.of(MobEffects.SPEED, MobEffects.POISON, MobEffects.NIGHT_VISION);
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: StatusEffectOracle run <inputs.jsonl> <outputs.jsonl>");
        }
        try (var reader = Files.newBufferedReader(Path.of(args[1])); var writer = Files.newBufferedWriter(Path.of(args[2]))) {
            for (String line; (line = reader.readLine()) != null;) {
                writer.write(GSON.toJson(run(JsonParser.parseString(line).getAsJsonObject())));
                writer.newLine();
            }
        }
    }
}
