// WinForensicsExt.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "WinForensicsExt.h"


// This is an example of an exported variable
WINFORENSICSEXT_API int nWinForensicsExt=0;

// This is an example of an exported function.
WINFORENSICSEXT_API int fnWinForensicsExt(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CWinForensicsExt::CWinForensicsExt()
{
    return;
}
