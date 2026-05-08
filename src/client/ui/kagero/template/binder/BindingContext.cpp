#include "BindingContext.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace mc::client::ui::kagero::tpl::binder {

// ========== Value实现 ==========

Value Value::fromAny(const std::any& any) {
    if (!any.has_value()) {
        return Value();
    }

    const std::type_info& type = any.type();

    if (type == typeid(bool)) {
        return Value(std::any_cast<bool>(any));
    }
    if (type == typeid(i32)) {
        return Value(std::any_cast<i32>(any));
    }
    if (type == typeid(i64)) {
        return Value(static_cast<i32>(std::any_cast<i64>(any)));
    }
    if (type == typeid(u32)) {
        return Value(static_cast<i32>(std::any_cast<u32>(any)));
    }
    if (type == typeid(f32)) {
        return Value(std::any_cast<f32>(any));
    }
    if (type == typeid(f64)) {
        return Value(static_cast<f32>(std::any_cast<f64>(any)));
    }
    if (type == typeid(std::string)) {
        return Value(std::any_cast<std::string>(any));
    }
    if (type == typeid(const char*)) {
        return Value(std::string(std::any_cast<const char*>(any)));
    }

    // 不支持的类型
    return Value();
}

bool Value::asBool() const {
    switch (m_type) {
        case ValueType::Bool: return m_boolValue;
        case ValueType::Integer: return m_intValue != 0;
        case ValueType::Float: return m_floatValue != 0.0f;
        case ValueType::String: return !m_stringValue.empty() && m_stringValue != "false";
        default: return false;
    }
}

i32 Value::asInteger() const {
    switch (m_type) {
        case ValueType::Bool: return m_boolValue ? 1 : 0;
        case ValueType::Integer: return m_intValue;
        case ValueType::Float: return static_cast<i32>(m_floatValue);
        case ValueType::String: {
            try {
                return std::stoi(m_stringValue);
            } catch (...) {
                return 0;
            }
        }
        default: return 0;
    }
}

f32 Value::asFloat() const {
    switch (m_type) {
        case ValueType::Bool: return m_boolValue ? 1.0f : 0.0f;
        case ValueType::Integer: return static_cast<f32>(m_intValue);
        case ValueType::Float: return m_floatValue;
        case ValueType::String: {
            try {
                return std::stof(m_stringValue);
            } catch (...) {
                return 0.0f;
            }
        }
        default: return 0.0f;
    }
}

const std::string& Value::asString() const {
    static const std::string empty;
    return m_type == ValueType::String ? m_stringValue : empty;
}

std::string Value::toString() const {
    switch (m_type) {
        case ValueType::Null: return "null";
        case ValueType::Bool: return m_boolValue ? "true" : "false";
        case ValueType::Integer: return std::to_string(m_intValue);
        case ValueType::Float: return std::to_string(m_floatValue);
        case ValueType::String: return m_stringValue;
        case ValueType::Array: {
            std::ostringstream oss;
            oss << "[";
            for (size_t i = 0; i < m_arrayValue.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << m_arrayValue[i].toString();
            }
            oss << "]";
            return oss.str();
        }
        case ValueType::Object: {
            std::ostringstream oss;
            oss << "{";
            bool first = true;
            for (const auto& [key, value] : m_objectValue) {
                if (!first) oss << ", ";
                first = false;
                oss << key << ": " << value.toString();
            }
            oss << "}";
            return oss.str();
        }
        default: return "";
    }
}

i32 Value::toInteger() const {
    return asInteger();
}

f32 Value::toFloat() const {
    return asFloat();
}

bool Value::toBool() const {
    return asBool();
}

size_t Value::arraySize() const {
    return m_type == ValueType::Array ? m_arrayValue.size() : 0;
}

Value Value::arrayGet(size_t index) const {
    if (m_type != ValueType::Array || index >= m_arrayValue.size()) {
        return Value();
    }
    return m_arrayValue[index];
}

