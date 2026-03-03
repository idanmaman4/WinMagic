#include "FieldInfoMagic.h"
#include "DebugMagic.h"

using namespace std;

FieldInfoMagic::FieldInfoMagic(DebugMagic& debugmagic): m_debug_magic(debugmagic)
{
}


FieldInfoMagic::~FieldInfoMagic()
{
}


void FieldInfoMagic::pre_create_cache_for_type(const std::string& module, const std::string& type_name)
{

	auto module_base = m_debug_magic.get_module_base(module);
	if (!module_base.has_value()) {
		throw std::exception("Can't resolve module, Invalid module name");
	}

	auto type_id = m_debug_magic.get_type_id(module_base.value(), type_name);
	if (!type_id.has_value()) {
		throw std::exception("Can't resolve type, Invalid type name");
	}

	if (!m_system_cache.contains(module)) {
		m_system_cache[module] = SymbolTableCache();
	}

	auto& symbol_table = m_system_cache[module];
	if (symbol_table.contains(type_name)) {
		return;
	}

	auto& type_cache = symbol_table[type_name] = TypeInfoCache();
	
	auto field_names = m_debug_magic.get_field_names(module_base.value(), type_id.value());
	if (!field_names.has_value()) {
		throw std::exception("Can't resolve fields, Invalid type");
	}

	for (auto& field_name: field_names.value()) {
		auto field_info = m_debug_magic.get_field_info(module_base.value(), type_id.value(), field_name);
		
		if (!field_info.has_value()) {
			continue;
		}

		type_cache[field_name] = field_info.value();
	
	}
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
	pre_create_cache_for_type(module_name, type_name);
	
	Expected<SymbolTableCache*> symbol_table_cache = get_current_symbol_table(module_name); 
	 if (!symbol_table_cache.has_value()) {
		return unexpected(symbol_table_cache.error());
	 }

	 if (!symbol_table_cache.value()->contains(type_name)) {
		return unexpected(std::exception("Type Cache does not contain given Type"));
	 }

	 return &symbol_table_cache.value()->at(type_name);
	
}


Expected<FieldInfo*> FieldInfoMagic::get_field_info(const std::string& module_name, const std::string& type_name, const std::string& field_name)
{
	
	 Expected<TypeInfoCache*> type_info_cache = get_type_info(module_name, type_name); 
	 if (!type_info_cache.has_value()) {
		return unexpected(type_info_cache.error());
	 }

	 if (!type_info_cache.value()->contains(field_name)) {
		return unexpected(std::exception("Type Cache does not contain given field"));
	 }

	 return &type_info_cache.value()->at(field_name);

}
