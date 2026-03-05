#pragma once

#include "GenericTypeContainer.h"
#include "DebugMagic.h"
#include "Types.h"
#include "WinStructres.hpp"
#include <array>
#include <memory>
#include <concepts>

namespace TypeMagic
{

	template <typename T>
	concept IsString = 
		std::derived_from<T, std::wstring>    ||
		std::derived_from<T, std::string>;

	using ParseBasicType = TypedValue (*)(DebugMagic& debugmagic, Address field_address,  FieldInfo& field_info); 

	constexpr std::string_view INVALID_TYPE = "<unnamed-tag>";
	constexpr std::string_view UNSIGNED_PREFIX = "unsigned"; 
	constexpr std::string_view ARRAY_SUFFIX = "[]";
	constexpr std::string_view POINTER_SUFFIX = "*";
	
	extern std::map<std::string, ParseBasicType> basic_types;

	 std::shared_ptr<GenericTypeContainer> do_type_magic(DebugMagic& debugmagic, Address field_address, std::string type_name, const std::string& module_base);

	TypedValue do_field_magic(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info);

	template <typename T>
	TypedValue parse_basic_type(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info)
	{
		Expected<T> value =  debugmagic.read_struct_memory_virtual<T>(field_address);
		
		if (!value.has_value()) {
			throw value.error();
		}

		return TypedValue(field_info.module_name, field_info.type_name, *value);

	}

	template <IsString T>
	TypedValue parse_as_string(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info)
	{
		Expected<Bytes> string_val = debugmagic.read_memory_virtual(field_address, field_info.size);
		if (!string_val.has_value()) {
			throw string_val.error(); 
		}

		T value(reinterpret_cast<T::value_type*>(string_val->data()), string_val->size());
	
		return TypedValue(field_info.module_name,
						  field_info.type_name,
						  value);
	}

	
	template <typename T>
	TypedValue parse_as_arrray(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info)
	{
		std::vector<T> res;
		for (size_t i = 0; i < field_info.size; i += sizeof(T)) {
			Expected<T> value_at_index = debugmagic.read_struct_memory_virtual<T>(field_address + i );
			if (!value_at_index.has_value()) {
				throw value_at_index.error(); 
			}
			res.emplace_back(*value_at_index);
		}

		return TypedValue(field_info.module_name,
						  field_info.type_name,
						  res);
	}


	TypedValue parse_as_as_bytes(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info);
};
	
