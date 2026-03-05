#include "TypeMagic.h"
#include "FieldInfoMagic.h"

using namespace std;

namespace TypeMagic {

map<std::string, ParseBasicType> basic_types = {


	{"char[]", parse_as_arrray<int8_t>},
	{"short[]", parse_as_arrray<int16_t>},
	{"long[]", parse_as_arrray<int32_t>},
	{"int64[]", parse_as_arrray<int64_t>},

	{"unsigned char[]", parse_as_arrray<uint8_t>},
	{"unsigned short[]", parse_as_arrray<uint16_t>},
	{"unsigned long[]", parse_as_arrray<uint32_t>},
	{"unsigned int64[]", parse_as_arrray<uint64_t>},

	{"void*", parse_basic_type<Address>},

	{"wchar_t*", parse_basic_type<Address>},


	{"char*", parse_basic_type<Address>}, // replace in dedcated function to support 32-bit
	{"short*", parse_basic_type<Address>},
	{"long*", parse_basic_type<Address>},
	{"int64*", parse_basic_type<Address>},

	{"unsigned char*", parse_basic_type<Address>},
	{"unsigned short*", parse_basic_type<Address>},
	{"unsigned long*", parse_basic_type<Address>},
	{"unsigned int64*", parse_basic_type<Address>},


	{"char", parse_basic_type<int8_t>}, //8bit
	{"short", parse_basic_type<int16_t>}, //16bit
	{"long", parse_basic_type<int32_t>}, //32bit
	{"int64", parse_basic_type<int64_t>}, //64bit

	{"unsigned char", parse_basic_type<uint8_t>}, //8bit
	{"unsigned short", parse_basic_type<uint16_t>}, //16bit
	{"unsigned long", parse_basic_type<uint32_t>}, //32bit
	{"unsigned int64", parse_basic_type<uint64_t>} //64bit

};


shared_ptr<GenericTypeContainer> do_type_magic(DebugMagic& debugmagic, Address address, std::string type_name, const string& module_base)
{

	Expected<TypeInfoCache*> type_cache_info = debugmagic.get_field_info_magic().get_type_info(module_base, type_name);
	if (!type_cache_info.has_value()) {
		throw type_cache_info.error();
	}

	shared_ptr<GenericTypeContainer> container = make_shared<GenericTypeContainer>(module_base, type_name, address);

	for (const auto& field_item : type_cache_info.value()->fields)
	{
		Address field_address = address + static_cast<Address>(field_item.second.offset);

		container->set(field_item.first, do_field_magic(debugmagic, field_address, const_cast<FieldInfo&>(field_item.second)));
	}

	return container;
}


TypedValue do_field_magic(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info)
{
	auto basic_type_ptr = basic_types.find(field_info.type_name);

	if (basic_type_ptr != basic_types.end()) {
		return basic_type_ptr->second(debugmagic, field_address, field_info);
	}

	if (field_info.type_name.ends_with(ARRAY_SUFFIX)) {
		string_view value_type_name_view = string_view(field_info.type_name.data(),
													field_info.type_name.size() - ARRAY_SUFFIX.size());

		string value_type_name(value_type_name_view.begin(), value_type_name_view.end());

		auto value_type_info = debugmagic.get_field_info_magic().get_type_info(field_info.module_name, value_type_name.data());
		if (!value_type_info.has_value()) {
			throw value_type_info.error();
		}

		vector<TypedValue> values;
		for (size_t i = 0; i < field_info.size; i += value_type_info.value()->type_size) {
			shared_ptr<GenericTypeContainer> nested = do_type_magic(debugmagic,
																	field_address + i,
																	value_type_name.data(),
																	field_info.module_name);

			values.emplace_back(TypedValue(field_info.module_name, value_type_name.data(), nested));
		}

		return TypedValue(field_info.module_name, field_info.type_name, values);
	}

	if (field_info.type_name.ends_with(POINTER_SUFFIX))
	{
		Expected<Address> value = debugmagic.read_pointer_memory_virtual(field_address);
		if (!value.has_value()) {
			throw value.error();
		}

		return TypedValue(field_info.module_name, field_info.type_name, *value);
	}


	if (field_info.type_name.starts_with(UNSIGNED_PREFIX))
	{
		return parse_as_as_bytes(debugmagic, field_address, field_info);
	}

	if (field_info.type_name == INVALID_TYPE) {
		return parse_as_as_bytes(debugmagic, field_address, field_info);
	}

	return TypedValue(field_info.module_name,
		field_info.type_name,
		do_type_magic(debugmagic,
			field_address,
			field_info.type_name,
			field_info.module_name));

}


TypedValue parse_as_as_bytes(DebugMagic& debugmagic, Address field_address, FieldInfo& field_info)
{
	Expected<Bytes> value = debugmagic.read_memory_virtual(field_address, field_info.size);
	if (value.has_value()) {
		return TypedValue(field_info.module_name, field_info.type_name, *value);
	}

	throw value.error();
}

};

