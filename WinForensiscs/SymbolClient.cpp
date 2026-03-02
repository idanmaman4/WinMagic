#include "SymbolClient.h"
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

SymbolClient::SymbolClient(CV_INFO_PDB70 pdb_info, std::filesystem::path file_path): m_guid(pdb_info), m_path(file_path)
{
}

SymbolClient::SymbolClient(CV_INFO_PDB70 pdb_info) : m_guid(pdb_info), m_path(create_temp_path())
{
}

std::filesystem::path SymbolClient::get_symbol_directory() const
{
	return m_path;
}

std::string SymbolClient::get_guid_string()
{
	return std::format("{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:X}",
					   m_guid.Signature.Data1,
					   m_guid.Signature.Data2,
					   m_guid.Signature.Data3,
					   m_guid.Signature.Data4[0], 
					   m_guid.Signature.Data4[1],  
					   m_guid.Signature.Data4[2], 
					   m_guid.Signature.Data4[3],
					   m_guid.Signature.Data4[4],
					   m_guid.Signature.Data4[5],
					   m_guid.Signature.Data4[6],
					   m_guid.Signature.Data4[7],
					   m_guid.Age);
}



void SymbolClient::download_pdb()
{

	std::string url = std::format("{}/{}/{}/{}",
								  DEFAULT_SERVER,
								  m_guid.PdbFileName,
								  get_guid_string(),
								  m_guid.PdbFileName);

	HRESULT hr = URLDownloadToFileA(NULL,
									url.c_str(),
									m_path.string().c_str(),
									0,
									NULL);
	if(FAILED(hr)){
		throw std::exception("Couldn't Downlaod pdb from server");
	}
}

std::filesystem::path SymbolClient::create_temp_path()
{

	std::string temp_path;
	temp_path.resize(MAX_PATH, '\0');

    GetTempPathA(MAX_PATH, temp_path.data());
	
	return temp_path;
}
