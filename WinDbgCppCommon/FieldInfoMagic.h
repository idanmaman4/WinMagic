#pragma once
#include <map>
#include <vector>
#include <string>
#include "GenericTypeContainer.h"
#include "Types.h"

class DebugMagic; // forward declare


// Per Type
struct TypeInfoCache {
	size_t type_size; 
	std::map<std::string, FieldInfo> fields;
};


// Per Module
using SymbolTableCache = std::map<std::string, TypeInfoCache>;


// Per Session
using SystemCache = std::map<std::string, SymbolTableCache>;



class FieldInfoMagic
{
public:
	
	FieldInfoMagic(DebugMagic& debugmagic);

	~FieldInfoMagic(); 

	void pre_create_cache_for_type(const std::string& module, const std::string& type_name);

	Expected<SymbolTableCache*> get_current_symbol_table(const std::string& module_base);
	
	Expected<TypeInfoCache*> get_type_info(const std::string& module_base, const std::string& type_name);
	
	Expected<FieldInfo*> get_field_info(const std::string& module_name, const std::string& type_name, const std::string& field_name);
	
private:
	DebugMagic& m_debug_magic;
	SystemCache m_system_cache;
};

