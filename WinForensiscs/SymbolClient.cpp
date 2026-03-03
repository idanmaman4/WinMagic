#include "SymbolClient.h"
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

SymbolClient::SymbolClient(std::filesystem::path file_path): m_path(file_path)
{
}


SymbolClient::SymbolClient() : m_path(create_temp_path())
{
}


std::filesystem::path SymbolClient::get_symbol_directory() const
{
	return m_path;
}


std::string SymbolClient::get_guid_string(const CV_INFO_PDB70& pdb_info)
{
	return std::format("{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:X}",
					   pdb_info.Signature.Data1,
					   pdb_info.Signature.Data2,
					   pdb_info.Signature.Data3,
					   pdb_info.Signature.Data4[0], 
					   pdb_info.Signature.Data4[1],  
					   pdb_info.Signature.Data4[2], 
					   pdb_info.Signature.Data4[3],
					   pdb_info.Signature.Data4[4],
					   pdb_info.Signature.Data4[5],
					   pdb_info.Signature.Data4[6],
					   pdb_info.Signature.Data4[7],
					   pdb_info.Age);
}


void SymbolClient::download_pdb(const CV_INFO_PDB70& pdb_info)
{
    std::string name(pdb_info.PdbFileName);

    std::string url = std::format("{}/{}/{}/{}",
                                   DEFAULT_SERVER,
                                   name,
                                   get_guid_string(pdb_info),
                                   name);

    std::filesystem::path file_path = m_path;
    file_path.append(name);

    HRESULT hr = URLDownloadToFileA(
        NULL,
        url.c_str(),
        file_path.string().c_str(),
        0,
        NULL
    );

    if (FAILED(hr)) {
        //throw std::runtime_error(std::format(
          //  "Failed to download PDB '{}' from '{}' (HRESULT: 0x{:08X})",
          //  name, url, static_cast<uint32_t>(hr)
       // ));
    }
}


std::filesystem::path SymbolClient::create_temp_path()
{

	std::string temp_path;
	temp_path.resize(MAX_PATH, '\0');

    GetTempPathA(MAX_PATH, temp_path.data());
	
	return temp_path;
}
