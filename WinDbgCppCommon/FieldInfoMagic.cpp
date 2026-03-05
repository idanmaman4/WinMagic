#include "FieldInfoMagic.h"
#include "DebugMagic.h"
#include <array>

using namespace std;


FieldInfoMagic::FieldInfoMagic(DebugMagic& debugmagic): m_debug_magic(debugmagic)
{
}


FieldInfoMagic::~FieldInfoMagic()
{
}


void FieldInfoMagic::pre_create_cache_for_type(const std::string& module_name, const std::string& type_name){
	
	Expected<Address> mod_address = m_debug_magic.get_module_base(module_name);
	
	if (!mod_address.has_value()) {
		 throw mod_address.error();
	}

	auto type_id = m_debug_magic.get_type_id(*mod_address, type_name);
	if (!type_id.has_value()) {
		throw type_id.error();
	}

	if (!m_system_cache.contains(module_name)) {
		m_system_cache[module_name] = SymbolTableCache();
	}

	auto& symbol_table = m_system_cache[module_name];
	if (symbol_table.contains(type_name)) {
		return;
	}

	auto& type_cache = symbol_table[type_name] = TypeInfoCache();
	
	auto field_names = m_debug_magic.get_field_names(*mod_address, *type_id);
	if (!field_names.has_value()) {
		throw field_names.error();
	}

	for (auto& field_name: field_names.value()) {
		auto field_info = m_debug_magic.get_field_info(*mod_address, *type_id, field_name);
		
		if (!field_info.has_value()) {
			continue;
		}

		field_info->module_name = module_name;
		
		type_cache.fields[field_name] = *field_info;
	
	}
	Expected<size_t> type_size = m_debug_magic.get_type_size(*mod_address, *type_id);
	if (!type_size.has_value()) {
		throw type_size.error();
	}
	
	type_cache.type_size = *type_size;
}


Expected<SymbolTableCache*> FieldInfoMagic::get_current_symbol_table(const std::string& module_name)
{
	if (!m_system_cache.contains(module_name)) {
		return unexpected(std::exception("Type Cache does not contain given module"));
	 }

	 return &m_system_cache.at(module_name);
}


Expected<TypeInfoCache*> FieldInfoMagic::get_type_info(const std::string& module_name, const std::string& type_name)
{
	 Expected<SymbolTableCache*> symbol_table_cache = get_current_symbol_table(module_name);
	 if (symbol_table_cache.has_value() && symbol_table_cache.value()->contains(type_name) ) {
		return &symbol_table_cache.value()->at(type_name);
	 }


	 pre_create_cache_for_type(module_name, type_name);

	 Expected<SymbolTableCache*> symbol_table_cache_2 = get_current_symbol_table(module_name); 
	 if (!symbol_table_cache_2.has_value()) {
		return unexpected(symbol_table_cache_2.error());
	 }

	 if (!symbol_table_cache_2.value()->contains(type_name)) {
		return unexpected(std::exception("Type Cache does not contain given Type"));
	 }

	 return &symbol_table_cache_2.value()->at(type_name);
}


Expected<FieldInfo*> FieldInfoMagic::get_field_info(const std::string& module_name, const std::string& type_name, const std::string& field_name)
{
	
	 Expected<TypeInfoCache*> type_info_cache = get_type_info(module_name, type_name); 
	 if (!type_info_cache.has_value()) {
		return unexpected(type_info_cache.error());
	 }

	 if (!(*type_info_cache)->fields.contains(field_name)) {
		return unexpected(std::exception("Type Cache does not contain given field"));
	 }

	 return &(*type_info_cache)->fields.at(field_name);

}
