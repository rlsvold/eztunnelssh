
#include "pch.h"
#include "ATLog.h"
#include "ATMainWindow.h"

// For the about box
const char *APP_VERSION = "0.7.4"; // Don't forget the resource file.
const char *APP_DATE = __DATE__;
const char *APP_NICE_NAME = "ezTunnel SSH";

// For the registry
const char *SETTINGS_COMPANY = "Hikey";
const char *SETTINGS_APP = "ezTunnelSSH";

ATLog atlog;
const char *argv0="";
QString g_strIniFile;

static void myMessageOutput(QtMsgType type, const char *msg)
{
	switch (type)
	{
	case QtDebugMsg:
		atlog.addDebugEntry( msg );
		fprintf(stderr, "Debug: %s\n", msg);
		break;
	case QtWarningMsg:
		atlog.addWarningEntry( msg );
		// fprintf(stderr, "Warning: %s\n", msg);
		break;
	case QtCriticalMsg:
		atlog.addCriticalEntry( msg );
		// fprintf(stderr, "Critical: %s\n", msg);
		break;
	case QtFatalMsg:
		atlog.addFatalEntry( msg );
		// fprintf(stderr, "Fatal: %s\n", msg);
		abort(); // deliberately core dump
		break;
	default:
		fprintf(stderr, "Type %d: %s\n", type, msg);
#ifdef WIN32
		{
			QString strDbg;
			strDbg.sprintf( "Type %d: %s\n", type, msg );
			OutputDebugStringA( strDbg.toUtf8().data() );
		}
#endif
	}
}

int main(int argc, char *argv[])
{
	qDebug( __FUNCTION__ );

	argv0 = argv[0];
	g_strIniFile = QString( "%1.ini" ).arg( argv0 );
	g_strIniFile.remove(".exe");

	QApplication app(argc, argv);
	// ATASSERT( app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit())) );

	qInstallMsgHandler(myMessageOutput);
	//atlog.setOutputFile( "atlog.txt" );

	ATMainWindow_c mainWindow;
	mainWindow.show();

	return app.exec();
}
