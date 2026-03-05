#pragma once

#include "WinStructres.hpp"
#include "Types.h"
#include "DebugMagic.h"

namespace PeUtils
{
static constexpr size_t PE_HEADER_SIZE = 0x1000 ;
Expected<BytesView> get_nt_header_for_pe(DebugMagic& debugmagic, BytesView pe_header_view);
Expected<CV_INFO_PDB70> get_pdb_info_for_pe(DebugMagic& debugmagic, Address module_base);

};

