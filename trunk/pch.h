// Precompiled header

#ifdef WIN32
#pragma once
#endif

#ifndef __EZTUNNEL_PCH_H__
#define __EZTUNNEL_PCH_H__

#include <Qt>
#include <QtGui>
//#include <QtNetwork/QtNetwork>

#ifdef WIN32
#include <windows.h>
#endif

#include <assert.h>

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

/*
static char *format_number(int number)
{
	static int idx=0;
	static char buffer[8][16];
	int cnt2, cnt3;
	
	idx=(idx+1)%8;
	
	sprintf_s(buffer[idx], "%d", number);
	for (cnt2=strlen(buffer[idx])-3; cnt2>0; cnt2-=3)
	{
		for (cnt3=strlen(buffer[idx])+1; cnt3>cnt2; cnt3--)
		{
			buffer[idx][cnt3]=buffer[idx][cnt3-1];
		}
		buffer[idx][cnt2]=',';
	}
	
	return buffer[idx];
}
*/

class ATLog;
extern ATLog atlog;
extern const char *argv0;
extern QString g_strIniFile;

#endif

