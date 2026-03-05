#include "PeUtils.h"

using namespace std;

namespace PeUtils {
Expected<CV_INFO_PDB70> get_pdb_info_for_pe(DebugMagic& debugmagic, Address module_base)
{
   auto failed_result = unexpected(std::exception("Couldn't find pdb information in given module base"));

   Expected<Bytes> ntos_header = debugmagic.read_memory_virtual(module_base, PE_HEADER_SIZE);
   if(!ntos_header.has_value())
       return failed_result;

   Expected<BytesView> nt_headers_view= get_nt_header_for_pe(debugmagic, *ntos_header);

   if (!nt_headers_view.has_value()) {
        return unexpected(nt_headers_view.error());
   }

   PIMAGE_NT_HEADERS64 nt_headers = (PIMAGE_NT_HEADERS64)nt_headers_view->data();
   
   IMAGE_DATA_DIRECTORY debugDirInfo = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
   if (debugDirInfo.Size == 0)
       return failed_result;

   Expected<Bytes> debug_directory = debugmagic.read_memory_virtual(module_base + debugDirInfo.VirtualAddress, debugDirInfo.Size);
   if(!debug_directory.has_value())
       return failed_result;


    DWORD numEntries = debugDirInfo.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    PIMAGE_DEBUG_DIRECTORY debugEntry = (PIMAGE_DEBUG_DIRECTORY)debug_directory.value().data();

    for (DWORD i = 0; i < numEntries; i++, debugEntry++) {
        if (debugEntry->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            Expected<CV_INFO_PDB70> debug_info =  debugmagic.read_struct_memory_virtual<CV_INFO_PDB70>(module_base + debugEntry->AddressOfRawData);
            if (debug_info.has_value()) {
                return debug_info.value();
            }

        }
    }

    return failed_result; 

}


Expected<BytesView> get_nt_header_for_pe(DebugMagic& debugmagic, BytesView pe_header_view)
{

   unsigned char* host_map_module = pe_header_view.data();

   PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)host_map_module;
   if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) 
       return unexpected(std::exception("Invalid Dos Signature"));;
 
    
   BytesView::pointer view_start = host_map_module + dosHeader->e_lfanew;
   PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)(view_start);
   if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) 
        return unexpected(std::exception("Invalid Image NT Signatures"));;;
   

   return BytesView(view_start, sizeof(IMAGE_NT_HEADERS64));
}

};