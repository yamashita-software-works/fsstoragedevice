#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

// C language
#include <stdio.h>
#include <tchar.h>
#include <locale.h>

// Windows
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <strsafe.h>

// ATL
#pragma warning(push)
#pragma warning(disable:4995)
#include <atlbase.h>
#include <atlsimpcoll.h>
#pragma warning(pop)

#pragma comment(lib, "setupapi.lib")
