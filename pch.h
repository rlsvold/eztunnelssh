// Precompiled header

#ifndef __EZTUNNEL_PCH_H__
#define __EZTUNNEL_PCH_H__

#ifdef WIN32
// WINDOWS
#define IF_WIN32(_X) _X
#define IF_NWIN32(_X)
#include <windows.h>
#else
// LINUX
#define IF_NWIN32(_X) _X
#define IF_WIN32(_X)
#endif

#include <Qt>
#include <QtGui>

#include <assert.h>

// stl
#include <list>
#include <algorithm>
#include <numeric>

// For the about box
extern const char *APP_VERSION;
extern const char *APP_DATE;
extern const char *APP_NICE_NAME;

// For the registry
extern const char *SETTINGS_COMPANY;
extern const char *SETTINGS_APP;

#ifdef _DEBUG
#define ATDEBUG qDebug
#define ATASSERT(X) assert(X)
#define ATVERIFY(X) assert(X)
#else
#define ATDEBUG (void)
#define ATASSERT (void)
#define ATVERIFY(X) (X)
#endif

#ifndef WIN32
	#define __FUNCTION__ ""
	#define sprintf_s sprintf
#endif

class ATLog;
extern ATLog atlog;
extern const char *argv0;
extern QString g_strIniFile;

#endif

