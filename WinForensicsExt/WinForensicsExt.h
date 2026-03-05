// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the WINFORENSICSEXT_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// WINFORENSICSEXT_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WINFORENSICSEXT_EXPORTS
#define WINFORENSICSEXT_API __declspec(dllexport)
#else
#define WINFORENSICSEXT_API __declspec(dllimport)
#endif

// This class is exported from the dll
class WINFORENSICSEXT_API CWinForensicsExt {
public:
	CWinForensicsExt(void);
	// TODO: add your methods here.
};

extern WINFORENSICSEXT_API int nWinForensicsExt;

WINFORENSICSEXT_API int fnWinForensicsExt(void);
