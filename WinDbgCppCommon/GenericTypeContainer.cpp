#include "GenericTypeContainer.h"

using namespace std;


Address GenericTypeContainer::address() {
    return m_address;
}


GenericTypeContainer::GenericTypeContainer(const std::string& module_name, const std::string& type_name, Address address) : m_address(address), m_struct_type(type_name),m_module_name(module_name)  {
    
}


void GenericTypeContainer::set(const std::string& field, TypedValue val)
{
    m_fields.insert_or_assign(field, std::move(val));
}


const std::optional<TypedValue> GenericTypeContainer::get(const std::string& field) const
{
    auto it = m_fields.find(field);
    if (it != m_fields.end()) {
        return it->second;
    }
    return std::nullopt;
}


Expected<Address> GenericTypeContainer::as_pointer(const std::string& field) {
    Expected<Address> address = std::visit([](auto&& v) -> Expected<Address>{
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, uint32_t>) return static_cast<Address>(v);
        if constexpr (std::is_same_v<T, uint64_t>) return static_cast<Address>(v);
        return unexpected(exception("Can't convert or find requested field to pointer"));
    }, m_fields.at(field).s_value);

    if (!address.has_value()) 
        return address;

    // Canonical address check (x86-64: bits [63:48] must sign-extend bit 47)
    constexpr Address CANONICAL_MASK = 0xFFFF800000000000ULL;
    Address sign_bits = *address & CANONICAL_MASK;

    bool is_canonical = (sign_bits == 0ULL) ||           
                        (sign_bits == CANONICAL_MASK);  

    if (!is_canonical)
        return unexpected(exception("Address is not Canonial"));

    return address;
}


Expected<std::string> GenericTypeContainer::as_string(const std::string& field) {
    return std::visit([](auto&& v) -> Expected<std::string> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::vector<int8_t>>)  return std::string(v.begin(), v.end());
        if constexpr (std::is_same_v<T, std::vector<uint8_t>>)  return std::string(v.begin(), v.end());
        return unexpected(exception("Can't convert to string or find request field"));
    }, m_fields.at(field).s_value);
}


Expected<std::wstring> GenericTypeContainer::as_wstring(const std::string& field) {
    return std::visit([](auto&& v) -> Expected<wstring> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::vector<int16_t>>) return std::wstring(v.begin(), v.end());
        if constexpr (std::is_same_v<T, std::vector<uint16_t>>)  return std::wstring(v.begin(), v.end());
        return unexpected(exception("Can't convert to wstring or find request field"));
    }, m_fields.at(field).s_value);
}


Expected<uint64_t> GenericTypeContainer::as_number_unsigned(const std::string& field) {
    return std::visit([](auto&& v) -> Expected<uint64_t> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, uint8_t>  || std::is_same_v<T, uint16_t> ||
                        std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>)
            return static_cast<uint64_t>(v);
        return unexpected(exception("Can't convert to unsigned number or find request field"));
    }, get(field)->s_value);
}


Expected<int64_t> GenericTypeContainer::as_number_signed(const std::string& field) {
    return std::visit([](auto&& v) -> Expected<int64_t> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int8_t>  || std::is_same_v<T, int16_t> ||
                        std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>)
            return static_cast<int64_t>(v);
        return unexpected(exception("Can't convert to singed number or find request field"));
    }, get(field)->s_value);
}


Expected<double> GenericTypeContainer::as_double(const std::string& field) {
        return std::visit([](auto&& v) -> Expected<double> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>  || std::is_same_v<T, float> )
            return static_cast<double>(v);
        return unexpected(exception("Can't convert to double or find request field"));
    }, get(field)->s_value);
}

  
Expected<float> GenericTypeContainer::as_float(const std::string& field) {
    return std::visit([](auto&& v) -> Expected<float> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, float>)  return v;
        if constexpr (std::is_same_v<T, double>) return static_cast<float>(v);
        return unexpected(exception("Can't convert to float or find request field"));
    }, m_fields.at(field).s_value);
}

Expected<shared_ptr<GenericTypeContainer>> GenericTypeContainer::as_object(const std::string& field) {
    return std::visit([](auto&& v) -> Expected<shared_ptr<GenericTypeContainer>> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, shared_ptr<GenericTypeContainer>>)  return v;
        return unexpected(exception("Can't convert to float or find request field"));
    }, m_fields.at(field).s_value);
}

