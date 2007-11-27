
#include "ATLog.h"

ATLog::ATLog()
{
	init();
}

ATLog::ATLog( const QString &strFile )
{
	init();
	setOutputFile( strFile );
}

ATLog::ATLog( QTextEdit *pGUI )
{
	init();
	setOutputGUI( pGUI );
}

ATLog::~ATLog()
{
	// App crashes when closing down if trying to log to
	// the text field because it is destroyed already.
	// Ideally I'd find a way to test whether the field 
	// still exists, but in the meantime the GUI log is
	// disabled at the end.
	// TODO: Find a better way
	bLoggingGUI = false;

	closeFile();
	closeGUI();
}

void ATLog::init()
{
	pOutputFile = NULL;
	pOutputGUI = NULL;
	bLoggingFile = false;
	bLoggingGUI = false;
}

void ATLog::setOutputFile( const QString &strFile )
{
	closeFile();

	pOutputFile = new QFile( strFile );
	bLoggingFile = pOutputFile->open( QIODevice::Append );

	if ( !bLoggingFile ) // arg error, but can't log it.
	{
		closeFile();
	}
	else
	{
		pOutputFile->write( QString("\n").toUtf8() );
		addEntry( "-- File log starts" );
	}
}

void ATLog::setOutputGUI( QTextEdit *pGUI )
{
	closeGUI();

	pOutputGUI = pGUI;

	bLoggingGUI = ( pOutputGUI != NULL );

	if ( !bLoggingGUI ) // arg error, but can't log it.
	{
		closeGUI();
	}
	else
	{
		pOutputGUI->append( "" );
		addEntry( "-- Log initialised" );
	}
}

void ATLog::closeFile()
{
	if (pOutputFile!=NULL)
	{
		if ( bLoggingFile )
		{
			addEntry( "-- File log ends" );
		}

		delete pOutputFile;
		pOutputFile = NULL;

		bLoggingFile = false;
	}
}

void ATLog::closeGUI()
{
	if (pOutputGUI!=NULL)
	{
		if ( bLoggingGUI )
		{
			addEntry( "-- GUI log ends" );
		}

//		delete pOutputGUI;  // hmm no, don't delete. it's been created by somebody else.
		pOutputGUI = NULL;

		bLoggingGUI = false;
	}
}

void ATLog::addEntry( const QString &strEntry )
{
	if ( bLoggingFile )
	{
		pOutputFile->write( QString("%1: %2\n")
				.arg( QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss.zzz" ) )
				.arg( strEntry )
				.toUtf8() );
		pOutputFile->flush();
	}

	if ( bLoggingGUI )
	{
		Q_ASSERT( pOutputGUI != NULL );

		pOutputGUI->append( QString("%1: %2")
				.arg( QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss.zzz" ) )
				.arg( strEntry )
				.toUtf8() );
		pOutputGUI->ensureCursorVisible();
	}

#ifdef WIN32
	{
		QString strDbg;
		strDbg.sprintf( "%s\n", strEntry.toUtf8().data() );
		OutputDebugStringA( strDbg.toUtf8().data() );
	}
#endif
}


void ATLog::addDebugEntry( const QString &strEntry )
{
	addEntry( QString("Debug: %1").arg(strEntry) );
}

void ATLog::addWarningEntry( const QString &strEntry )
{
	addEntry( QString("Warning: %1").arg(strEntry) );
}

void ATLog::addCriticalEntry( const QString &strEntry )
{
	addEntry( QString("Critical: %1").arg(strEntry) );
}

void ATLog::addFatalEntry( const QString &strEntry )
{
	addEntry( QString("Fatal: %1").arg(strEntry) );
}

