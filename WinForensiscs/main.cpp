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
#include "MagicUtils.h"
#include "ProcessMagic.h"
#include "GenericTypeContainer.h"
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "dbgeng.lib")





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
	Expected<std::vector<uint32_t>> ssdt = debug_magic.get_symbol_address_as_struct_array<uint32_t>("nt", "KiServiceTable", *ssdt_size);
	Expected<Address> ssdt_address = debug_magic.symbols().get_symbol_address("nt", "KiServiceTable");
	if (!ssdt.has_value() || !ssdt_address.has_value()) {
		return -1;
	}
	
	size_t i=0;
	auto resolveEntry = [&](uint32_t rawEntry) -> std::pair<Address, std::string>{
		const Address entryAddress = (rawEntry >> 4) + *ssdt_address;
		auto symbolResult = debug_magic.symbols().get_symbol_from_address(entryAddress);
		if (symbolResult) {
			return {entryAddress, symbolResult->second};
		}

	   return {entryAddress,"Unknown"};


	};

	for (auto ssdt_item : ssdt.value() | std::views::transform(resolveEntry)){
		std::cout << i << ".) "<< ssdt_item.second << " | " << std::hex << ssdt_item.first << std::dec << std::endl;
		i+= 1;
	}




	auto  process_head = debug_magic.get_symbol_address_as<"nt","_LIST_ENTRY">("nt","PsActiveProcessHead");
	if (!process_head.has_value()) {
		return -1;
	}

	Address current_link = process_head.value()->get<Address>("Flink").value();    
	auto head= process_head.value()->address();
	
	i=0;
	do {
	   i+=1;
	   auto eprocess = debug_magic.get_struct_from_field_as<"nt", "_EPROCESS", "ActiveProcessLinks">(current_link);
	   if (!eprocess.has_value()){
			break;
	   }


	   auto image_file_pointer = debug_magic.from_ptr(eprocess.value(), "ImageFilePointer");
	   std::wstring full_name;
	   if (image_file_pointer.has_value()) {
			auto unicode_string_obj = image_file_pointer.value()->as_object("FileName"); 
			full_name = MagicUtils::parse_unicode_string(debug_magic, unicode_string_obj.value());
	   }

	   auto pid =  eprocess.value()->as_number_unsigned("UniqueProcessId");

	   std::cout << i << " .) ";
	   std::cout << " Pid: " <<pid.value_or(0);
	   std::cout << " | Name: " << eprocess.value()->as_string("ImageFileName").value_or("Unkown_Name");
	   std::wcout << " | Full Path: " << full_name << std::endl ;
	   
	   try {
		   if (pid.has_value())
		   {
			   ProcessMagic magic(debug_magic, eprocess.value()->address());
			   auto peb = debug_magic.from_ptr(eprocess.value(), "Peb");
			   if (peb.has_value())
			   {
				  //debug_magic.load_module_symbols("ntdll");

				   auto ldr = debug_magic.from_ptr(peb.value(), "Ldr");
				   if (ldr.has_value())
				   {
						Address ldr_entry =  ldr.value()->as_object("InLoadOrderModuleList").value()->as_pointer("Flink").value();
						auto value = debug_magic.get_struct_from_field_as<"ntdll","_LDR_DATA_TABLE_ENTRY","InMemoryOrderLinks">(ldr_entry).value();
						std::wcout << " PEB NAME: " << MagicUtils::parse_unicode_string(debug_magic,value->as_object("BaseDllName").value()) << std::endl;
				   }
			   }
			   else
			   {
				   
				   std::cout << peb.error().what() << std::endl;
			   }

		   }
	   }
		catch (const std::exception& exp)
		{
			
			std::cout << "ERROR: " << exp.what() << std::endl;

		}


	   Expected<Address> link = eprocess.value()->as_object("ActiveProcessLinks")->get()->as_pointer("Flink");
	   if (!link.has_value()) {
			break;
	   }

	   current_link = link.value();
	
	}while (current_link != 0 && current_link != head);
	
	
	return 0;
}