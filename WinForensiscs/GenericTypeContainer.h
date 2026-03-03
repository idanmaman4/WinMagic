#pragma once

#include <string>
#include <memory>
#include <format>
#include <variant>
#include <map>
#include <ostream>
#include <sstream>
#include <algorithm>
#include "Types.h"

class GenericTypeContainer;

struct TypedValue
{
    std::string type_name;
    std::variant<
        uint8_t, uint16_t, uint32_t, uint64_t,
        int8_t,  int16_t,  int32_t,  int64_t,
        float,   double,
        std::string,
        std::wstring,
        Bytes,
        std::shared_ptr<GenericTypeContainer>
    > value;
};

class GenericTypeContainer
{
public:
    std::string struct_type;

    Address address() {
        return m_address;
    }

    explicit GenericTypeContainer(const std::string& type, Address address) : m_address(address), struct_type(type) {
    
    }

    void set(const std::string& field, TypedValue val)
    {
        m_fields[field] = std::move(val);
    }

    const TypedValue* get(const std::string& field) const
    {
        auto it = m_fields.find(field);
        return it != m_fields.end() ? &it->second : nullptr;
    }

    template <typename T>
    const T get(const std::string& field) const
    {
        auto it = m_fields.find(field);
        if (it == m_fields.end())
            throw std::out_of_range(std::format("field '{}' not found", field));

        return std::get<T>(it->second.value);
    }

    std::string to_string(int indent = 0) const
    {
        std::string pad(indent * 2, ' ');
        std::string inner((indent + 1) * 2, ' ');
        std::string out;

        out += std::format("('{}', {{\n", struct_type);

        size_t i = 0;
        for (auto& [name, typed] : m_fields)
        {
            out += inner + std::format("'{}': {}", name, value_to_string(typed, indent + 1));
            if (++i < m_fields.size()) out += ",";
            out += "\n";
        }

        out += pad + "})";
        return out;
    }

private:
    std::map<std::string, TypedValue> m_fields;
    Address m_address;

    static std::string value_to_string(const TypedValue& typed, int indent)
    {
        return std::visit([&](auto&& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, std::shared_ptr<GenericTypeContainer>>) {
                return v ? v->to_string(indent) 
                         : std::format("('{}', null)", typed.type_name);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return std::format("('{}', '{}')", typed.type_name, v);
            }
            else if constexpr (std::is_same_v<T, std::wstring>) {
                return std::format("('{}', '{}')", typed.type_name, 
                                   std::string(v.begin(), v.end()));
            }
            else if constexpr (std::is_same_v<T, Bytes>) {
                std::string hex;
                size_t limit = (std::min)(v.size(), size_t(8));
                for (size_t i = 0; i < limit; i++)
                    hex += std::format("{:02X}", static_cast<uint8_t>(v[i]));
                if (v.size() > 8) hex += "...";
                return std::format("('{}', 0x{})", typed.type_name, hex);
            }
            else if constexpr (std::is_same_v<T, uint64_t>) {
                if (typed.type_name.ends_with('*') || typed.type_name.starts_with("Ptr"))
                    return std::format("('{}', 0x{:016X})", typed.type_name, v);
                return std::format("('{}', {})", typed.type_name, v);
            }
            else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                return std::format("('{}', {})", typed.type_name, v);
            }
            else {
                return std::format("('{}', {})", typed.type_name, v);
            }
        }, typed.value);
    }
};

// Defined AFTER GenericTypeContainer is fully defined so v->to_string() is valid
inline std::ostream& operator<<(std::ostream& os, const TypedValue& tv)
{
    std::visit([&](auto&& v)
    {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::string>) {
            os << v;
        }
        else if constexpr (std::is_same_v<T, std::wstring>) {
            os << std::string(v.begin(), v.end());
        }
        else if constexpr (std::is_same_v<T, Bytes>) {
            os << "[" << v.size() << " bytes: ";
            size_t limit = (std::min)(v.size(), size_t(8));
            for (size_t i = 0; i < limit; i++)
                os << std::format("{:02X} ", static_cast<uint8_t>(v[i]));
            if (v.size() > 8) os << "...";
            os << "]";
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<GenericTypeContainer>>) {
            os << (v ? v->to_string() : "null");
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
            if (tv.type_name.ends_with('*') || tv.type_name.starts_with("Ptr"))
                os << std::format("0x{:016X}", v);
            else
                os << v;
        }
        else {
            os << v;
        }
    }, tv.value);

    return os;
}