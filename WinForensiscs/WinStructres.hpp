#pragma once

#include "Windows.h"

#define SYMOPT_CASE_INSENSITIVE 0x1
#define SYMOPT_UNDNAME         0x2
#define SYMOPT_DEFERRED_LOADS    0x4
#define SYMOPT_NO_CPP                0x8
#define SYMOPT_LOAD_LINES             0x10
#define SYMOPT_OMAP_FIND_NEAREST    0x20
#define SYMOPT_LOAD_ANYTHING          0x40
#define SYMOPT_IGNORE_CVREC          0x80
#define SYMOPT_NO_UNQUALIFIED_LOADS 0x100   
#define SYMOPT_FAIL_CRITICAL_ERRORS   0x200 
#define SYMOPT_EXACT_SYMBOLS           0x400    
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS    0x800  
#define SYMOPT_IGNORE_NT_SYMPATH          0x1000
#define SYMOPT_INCLUDE_32BIT_MODULES        0x2000
#define SYMOPT_PUBLICS_ONLY                  0x4000
#define SYMOPT_NO_PUBLICS                   0x8000
#define SYMOPT_AUTO_PUBLICS                   0x10000
#define SYMOPT_NO_IMAGE_SEARCH               0x20000
#define SYMOPT_SECURE                         0x40000
#define SYMOPT_NO_PROMPTS                      0x80000
#define SYMOPT_DEBUG                        0x80000000



// CodeView RSDS structure
struct CV_INFO_PDB70 {
    DWORD  CvSignature; 
    GUID   Signature;   
    DWORD  Age;         
    CHAR   PdbFileName[MAX_PATH]; 
};