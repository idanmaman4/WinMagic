#include <windows.h>
#include <dbgeng.h>
#include <iostream>
#include <iomanip>
#include <string>
#include "wdbgexts.h"
#include <urlmon.h>
#include<vector>
#include <array>
#include "DebugMagic.h"
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
    Expected<Address> head = debug_magic.get_symbol_address("nt!PsActiveProcessHead");
    Address current_link;
    if (!head.has_value()) {
        return -1;
    }
    current_link = head.value();
    
    do {
        auto eprocess = debug_magic.get_struct_base_from_field("nt", "_EPROCESS", "ActiveProcessLinks", current_link);

        if (!eprocess.has_value()){
            break;
        }
        
        auto image_name = debug_magic.get_struct_field<std::array<char, 15>>("nt", "_EPROCESS", "ImageFileName", eprocess.value());
        if (image_name.has_value()) {
           std::string name(image_name.value().begin(), image_name.value().end());
           std::cout << name << std::endl;
        }

       Expected<Address> link = debug_magic.get_struct_field<Address>("nt", "_LIST_ENTRY", "Flink", current_link);
       if (!link.has_value()) {
            break;
       }
       current_link = link.value();

    }while (current_link != head.value() && current_link != 0 );

    return 0;
}