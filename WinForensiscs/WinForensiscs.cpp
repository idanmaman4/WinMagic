#include <windows.h>
#include <dbgeng.h>
#include <iostream>
#include <iomanip>
#include <string>
#include "wdbgexts.h"
#include <urlmon.h>
#include<vector>
#include <array>
#include <ranges>
#include "DebugMagic.h"
#include "GenericTypeContainer.h"
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "dbgeng.lib")




class StdOutCallbacks : public IDebugOutputCallbacks {
public:
    // IUnknown methods
    STDMETHODV_(ULONG, AddRef)(THIS) { return 1; }
    STDMETHODV_(ULONG, Release)(THIS) { return 0; }
    STDMETHODV(QueryInterface)(THIS_ IN REFIID InterfaceId, OUT PVOID* Interface) {
        if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) || IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks))) {
            *Interface = (IDebugOutputCallbacks*)this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    // IDebugOutputCallbacks method
    STDMETHODV(Output)(THIS_ IN ULONG Mask, IN PCSTR Text) {
        std::cout << Text;
        return S_OK;
    }
};





int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2) {
        std::wcerr << L"Usage:\n";
        std::wcerr << L"  DumpProcessLister.exe <path_to_kernel_dump.dmp>\n\n";
        std::wcerr << L"Example:\n";
        std::wcerr << L"  DumpProcessLister.exe C:\\Dumps\\MEMORY.DMP\n";
        return 1;
    }

    DebugMagic debug_magic(argv[1]);
    debug_magic.load_ntos_symbols();
    
    Expected<uint32_t> ssdt_size = debug_magic.get_symbol_address_as_struct<uint32_t>("nt", "KiServiceLimit");
    if (!ssdt_size.has_value()) {
        return -1;
    }
    Expected<std::vector<uint32_t>> ssdt = debug_magic.get_symbol_address_as_struct_array<uint32_t>("nt", "KiServiceTable", ssdt_size.value());
    Expected<Address> ssdt_address = debug_magic.get_symbol_address("nt", "KiServiceTable");
    if (!ssdt.has_value() || !ssdt_address.has_value()) {
        return -1;
    }
    
    size_t i=0;
    auto resolveEntry = [&](uint32_t rawEntry) -> std::pair<Address, std::string>{
        const Address entryAddress = (rawEntry >> 4) + ssdt_address.value();
        auto symbolResult = debug_magic.get_symbol_from_address(entryAddress);
        if (symbolResult) {
            return {entryAddress, symbolResult->second};
        }
       return {entryAddress,"Unknown"};
    };

    for (auto ssdt_item : ssdt.value() | std::views::transform(resolveEntry)){
        std::cout << i << ".) "<< ssdt_item.second << " | " << std::hex << ssdt_item.first << std::dec << std::endl;
        i+= 1;
    }




    /*Expected<GenericTypeContainer> process_head = debug_magic.get_symbol_address_as<"nt","_LIST_ENTRY">("nt!PsActiveProcessHead");
    if (!process_head.has_value()) {
        return -1;
    }
    Address current_link = process_head.value().get<Address>("Flink"), head= process_head.value().address();
    

    size_t i=0;
    do {
       i+=1;
       auto eprocess = debug_magic.get_struct_from_field_as<"nt", "_EPROCESS", "ActiveProcessLinks">(current_link);
       if (!eprocess.has_value()){
            break;
       }
       std::cout << i << " .) "
                 << " Pid: " << *eprocess.value().get("UniqueProcessId") 
                 << " Name: " << *eprocess.value().get("ImageFileName") 
                 << std::endl;

       Expected<Address> link = debug_magic.get_struct_field<Address>("nt", "_LIST_ENTRY", "Flink", current_link);
       if (!link.has_value()) {
            break;
       }
       current_link = link.value();
    }while (current_link != 0 && current_link != head);
    */
    
    return 0;
}