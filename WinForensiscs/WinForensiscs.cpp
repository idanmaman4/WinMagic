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
    

    Expected<GenericTypeContainer> process_head = debug_magic.get_symbol_address_as<"nt","_LIST_ENTRY">("nt!PsActiveProcessHead");
    if (!process_head.has_value()) {
        return -1;
    }
    Address current_link = process_head.value().get<Address>("Flink"), head= process_head.value().address();
    
    do {
       Expected<Address> link = debug_magic.get_struct_field<Address>("nt", "_LIST_ENTRY", "Flink", current_link);
       if (!link.has_value()) {
            break;
       }

       current_link = link.value();
       if (current_link == head) {
            break;
       }
       auto eprocess = debug_magic.get_struct_from_field_as<"nt", "_EPROCESS", "ActiveProcessLinks">(current_link);
       if (!eprocess.has_value()){
            break;
       }
      
       std::cout << " Pid: " << *eprocess.value().get("UniqueProcessId") 
                 << " Name:" << *eprocess.value().get("ImageFileName") 
                 << std::endl;


    }while (current_link != 0 );

    return 0;
}