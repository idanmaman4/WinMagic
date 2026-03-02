#pragma once
#include <string>
#include "DebugMagic.h"
#include "WinStructres.hpp"
#include <filesystem>

class SymbolClient
{
public:

	static constexpr std::string_view DEFAULT_SERVER =  "https://msdl.microsoft.com/download/symbols/";

	SymbolClient(CV_INFO_PDB70 pdb_info,std::filesystem::path file_path);
	SymbolClient(CV_INFO_PDB70 pdb_info);

	std::filesystem::path get_symbol_directory() const;
	std::string get_guid_string();

	void download_pdb();

private: 

	static std::filesystem::path create_temp_path();

	std::filesystem::path m_path;
	CV_INFO_PDB70 m_guid;
};

