#pragma once

#include <string>
#include "DebugMagic.h"
#include "GenericTypeContainer.h"

namespace MagicUtils
{
	std::wstring parse_unicode_string(DebugMagic& debug_magic, std::shared_ptr<GenericTypeContainer> unicode_object); 
};

