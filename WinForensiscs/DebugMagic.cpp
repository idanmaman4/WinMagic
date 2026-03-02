#include "DebugMagic.h"
#include "SymbolClient.h"

using namespace std;


DebugMagic::DebugMagic(std::wstring dump_name) : m_master_bridge(),
                                                 m_client(m_master_bridge),
                                                 m_control(m_master_bridge),
                                                 m_symbols(m_master_bridge),
                                                 m_data_space(m_master_bridge),
                                                 m_symbols_fields(m_master_bridge)
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

void DebugMagic::load_module_base_symbols(Address module_base)
{
    Expected<CV_INFO_PDB70> pdb_info = get_pdb_info_for_module_base(module_base);
    if (!pdb_info.has_value()) {
        throw pdb_info.error();
    }

    SymbolClient symbol_client(pdb_info.value());
    m_symbols.get_interface()->SetSymbolPathWide(symbol_client.get_symbol_directory().c_str());
    m_symbols.get_interface()->Reload("");
}

void DebugMagic::load_ntos_symbols()
{
    Expected<Address> ntos_base = get_ntos_base();
    if (ntos_base.has_value()) {
        load_module_base_symbols(ntos_base.value());
        
    }
}

void DebugMagic::load_module_symbols(std::string module_name)
{
     Expected<Address> module_base = get_module_base(module_name);
    if (module_base.has_value()) {
        load_module_base_symbols(module_base.value());
        
    }
}

Expected<Address> DebugMagic::get_module_base(std::string module_name)
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

Expected<Address> DebugMagic::get_symbol_address(std::string symbol)
{
    Address result = {0};
    HRESULT hr = m_symbols.get_interface()->GetOffsetByName(symbol.c_str(),&result);
    if (FAILED(hr)) {

        return unexpected(std::exception("Couldn't read symbol"));
    }

    return result;
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

Expected<Address> DebugMagic::get_struct_base_from_field(std::string module_name, std::string type, std::string field_name, Address address)
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
