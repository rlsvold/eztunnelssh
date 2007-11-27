
#ifndef __AT_LOG_H__
#define __AT_LOG_H__

#include "pch.h"

class ATLog
{
private:
	QFile *pOutputFile;
	QTextEdit *pOutputGUI;
	bool bLoggingFile, bLoggingGUI;

	void init();
		
public:
	ATLog();
	ATLog( const QString &strFile );
	ATLog( QTextEdit *pGUI );
	~ATLog();

	void setOutputFile( const QString &strFile );
	void setOutputGUI( QTextEdit *pGUI );
	void closeFile();
	void closeGUI();
	bool logging();
	void addEntry( const QString &strEntry );

	void addDebugEntry( const QString &strEntry );
	void addWarningEntry( const QString &strEntry );
	void addCriticalEntry( const QString &strEntry );
	void addFatalEntry( const QString &strEntry );
};

#endif

