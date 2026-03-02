#pragma once
#include "memory"
#include "string"
#include "DbgEng.h"
#include "DebugBridge.h"
#include <expected>
#include <vector>
#include "WinStructres.hpp"


using Bytes = std::vector<std::byte>;

template <typename T>
using Expected = std::expected<T,std::exception>;


using Address = uintptr_t;

class DebugMagic
{
public:
   DebugMagic(std::wstring path);
  ~DebugMagic();

  void load_module_base_symbols(Address module_base);

  void load_ntos_symbols();

  void load_module_symbols(std::string module_name); 

  Expected<Address> get_module_base(std::string module_name);
  
  Expected<Address> get_ntos_base();
  
  Expected<Bytes> read_memory_virtual(Address address, size_t size);
  
  Expected<Bytes> read_memory_physical(Address address, size_t size);
  
  template <typename T>
  Expected<T> read_struct_memory_virtual(Address address);
  
  Expected<Address> read_pointer_memory_virtual(Address address);
  
  template <typename T>
  Expected<T> read_struct_memory_physical(Address address);
  
  Expected<Address> read_pointer_memory_physical(Address address);

  template<typename ValueType>
  Expected<ValueType> parse_windbg_command_value(std::string command, ValueType (*parser)(std::string));

   Expected<Address> get_symbol_address(std::string symbol);

   template<typename ValueType>
   Expected<ValueType> get_struct_field(std::string module_name, std::string type, std::string field_name, Address address);
   
   Expected<Address> get_struct_base_from_field(std::string module_name, std::string type, std::string field_name, Address address);


private:
    
    Expected<CV_INFO_PDB70> get_pdb_info_for_module_base(Address module_base);

    MasterDebugBridge m_master_bridge;
    DebugBridge<IDebugClient6> m_client;
    DebugBridge<IDebugControl> m_control;
    DebugBridge<IDebugSymbols3> m_symbols_fields;
    DebugBridge<IDebugSymbols5> m_symbols;
    DebugBridge<IDebugDataSpaces> m_data_space;


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

template<typename ValueType>
inline Expected<ValueType> DebugMagic::parse_windbg_command_value(std::string command, ValueType(*parser)(std::string))
{
    return Expected<ValueType>();
}


template<typename ValueType>
inline Expected<ValueType> DebugMagic::get_struct_field(std::string module_name, std::string type, std::string field_name, Address address)
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
