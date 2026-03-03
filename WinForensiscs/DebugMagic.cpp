#include "DebugMagic.h"


using namespace std;


DebugMagic::DebugMagic(const std::wstring& dump_name) : m_master_bridge(),
                                                 m_client(m_master_bridge),
                                                 m_control(m_master_bridge),
                                                 m_symbols(m_master_bridge),
                                                 m_data_space(m_master_bridge),
                                                 m_symbols_fields(m_master_bridge),
                                                 m_field_magic(*this)
{   
    HRESULT hr = m_client.get_interface()->OpenDumpFileWide(dump_name.c_str(), 0);

    if (FAILED(hr)) {
        throw std::exception("Couldnt initialize dump");
    }
    
    hr = m_control.get_interface()->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
    if (FAILED(hr)) {
         throw std::exception("Couldnt initialize dump");
    }
}


DebugMagic::~DebugMagic()
{
}


void DebugMagic::load_module_base_symbols(const Address module_base)
{
    Expected<CV_INFO_PDB70> pdb_info = get_pdb_info_for_module_base(module_base);
    if (!pdb_info.has_value()) {
        throw pdb_info.error();
    }

    
    m_symbol_client.download_pdb(pdb_info.value());
    m_symbols.get_interface()->SetSymbolPathWide(m_symbol_client.get_symbol_directory().c_str());
    m_symbols.get_interface()->Reload("");
}


void DebugMagic::load_ntos_symbols()
{
    Expected<Address> ntos_base = get_ntos_base();
    if (ntos_base.has_value()) {
        load_module_base_symbols(ntos_base.value());
        
    }
}


void DebugMagic::load_module_symbols(const std::string& module_name)
{
     Expected<Address> module_base = get_module_base(module_name);
    if (module_base.has_value()) {
        load_module_base_symbols(module_base.value());
        
    }
}


Expected<Address> DebugMagic::get_module_base(const std::string& module_name)
{
    Address module_base;
    HRESULT hr = m_symbols.get_interface()->GetModuleByModuleName(module_name.c_str(), 0, nullptr, &module_base);
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldnt communicate with bridge"));
    }
    return module_base;
}


Expected<Address> DebugMagic::get_ntos_base()
{
    return get_module_base("nt");
}


Expected<Bytes> DebugMagic::read_memory_virtual(Address address, size_t size)
{
    Bytes result(size);

    HRESULT hr = m_data_space.get_interface()->ReadVirtual(address, result.data(), result.size(), nullptr);
    if (FAILED(hr)) {
        return unexpected(std::exception("Couldn't read virtual memory"));
    }
    
    return result;
}


Expected<Bytes> DebugMagic::read_memory_physical(Address address, size_t size)
{
    Bytes result(size);

    HRESULT hr = m_data_space.get_interface()->ReadPhysical(address, result.data(), result.size(), nullptr);
    if (FAILED(hr)) {
        return unexpected(std::exception("Couldn't read virtual memory"));
    }

    return result;
}


Expected<Address> DebugMagic::read_pointer_memory_virtual(Address address)
{
    Address pointer;
    HRESULT hr = m_data_space.get_interface()->ReadPointersVirtual(1, address,&pointer);
    if (FAILED(hr)) {
        return unexpected(std::exception("Couldn't read virtual memory"));
    }

    return pointer;
}


Expected<Address> DebugMagic::read_pointer_memory_physical(Address address)
{
    Expected<Bytes> res = read_memory_physical(address,sizeof(Address));
    if (res.has_value()) {
        return *reinterpret_cast<Address*>(res.value().data());
    }

    return unexpected(res.error());
}


Expected<Address> DebugMagic::get_symbol_address(const std::string& module, const std::string& symbol)
{
    Address result = {0};
    HRESULT hr = m_symbols.get_interface()->GetOffsetByName(format_symbol_module(module, symbol).c_str(),&result);
    if (FAILED(hr)) {

        return unexpected(std::exception("Couldn't read symbol"));
    }

    return result;
}