Value Value::getProperty(const std::string& name) const {
    // 对象类型支持属性访问
    if (m_type == ValueType::Object) {
        auto it = m_objectValue.find(name);
        if (it != m_objectValue.end()) {
            return it->second;
        }
        return Value();
    }

    // 数组类型支持特殊属性
    if (m_type == ValueType::Array) {
        if (name == "length" || name == "size") {
            return Value(static_cast<i32>(m_arrayValue.size()));
        }
        // 支持数字索引作为属性
        try {
            size_t index = static_cast<size_t>(std::stoul(name));
            if (index < m_arrayValue.size()) {
                return m_arrayValue[index];
            }
        } catch (...) {
            // 不是数字，忽略
        }
        return Value();
    }

    // 字符串类型支持特殊属性
    if (m_type == ValueType::String) {
        if (name == "length" || name == "size") {
            return Value(static_cast<i32>(m_stringValue.size()));
        }
        if (name == "empty") {
            return Value(m_stringValue.empty());
        }
        return Value();
    }

    return Value();
}

void Value::setProperty(const std::string& name, const Value& value) {
    // 确保是对象类型
    if (m_type != ValueType::Object) {
        m_type = ValueType::Object;
        m_objectValue.clear();
    }
    m_objectValue[name] = value;
}

bool Value::hasProperty(const std::string& name) const {
    if (m_type == ValueType::Object) {
        return m_objectValue.find(name) != m_objectValue.end();
    }
    return false;
}

Value Value::getElement(size_t index) const {
    // 对于数组类型，支持索引访问
    if (m_type == ValueType::Array && index < m_arrayValue.size()) {
        return m_arrayValue[index];
    }
    return Value();
}

bool Value::operator==(const Value& other) const {
    if (m_type != other.m_type) {
        // 尝试类型转换比较
        if (isNumber() && other.isNumber()) {
            return asFloat() == other.asFloat();
        }
        return false;
    }

    switch (m_type) {
        case ValueType::Null: return true;
        case ValueType::Bool: return m_boolValue == other.m_boolValue;
        case ValueType::Integer: return m_intValue == other.m_intValue;
        case ValueType::Float: return m_floatValue == other.m_floatValue;
        case ValueType::String: return m_stringValue == other.m_stringValue;
        case ValueType::Array: return m_arrayValue == other.m_arrayValue;
        case ValueType::Object: return m_objectValue == other.m_objectValue;
        default: return false;
    }
}

// ========== BindingContext实现 ==========

BindingContext::BindingContext(state::StateStore& store, event::EventBus& eventBus)
    : m_store(store)
    , m_eventBus(eventBus) {
}

void BindingContext::exposeCallback(const std::string& name, Callback callback) {
    m_callbacks[name] = std::move(callback);
}

void BindingContext::exposeSimpleCallback(const std::string& name, std::function<void()> callback) {
    m_callbacks[name] = [callback](widget::Widget*, const event::Event&) {
        callback();
    };
}

bool BindingContext::hasCallback(const std::string& name) const {
    return m_callbacks.find(name) != m_callbacks.end();
}

bool BindingContext::invokeCallback(const std::string& name, widget::Widget* source,
                                     const event::Event& event) {
    auto it = m_callbacks.find(name);
    if (it == m_callbacks.end()) {
        return false;
    }

    it->second(source, event);
    return true;
}

