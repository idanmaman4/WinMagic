#include "MagicUtils.h"


namespace MagicUtils {
    std::wstring parse_unicode_string(DebugMagic& debug_magic, std::shared_ptr<GenericTypeContainer> unicode_object)
    {

        Expected<Address> buffer_address  = unicode_object.get()->as_pointer("Buffer");
        Expected<size_t> buffer_size = unicode_object.get()->as_number_unsigned("Length");

        if (!buffer_address.has_value() || !buffer_size.has_value()) {
            return L"";
        }
       
        Expected<Bytes> raw_wstring = debug_magic.read_memory_virtual(*buffer_address, *buffer_size);
        if (!raw_wstring.has_value()) {
            return L"";
        }

        std::wstring res(raw_wstring->begin(), raw_wstring->end()); 

        return res;
    }
}
