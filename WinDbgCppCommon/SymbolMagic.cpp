#include "SymbolMagic.h"
#include "WinStructres.hpp"
using namespace std;



SymbolMagic::SymbolMagic(MasterDebugBridge& master_interface) : m_symbols(master_interface), m_symbols_fields(master_interface)
{
}


void SymbolMagic::reload_symbol_path(std::filesystem::path symbol_path)
{
    m_symbols.get_interface()->SetSymbolOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS | SYMOPT_DEBUG);
    m_symbols.get_interface()->SetSymbolPathWide(symbol_path.c_str());
    m_symbols.get_interface()->Reload("");
}


Expected<Address> SymbolMagic::get_module_base(const std::string& module_name)
{
    Address module_base;
    HRESULT hr = m_symbols.get_interface()->GetModuleByModuleName(module_name.c_str(), 0, nullptr, &module_base);
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldnt communicate with bridge"));
    }
    return module_base;
}


Expected<Address> SymbolMagic::get_ntos_base()
{
    return get_module_base("nt");
}



Expected<Address> SymbolMagic::get_symbol_address(const std::string& module, const std::string& symbol)
{
    Address result = {0};
    HRESULT hr = m_symbols.get_interface()->GetOffsetByName(format_symbol_module(module, symbol).c_str(),&result);
    if (FAILED(hr)) {

        return unexpected(exception("Couldn't read symbol"));
    }

    return result;
}


Expected<std::pair<std::string,std::string>> SymbolMagic::get_symbol_from_address(Address address)
{
    std::string name(MAX_PATH,'\0');

    HRESULT hr = m_symbols.get_interface()->GetNameByOffset(address, name.data(), name.size(), nullptr, nullptr);
    if (FAILED(hr)) {
        return unexpected(exception("Couldn't read symbol from address"));
    }

    size_t sep = name.find('!');
    if (sep == std::string::npos)
        return std::unexpected(std::exception("No module separator found"));

    name.resize(strlen(name.data()));
    
    return std::make_pair(name.substr(0, sep), name.substr(sep + 1));
}


std::string SymbolMagic::format_symbol_module(const std::string& module, const std::string symbol)
{
    return format("{}!{}",module,symbol);
}


Expected<Address> SymbolMagic::get_struct_base_from_field(const std::string& module_name, 
                                                         const std::string& type,
                                                         const std::string& field_name, 
                                                         Address address)
{
    Address result = {0};
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

    return address - field_offset;
}


Expected<ULONG> SymbolMagic::get_field_offset(const std::string& module_name, const std::string& type_name, const std::string& field_name)
{
    ULONG field_offset;

    Expected<Address> module_base = get_module_base(module_name);
    if (!module_base.has_value()) {
        return std::unexpected(module_base.error());
    
    }


    Expected<ULONG> type_id = get_type_id(*module_base, type_name);
    if (!type_id.has_value()) {
        return std::unexpected(type_id.error());
    }

    return get_field_offset(*module_base, *type_id, field_name);
}


Expected<ULONG> SymbolMagic::get_field_offset(Address module_base, ULONG type_id, const std::string& field_name)
{   
	ULONG field_offset; 
	HRESULT hr = m_symbols_fields.get_interface()->GetFieldOffset(module_base, type_id, field_name.c_str(), &field_offset);
    if (FAILED(hr)) {

        return std::unexpected(std::exception("Couldn't read symbol"));
    }
    return field_offset;
}


Expected<ULONG> SymbolMagic::get_type_id(ULONG64 mod,
                                         const std::string& type_name)
{
    ULONG type_id = 0;
    HRESULT hr = m_symbols_fields.get_interface()->GetTypeId(mod, type_name.c_str(), &type_id);
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldn't get type id"));
    }

    return type_id;
}


Expected<ULONG> SymbolMagic::get_type_size(ULONG64 mod,
                                          ULONG type_id)
{
    ULONG size = 0;
    HRESULT hr = m_symbols_fields.get_interface()->GetTypeSize(mod, type_id, &size);
    return FAILED(hr) 
        ? std::unexpected(std::exception("Couldn't get type size")) 
        : Expected<ULONG>{ size };
}


Expected<std::string> SymbolMagic::get_type_name(ULONG64 mod,
                                                ULONG type_id)
{
    std::string buf(MAX_PATH, '\0');

    HRESULT hr = m_symbols_fields.get_interface()->GetTypeName(mod, type_id, buf.data(), buf.size(), nullptr);
    
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldn't get type name"));
    }

    buf.resize(strlen(buf.data()));

    return buf;
}


Expected<FieldInfo> SymbolMagic::get_field_info(ULONG64 mod,
                                               ULONG container_type_id,
                                               const std::string& field_name)
{
    auto* sym = m_symbols_fields.get_interface();

    ULONG field_type_id = 0, field_offset = 0;
    if (FAILED(sym->GetFieldTypeAndOffset(mod, container_type_id, field_name.c_str(), &field_type_id, &field_offset)))
        return std::unexpected(std::exception("Couldn't get field type and offset"));

    auto type_name = get_type_name(mod, field_type_id);
    if (!type_name.has_value())
        return std::unexpected(type_name.error());

    auto type_size = get_type_size(mod, field_type_id);
    if (!type_size.has_value())
        return std::unexpected(type_size.error());

    return FieldInfo {
        .name       = field_name,
        .type_name  = type_name.value(),
        .type_id    = field_type_id,
        .offset     = field_offset,
        .size       = type_size.value()
    };
}


Expected<std::vector<std::string>> SymbolMagic::get_field_names(ULONG64 mod,
                                                               ULONG container_type_id)
{
    std::vector<std::string> names;

    for (ULONG i = 0; ; i++)
    {
        char buf[256] = {};
        if (FAILED(m_symbols_fields.get_interface()->GetFieldName(mod, container_type_id, i, buf, sizeof(buf), nullptr)))
            break;
        names.emplace_back(buf);
    }


    return names;
}