Value BindingContext::resolveBinding(const std::string& path,
                                      const std::string& loopVar,
                                      const Value& loopValue) const {
    if (path.empty()) {
        return Value();
    }

    // 检查是否是循环变量引用 ($varName 或 $varName.property)
    if (path.size() > 1 && path[0] == '$') {
        // 解析变量名和属性
        size_t dotPos = path.find('.');
        std::string varName;
        std::string property;

        if (dotPos != std::string::npos) {
            varName = path.substr(1, dotPos - 1);
            property = path.substr(dotPos + 1);
        } else {
            varName = path.substr(1);
        }

        // 如果是当前循环变量且提供了非空值
        if (!loopVar.empty() && varName == loopVar && !loopValue.isNull()) {
            if (property.empty()) {
                return loopValue;
            }
            return loopValue.getProperty(property);
        }

        // 查找循环变量表（包括当前循环变量从表中获取的情况）
        auto loopIt = m_loopVariables.find(varName);
        if (loopIt != m_loopVariables.end()) {
            if (property.empty()) {
                return loopIt->second;
            }
            return loopIt->second.getProperty(property);
        }

        return Value();
    }

    // 查找暴露的变量
    auto it = m_exposedVars.find(path);
    if (it != m_exposedVars.end()) {
        return it->second.readFunc();
    }

    // 尝试从StateStore获取
    if (m_store.has(path)) {
        // 由于StateStore使用std::any，我们需要尝试几种常见类型
        // 这里简化处理，实际使用时需要更完善的类型支持
        return Value(); // 暂时返回空值，后续扩展StateStore的类型支持
    }

    // 尝试路径解析（嵌套属性）
    return resolvePath(path);
}

bool BindingContext::setBinding(const std::string& path, const Value& value) {
    auto it = m_exposedVars.find(path);
    if (it == m_exposedVars.end() || !it->second.isWritable) {
        return false;
    }

    if (it->second.writeFunc) {
        it->second.writeFunc(value);
        return true;
    }

    return false;
}

bool BindingContext::hasPath(const std::string& path) const {
    // 检查循环变量
    if (path.size() > 1 && path[0] == '$') {
        size_t dotPos = path.find('.');
        std::string varName = (dotPos != std::string::npos) ? path.substr(1, dotPos - 1) : path.substr(1);
        return m_loopVariables.find(varName) != m_loopVariables.end();
    }

    // 检查暴露的变量
    if (m_exposedVars.find(path) != m_exposedVars.end()) {
        return true;
    }

    // 检查StateStore
    return m_store.has(path);
}

bool BindingContext::isWritable(const std::string& path) const {
    auto it = m_exposedVars.find(path);
    return it != m_exposedVars.end() && it->second.isWritable;
}

void BindingContext::notifyChange(const std::string& path, const Value& newValue) {
    // 通知订阅者
    auto it = m_subscribers.find(path);
    if (it != m_subscribers.end()) {
        for (const auto& [id, callback] : it->second) {
            callback(path, newValue);
        }
    }

    // 调用变量的更新回调
    auto varIt = m_exposedVars.find(path);
    if (varIt != m_exposedVars.end() && varIt->second.onUpdate) {
        varIt->second.onUpdate(path, newValue);
    }
}

u64 BindingContext::subscribe(const std::string& path, StateChangeCallback callback) {
    u64 id = m_nextSubscriberId++;
    m_subscribers[path].emplace_back(id, std::move(callback));
    return id;
}

void BindingContext::unsubscribe(u64 id) {
    for (auto& [path, subscribers] : m_subscribers) {
        auto it = std::remove_if(subscribers.begin(), subscribers.end(),
                                 [id](const auto& pair) { return pair.first == id; });
        subscribers.erase(it, subscribers.end());
    }
}

void BindingContext::setLoopVariable(const std::string& varName, const Value& value) {
    m_loopVariables[varName] = value;
}

void BindingContext::clearLoopVariable(const std::string& varName) {
    m_loopVariables.erase(varName);
}

Value BindingContext::getLoopVariable(const std::string& varName) const {
    auto it = m_loopVariables.find(varName);
    return it != m_loopVariables.end() ? it->second : Value();
}

bool BindingContext::hasLoopVariable(const std::string& varName) const {
    return m_loopVariables.find(varName) != m_loopVariables.end();
}

