#pragma once

#pragma once
#include "Types.h"             
#include "GenericTypeContainer.h" 
#include "FieldInfoMagic.h"     
#include "DebugBridge.h"       
#include "WinStructres.hpp"     
#include <DbgEng.h>             
#include <string>
#include <vector>
#include <expected>
#include <variant>
#include <memory>
#include "SymbolClient.h"


class DebugMagic
{
public:
    DebugMagic(const std::wstring& path);

    ~DebugMagic();

    void load_module_base_symbols(const Address module_base);

    void load_ntos_symbols();

    void load_module_symbols(const std::string& module_name); 

    Expected<Address> get_module_base(const std::string& module_name);
  
    Expected<Address> get_ntos_base();
  
    Expected<Bytes> read_memory_virtual(Address address, size_t size);
  
    Expected<Bytes> read_memory_physical(Address address, size_t size);
  
    template <typename T>
    Expected<T> read_struct_memory_virtual(Address address);
  
    Expected<Address> read_pointer_memory_virtual(Address address);
  
    template <typename T>
    Expected<T> read_struct_memory_physical(Address address);

    template <typename T>
    Expected<std::vector<T>> read_struct_array_memory_physical(Address address, size_t length);

  
    template <typename T>
    Expected<std::vector<T>> read_struct_array_memory_virtual(Address address, size_t length);

    Expected<Address> read_pointer_memory_physical(Address address);

    template<typename ValueType>
    Expected<ValueType> parse_windbg_command_value(const std::string& command,
                                                   ValueType (*parser)(std::string));

    Expected<Address> get_symbol_address(const std::string& module,
                                        const std::string& symbol);

    template<typename ValueType>
    Expected<ValueType> get_symbol_address_as_struct(const std::string& module,
                                                     const std::string& symbol);

    template<typename ValueType>
    Expected<std::vector<ValueType>> get_symbol_address_as_struct_array(const std::string& module,
                                                                        const std::string& symbol, size_t length);


    template<typename ValueType>
    Expected<ValueType> get_struct_field(const std::string& module_name,
                                         const std::string& type,
                                         const std::string& field_name,
                                         Address address);
   
    Expected<Address> get_struct_base_from_field(const std::string& module_name,
                                                 const std::string& type,
                                                 const std::string& field_name,
                                                 const Address address);


    Expected<std::shared_ptr<GenericTypeContainer>> struct_magic(const std::string& module_name,
                                                      const std::string& type,
                                                      const Address address);

    Expected<ULONG> get_type_id(ULONG64 mod,
                                const std::string& type_name);
    
    Expected<ULONG> get_type_size(ULONG64 mod,
                                  ULONG type_id);
    
    Expected<std::string> get_type_name(const ULONG64 mod, ULONG type_id);
    
    Expected<std::vector<std::string>> get_field_names(const ULONG64 mod, ULONG container_type_id);
    
    Expected<FieldInfo> get_field_info(ULONG64 mod,
                                       ULONG container_type_id,
                                       const std::string& field_name);
    

 
    template <FixedString  MODULE_NAME, FixedString TYPE_NAME>
    Expected<std::shared_ptr<GenericTypeContainer>> get_symbol_address_as(const std::string& module,
                                                         const std::string& symbol);

    template <FixedString  MODULE_NAME, FixedString TYPE_NAME, FixedString FIELD_NAME>
    Expected<std::shared_ptr<GenericTypeContainer>> get_struct_from_field_as(Address field_address); 


    Expected<std::pair<std::string,std::string>> get_symbol_from_address(Address address);

    FieldInfoMagic& get_field_info_magic();

    Expected<std::shared_ptr<GenericTypeContainer>> from_ptr(std::shared_ptr<GenericTypeContainer> container, const std::string& field);

private:
    
    std::string format_symbol_module(const std::string& module, const std::string symbol);

    SymbolClient m_symbol_client;
    MasterDebugBridge m_master_bridge;
    DebugBridge<IDebugClient6> m_client;
    DebugBridge<IDebugControl> m_control;
    DebugBridge<IDebugSymbols3> m_symbols_fields;
    DebugBridge<IDebugSymbols5> m_symbols;
    DebugBridge<IDebugDataSpaces> m_data_space;
    FieldInfoMagic m_field_magic;

};



template<typename T>
inline Expected<T> DebugMagic::read_struct_memory_virtual(Address address)
{
    Expected<Bytes> content = read_memory_virtual(address, sizeof(T));
    if (content.has_value()) {
        return *reinterpret_cast<T*>(content->data());
    }
    
    return std::unexpected(content.error());
}