Expected<std::pair<std::string,std::string>> DebugMagic::get_symbol_from_address(Address address)
{
    std::string name;
    name.resize(MAX_PATH);

    HRESULT hr = m_symbols.get_interface()->GetNameByOffset(address, name.data(), name.size(), nullptr, nullptr);
    if (FAILED(hr)) {
        return unexpected(exception("Coludn't read symbol from address"));
    }

    size_t sep = name.find('!');
    if (sep == std::string::npos)
        return std::unexpected(std::exception("No module separator found"));

    return std::make_pair(name.substr(0, sep), name.substr(sep + 1));
}


std::string DebugMagic::format_symbol_module(const std::string& module, const std::string symbol)
{
    return format("{}!{}",module,symbol);
}

Expected<CV_INFO_PDB70> DebugMagic::get_pdb_info_for_module_base(Address module_base)
{
   static constexpr size_t PE_HEADER_SIZE = 0x1000 ;

   auto failed_result = unexpected(std::exception("Couldn't find pdb information in given module base"));

   Expected<Bytes> ntos_header = read_memory_virtual(module_base, PE_HEADER_SIZE);
   if(!ntos_header.has_value())
       return failed_result;

   std::byte* host_map_module = ntos_header.value().data();

   PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)host_map_module;
   if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) 
       return failed_result;
 
   PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(host_map_module + dosHeader->e_lfanew);
   if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) 
        return failed_result;
   IMAGE_DATA_DIRECTORY debugDirInfo = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
   if (debugDirInfo.Size == 0)
       return failed_result;

   Expected<Bytes> debug_directory =  read_memory_virtual(module_base + debugDirInfo.VirtualAddress, debugDirInfo.Size);
   if(!debug_directory.has_value())
       return failed_result;
    DWORD numEntries = debugDirInfo.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    PIMAGE_DEBUG_DIRECTORY debugEntry = (PIMAGE_DEBUG_DIRECTORY)debug_directory.value().data();

    for (DWORD i = 0; i < numEntries; i++, debugEntry++) {
        if (debugEntry->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            Expected<CV_INFO_PDB70> debug_info =  read_struct_memory_virtual<CV_INFO_PDB70>(module_base + debugEntry->AddressOfRawData);
            if (debug_info.has_value()) {
                return debug_info.value();
            }

        }
    }

    return failed_result; 

}


Expected<Address> DebugMagic::get_struct_base_from_field(const std::string& module_name, 
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


Expected<ULONG> DebugMagic::get_type_id(ULONG64 mod,
                                        const std::string& type_name)
{
    ULONG type_id = 0;
    HRESULT hr = m_symbols_fields.get_interface()->GetTypeId(mod, type_name.c_str(), &type_id);
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldn't get type id"));
    }

    return type_id;
}


Expected<ULONG> DebugMagic::get_type_size(ULONG64 mod,
                                          ULONG type_id)
{
    ULONG size = 0;
    HRESULT hr = m_symbols_fields.get_interface()->GetTypeSize(mod, type_id, &size);
    return FAILED(hr) 
        ? std::unexpected(std::exception("Couldn't get type size")) 
        : Expected<ULONG>{ size };
}


Expected<std::string> DebugMagic::get_type_name(ULONG64 mod,
                                                ULONG type_id)
{
    std::string buf;
    buf.resize(MAX_PATH);


    HRESULT hr = m_symbols_fields.get_interface()->GetTypeName(mod, type_id, buf.data(), buf.size(), nullptr);
    if (FAILED(hr)) {
        return std::unexpected(std::exception("Couldn't get type name"));
    }

    return buf;
}


Expected<FieldInfo> DebugMagic::get_field_info(ULONG64 mod,
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
        .size       = type_size.value(),
        .is_pointer = !type_name->empty() && type_name->back() == '*'
    };
}


