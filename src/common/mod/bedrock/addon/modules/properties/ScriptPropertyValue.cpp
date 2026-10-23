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

#include "common/mod/bedrock/addon/modules/properties/ScriptPropertyValue.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

namespace mc::mod::bedrock::addon {

Result<nlohmann::json> readScriptPropertyValue(IScriptBindingContext& context, void* value)
{
    if (context.isException(value)) {
        return Error(ErrorCode::InvalidArgument, "Script property getter threw an exception");
    }
    switch (context.getType(value)) {
        case ScriptType::Undefined:
        case ScriptType::Null:
            return nlohmann::json(nullptr);
        case ScriptType::Boolean:
            return nlohmann::json(*context.toBool(value));
        case ScriptType::Number:
            return nlohmann::json(*context.toFloat64(value));
        case ScriptType::String:
            return nlohmann::json(*context.toString(value));
        case ScriptType::Object: {
            nlohmann::json vector = nlohmann::json::object();
            for (const char* coordinate : {"x", "y", "z"}) {
                void* member = context.getProperty(value, coordinate);
                if (context.isException(member)) {
                    context.releaseValue(member);
                    return Error(ErrorCode::InvalidArgument, "Vector3 getter threw an exception");
                }
                const bool numeric = context.isNumber(member);
                const auto number = numeric ? context.toFloat64(member) : std::nullopt;
                context.releaseValue(member);
                if (!number) {
                    return Error(ErrorCode::InvalidArgument, "Property object must be a Vector3");
                }
                vector[coordinate] = *number;
            }
            return vector;
        }
        default:
            return Error(ErrorCode::InvalidArgument, "Unsupported property value type");
    }
}

void* writeScriptPropertyValue(IScriptBindingContext& context, const nlohmann::json& value)
{
    if (value.is_boolean()) {
        return context.createBoolean(value.get<bool>());
    }
    if (value.is_string()) {
        return context.createString(value.get_ref<const std::string&>());
    }
    if (value.is_number()) {
        return context.createFloat64(value.get<double>());
    }

    void* vector = context.createObject();
    for (const char* coordinate : {"x", "y", "z"}) {
        context.setPropertyFloat(vector, coordinate, value[coordinate].get<double>());
    }
    return vector;
}

bool hasScriptPropertyName(IScriptBindingContext& context, i32 argc, void** args)
{
    return argc > 0 && context.isString(args[0]) && !context.toString(args[0])->empty();
}

void* throwScriptPropertyError(IScriptBindingContext& context, const char* message)
{
    if (context.hasException()) {
        void* exception = context.getException();
        void* result = context.throwValue(exception);
        context.releaseValue(exception);
        return result;
    }
    return context.throwTypeError(message);
}

} // namespace mc::mod::bedrock::addon
