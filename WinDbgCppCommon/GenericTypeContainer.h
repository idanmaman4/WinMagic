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
#include <optional>

class GenericTypeContainer;




struct TypedValue
{
    using ValueType =  std::variant<
        
        std::vector<uint8_t>, std::vector<uint16_t>, std::vector<uint32_t>, std::vector<uint64_t>,
        std::vector<int8_t>,  std::vector<int16_t>, std::vector<int32_t>,  std::vector<int64_t>,
        std::vector<float>,   std::vector<double>,

 
        // data
        uint8_t, uint16_t, uint32_t, uint64_t,
        int8_t,  int16_t,  int32_t,  int64_t,
        float,   double,

        std::vector<TypedValue>,
        std::shared_ptr<GenericTypeContainer>
    > ;

    TypedValue(std::string module_name, const std::string& type_name, ValueType value) : s_module_name(module_name), s_type_name(type_name), s_value(value)
    {
    
    }

    std::string s_module_name; // if modulename is "" then it is basic type...
    std::string s_type_name; // we can further resolve it...
    ValueType s_value;
};

class GenericTypeContainer
{
public:


    Address address();

    explicit GenericTypeContainer(const std::string& module_name, const std::string& type, Address address);

    void set(const std::string& field, TypedValue val);

    const std::optional<TypedValue> get(const std::string& field) const;

    template <typename T>
    std::optional<T> get(const std::string& field) const;

    Expected<Address> as_pointer(const std::string& field);

    Expected<std::string> as_string(const std::string& field);

    Expected<std::wstring> as_wstring(const std::string& field);

    Expected<uint64_t> as_number_unsigned(const std::string& field);

    Expected<int64_t> as_number_signed(const std::string& field);

    Expected<double> as_double(const std::string& field);

    Expected<float> as_float(const std::string& field);

    Expected<std::shared_ptr<GenericTypeContainer>> as_object(const std::string& field);
    
private:

    std::string m_struct_type;
    const std::string& m_module_name;

    std::map<std::string, TypedValue> m_fields;
    Address m_address;
};




template <typename T>
 std::optional<T> GenericTypeContainer::get(const std::string& field) const
{
    auto it = m_fields.find(field);
    if (it == m_fields.end()) {
        return std::nullopt;
    }

    return std::get<T>(it->second.s_value);
}