Expected<std::vector<std::string>> DebugMagic::get_field_names(ULONG64 mod,
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



TypedValue DebugMagic::resolve_field_value(
    ULONG64            mod,
    const FieldInfo&   field,
    Address            field_address,
    const std::string& module_name,
    int                max_depth)
{
    auto make = [&](auto val) -> TypedValue {
        return TypedValue{ field.type_name, val };
    };

    auto unreadable = [&]() -> TypedValue {
        return TypedValue{ field.type_name, std::string("<unreadable>") };
    };

    // Pointer
    if (field.is_pointer) {
        auto ptr = read_pointer_memory_virtual(field_address);
        return ptr.has_value() ? make(ptr.value()) : unreadable();
    }

    // char[]
    if (field.type_name.contains("har") && !field.type_name.contains('W') && field.size > 1) {
        auto bytes = read_memory_virtual(field_address, field.size);
        if (!bytes.has_value()) return unreadable();
        auto* data = reinterpret_cast<const char*>(bytes->data());
        return make(std::string(data, strnlen(data, field.size)));
    }

    // wchar[]
    if (field.type_name.contains("har") && field.type_name.contains('W') && field.size > 1) {
        auto bytes = read_memory_virtual(field_address, field.size);
        if (!bytes.has_value()) return unreadable();
        auto* data = reinterpret_cast<const wchar_t*>(bytes->data());
        return make(std::wstring(data, wcsnlen(data, field.size / sizeof(wchar_t))));
    }

    // Nested struct — recurse
    if (field.size > 8 && max_depth > 1) {
        auto nested = struct_magic(module_name, field.type_name, field_address, max_depth - 1);
        if (nested.has_value())
            return make(std::make_shared<GenericTypeContainer>(std::move(nested.value())));
        return unreadable();
    }

    // Floats
    if (field.type_name == "float")  { auto v = read_struct_memory_virtual<float> (field_address); return v.has_value() ? make(v.value()) : unreadable(); }
    if (field.type_name == "double") { auto v = read_struct_memory_virtual<double>(field_address); return v.has_value() ? make(v.value()) : unreadable(); }

    // Integers by size
    switch (field.size)
    {
        case 1: { auto v = read_struct_memory_virtual<uint8_t> (field_address); return v.has_value() ? make(v.value()) : unreadable(); }
        case 2: { auto v = read_struct_memory_virtual<uint16_t>(field_address); return v.has_value() ? make(v.value()) : unreadable(); }
        case 4: { auto v = read_struct_memory_virtual<uint32_t>(field_address); return v.has_value() ? make(v.value()) : unreadable(); }
        case 8: { auto v = read_struct_memory_virtual<uint64_t>(field_address); return v.has_value() ? make(v.value()) : unreadable(); }
        default: break;
    }

    // Raw blob fallback
    auto bytes = read_memory_virtual(field_address, field.size);
    return bytes.has_value() ? make(bytes.value()) : unreadable();
}


Expected<GenericTypeContainer> DebugMagic::struct_magic(
    const std::string& module_name,
    const std::string& type,
    Address     address,
    int         max_depth)
{
    Expected<Address> module_base = get_module_base(module_name);
    if (!module_base.has_value()) {
        return unexpected(std::exception("Couldn't resolve module base"));
    }

    if (max_depth <= 0)
        return std::unexpected(std::exception("Max depth reached"));

    Expected<TypeInfoCache*> type_cache_info = m_field_magic.get_type_info(module_name, type);
    if (!type_cache_info.has_value()) {
        return unexpected(std::exception("Couldnt load type!"));
    }

    GenericTypeContainer container(type, address);

    for (const auto& field_item : *type_cache_info.value())
    {
        Address field_address = address + static_cast<Address>(field_item.second.offset);
        
        container.set(field_item.first, 
                      resolve_field_value(module_base.value(), 
                                          field_item.second, 
                                          field_address, 
                                          module_name, 
                                          max_depth));
    }

    return container;
}