template<typename T>
inline Expected<T> DebugMagic::read_struct_memory_physical(Address address)
{
    Expected<Bytes> content = read_memory_physical(address, sizeof(T));
    if (content.has_value()) {
        return *reinterpret_cast<T*>(content->data());
    }
    
    return std::unexpected(content.error());
}


template<typename T>
inline Expected<std::vector<T>> DebugMagic::read_struct_array_memory_physical(Address address, size_t length)
{
    std::vector<T> result;
    for (size_t i = 0; i < length; i++)
    {
        Expected<T> value = read_struct_memory_physical<T>(address + i * sizeof(T));
        if (!value.has_value()) {
            return std::unexpected(value.error());
        }
        result.emplace_back(value.value());
    }

    return result;
}


template<typename T>
inline Expected<std::vector<T>> DebugMagic::read_struct_array_memory_virtual(Address address, size_t length)
{
    std::vector<T> result;
    for (size_t i = 0; i < length; i++)
    {
        Expected<T> value = read_struct_memory_virtual<T>(address + i * sizeof(T));
        if (!value.has_value()) {
            return std::unexpected(value.error());
        }
        result.emplace_back(value.value());
    }

    return result;
}


template<typename ValueType>
inline Expected<ValueType> DebugMagic::parse_windbg_command_value(const std::string& command,
                                                                  ValueType(*parser)(std::string))
{
    return Expected<ValueType>();
}

template<typename ValueType>
inline Expected<ValueType> DebugMagic::get_symbol_address_as_struct(const std::string& module, const std::string& symbol)
{
    Expected<Address> symbol_address = get_symbol_address(module, symbol);
    if (!symbol_address.has_value()) {
        return std::unexpected(symbol_address.error());
    }

    
    return read_struct_memory_virtual<ValueType>(symbol_address.value());
}


template<typename ValueType>
inline Expected<std::vector<ValueType>> DebugMagic::get_symbol_address_as_struct_array(const std::string& module, const std::string& symbol, size_t length)
{
    Expected<Address> symbol_address = get_symbol_address(module, symbol);
    
    if (!symbol_address.has_value()) {
        return std::unexpected(symbol_address.error());
    }
    
    return read_struct_array_memory_virtual<ValueType>(symbol_address.value(), length);
}


template<typename ValueType>
inline Expected<ValueType> DebugMagic::get_struct_field(const std::string& module_name,
                                                        const std::string& type,
                                                        const std::string& field_name,
                                                        Address address)
{
    ValueType result = {0};
    ULONG field_offset;
    ULONG type_id;
    HRESULT hr;
    
    Expected<Address> module_base = get_module_base(module_name);
    if (!module_base.has_value()) {
        return std::unexpected(module_base.error());
    
    }

    hr = m_symbols_fields.get_interface()->GetTypeId(module_base.value(), type.c_str(), &type_id);
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldn't get symbol type id"));
    
    }
    m_symbols_fields.get_interface()->GetFieldOffset(module_base.value(), type_id, field_name.c_str(), &field_offset);
    if (FAILED(hr)) {

        return std::unexpected(std::exception("Couldn't read symbol"));
    }

    return read_struct_memory_virtual<ValueType>(address + field_offset);
}


template<FixedString  MODULE_NAME, FixedString  TYPE_NAME>
inline Expected<std::shared_ptr<GenericTypeContainer>> DebugMagic::get_symbol_address_as(const std::string& module,
                                                                        const std::string& symbol)
{
    Expected<Address> sym_address = get_symbol_address(module, symbol);
    if (!sym_address.has_value()) {
        return std::unexpected(sym_address.error());
    }

    std::string module_name = MODULE_NAME.view().data();
    std::string type_name = TYPE_NAME.view().data();

    return struct_magic(module_name, type_name, sym_address.value());
}


template<FixedString MODULE_NAME, FixedString TYPE_NAME, FixedString FIELD_NAME>
inline Expected<std::shared_ptr<GenericTypeContainer>> DebugMagic::get_struct_from_field_as(Address field_address)
{
    std::string module_name = MODULE_NAME.view().data();
    std::string type_name = TYPE_NAME.view().data();
    std::string field_name = FIELD_NAME.view().data();

    Expected<Address> struct_address = get_struct_base_from_field(module_name, type_name, field_name, field_address);
    if (!struct_address.has_value()) {
        return std::unexpected(struct_address.error());
    }


     return struct_magic(module_name, type_name, struct_address.value());
}