std::vector<Value> BindingContext::resolveCollection(const std::string& path) const {
    std::vector<Value> result;

    // 首先尝试从循环变量获取
    if (path.size() > 1 && path[0] == '$') {
        // 解析变量名和属性
        size_t dotPos = path.find('.');
        std::string varName;
        std::string property;

        if (dotPos != std::string::npos) {
            varName = path.substr(1, dotPos - 1);
            property = path.substr(dotPos + 1);
        } else {
            varName = path.substr(1);
        }

        auto loopIt = m_loopVariables.find(varName);
        if (loopIt != m_loopVariables.end()) {
            const Value& val = loopIt->second;
            if (val.isArray()) {
                // 如果本身就是数组，直接返回
                if (property.empty()) {
                    return val.asArray();
                }
                // 如果有属性访问，对每个元素获取属性
                for (const auto& item : val.asArray()) {
                    result.push_back(item.getProperty(property));
                }
                return result;
            }
        }
    }

    // 尝试从暴露的变量获取
    auto it = m_exposedVars.find(path);
    if (it != m_exposedVars.end()) {
        Value val = it->second.readFunc();
        if (val.isArray()) {
            return val.asArray();
        }
        // 单个值包装成数组
        if (!val.isNull()) {
            result.push_back(val);
        }
        return result;
    }

    // 尝试路径解析
    Value resolved = resolvePath(path);
    if (resolved.isArray()) {
        return resolved.asArray();
    }
    if (!resolved.isNull()) {
        result.push_back(resolved);
    }

    return result;
}

void BindingContext::setCollectionValue(const std::string& name, const std::vector<Value>& values) {
    m_exposedVars[name] = ExposedVar{
        nullptr, 0, "", false,
        [values]() -> Value { return Value(values); },
        nullptr, nullptr
    };
}

void BindingContext::clear() {
    m_exposedVars.clear();
    m_callbacks.clear();
    m_subscribers.clear();
    m_loopVariables.clear();
}

Value BindingContext::resolvePath(const std::string& path) const {
    // 分割路径并逐层解析
    std::vector<std::string> parts = splitPath(path);

    if (parts.empty()) {
        return Value();
    }

    // 第一部分应该是暴露的变量或StateStore中的键
    std::string rootKey = parts[0];
    Value current;

    // 检查是否是数组索引作为第一个部分
    if (!rootKey.empty() && rootKey[0] == '[') {
        // 以数组索引开头，需要找到上下文
        return Value();
    }

    // 检查暴露的变量
    auto it = m_exposedVars.find(rootKey);
    if (it != m_exposedVars.end()) {
        current = it->second.readFunc();
    } else if (m_store.has(rootKey)) {
        // 从StateStore获取（简化处理）
        current = Value();
    } else {
        return Value();
    }

    // 遍历剩余路径
    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string& part = parts[i];

        // 检查是否是数组索引
        if (!part.empty() && part[0] == '[' && part.back() == ']') {
            std::string indexStr = part.substr(1, part.size() - 2);
            try {
                size_t index = static_cast<size_t>(std::stoul(indexStr));
                current = current.getElement(index);
            } catch (...) {
                return Value();
            }
        } else {
            current = current.getProperty(part);
        }

        if (current.isNull()) {
            return Value();
        }
    }

    return current;
}

std::vector<std::string> BindingContext::splitPath(const std::string& path) const {
    std::vector<std::string> parts;
    std::string current;
    bool inBracket = false;

    for (char c : path) {
        if (c == '.') {
            if (!inBracket && !current.empty()) {
                parts.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        } else if (c == '[') {
            if (!inBracket) {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
                current += c;
                inBracket = true;
            } else {
                current += c;
            }
        } else if (c == ']') {
            if (inBracket) {
                current += c;
                inBracket = false;
                parts.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        parts.push_back(current);
    }

    return parts;
}

} // namespace mc::client::ui::kagero::tpl::binder
