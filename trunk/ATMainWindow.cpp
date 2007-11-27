#include "pch.h"

#include "ATMainWindow.h"
#include "ATNamedAction.h"
#include "ATSkeleton.h"

//++
// Details:	None
// Args:	None
// Return:	None
//--
ATMainWindow_c::ATMainWindow_c( QWidget *vpParent ):
QMainWindow( vpParent )
{
	m_pMainWindow = new ATSkeletonWindow(this);
	setCentralWidget( m_pMainWindow );

	setWindowIcon( QPixmap( ":connected.png" ) );
	setWindowTitle( QString( "%1 %2" ).arg( APP_NICE_NAME ).arg( APP_VERSION ) );

	InitMenusAndActions();

	createTrayIcon();

	readSettings();

	slotChangeStyle( m_strStyle );
}


void ATMainWindow_c::createTrayIcon()
{
	QAction *aboutAction = new QAction(tr("&About..."), this);
	ATVERIFY( connect(aboutAction, SIGNAL(triggered()), this, SLOT(slotShowAbout())) );

	QAction *restoreAction = new QAction(tr("&Restore"), this);
	ATVERIFY( connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal())) );

	QAction *quitAction = new QAction(tr("&Quit"), this);
	ATVERIFY( connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit())) );

	m_trayIconMenu = new QMenu( this );
	m_trayIconMenu->addAction(aboutAction);
	m_trayIconMenu->addAction(restoreAction);
	m_trayIconMenu->addAction(quitAction);

	m_trayIcon = new QSystemTrayIcon( this );
	m_trayIcon->setContextMenu( m_trayIconMenu );
	m_trayIcon->setIcon( QPixmap( ":disconnected.png" ) );
	m_trayIcon->show();

	ATVERIFY( connect( m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason))) );

	ATVERIFY( connect(m_pMainWindow, SIGNAL(signalSetTrayIcon(int)), this, SLOT(slotSetTrayIcon(int))) );
}

void ATMainWindow_c::slotSetTrayIcon( int iIndex )
{
	m_trayIcon->setIcon( QPixmap( iIndex ? ":connected.png" : ":disconnected.png" ) );
}

void ATMainWindow_c::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
	case QSystemTrayIcon::Trigger:
#ifndef WIN32
		hide();
#endif
		break;
	case QSystemTrayIcon::DoubleClick:
#ifdef WIN32
		::ShowWindow( winId(), SW_SHOWNORMAL );
		::SetForegroundWindow( winId() );
#else
		showNormal();
#endif
		break;
	//case QSystemTrayIcon::MiddleClick:
	//	break;
	default:
		;
	}
}

#ifdef WIN32
bool ATMainWindow_c::winEvent( MSG *m, long *result )
{
	if ( 1 ) // minimize to tray
	{
		switch ( m->message )
		{
		case WM_SIZE:
			{
				if ( m->wParam == SIZE_MINIMIZED )
				{
					ShowWindow(winId(), SW_HIDE);
					return true;
				}
			}
			break;
		default:
			break;
		}
	}

	return QWidget::winEvent(m, result);
} 
#endif

//++
// Details:	None
// Args:	None
// Return:	None
//--
bool ATMainWindow_c::InitMenusAndActions()
{
	QAction *pAction;

	QMenu *pFileMenu	= new QMenu(this);
	pAction				= new QAction(QObject::tr("&Quit"), this);
	pAction->setShortcut(QObject::tr("CTRL+Q"));
	bool bRet			= QObject::connect(pAction, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	ATASSERT( bRet );

	pFileMenu->addAction( pAction );

	QMenu *pStyleMenu		= new QMenu(this);
	QStringList styleList	= QStyleFactory::keys();

	for( int i=0; i<styleList.count(); i++ )
	{
		ATNamedAction *pNamedAction = new ATNamedAction( styleList.at(i), this );
		ATASSERT( connect( pNamedAction, SIGNAL( signalTriggerNamed(QString) ), this, SLOT(slotChangeStyle(QString))) );
		pStyleMenu->addAction(pNamedAction);
	}

	QMenu *pHelpMenu	= new QMenu(this);
	pAction				= new QAction(QObject::tr("&About..."), this);
//	pAction->setShortcut(QObject::tr("F1"));
	ATASSERT( QObject::connect(pAction, SIGNAL(triggered()), this, SLOT(slotShowAbout())) );
	pHelpMenu->addAction(pAction);

	menuBar()->addMenu(pFileMenu)->setText(QObject::tr("File"));
	menuBar()->addMenu(pStyleMenu)->setText(QObject::tr("Style"));
	menuBar()->addMenu(pHelpMenu)->setText(QObject::tr("Help"));

	return true;
}

//++
// Details:	None
// Args:	None
// Return:	None
//--
ATMainWindow_c::~ATMainWindow_c()
{
	writeSettings();
}

//++
// Details:	None
// Args:	None
// Return:	None
//--
void ATMainWindow_c::readSettings()
{
	QSettings settings( g_strIniFile, QSettings::IniFormat);
	QPoint pos = settings.value( "pos", QPoint(0, 0) ).toPoint();
	QSize size = settings.value( "size", QSize(800, 480) ).toSize();
	int bMax = settings.value( "maximized", 0 ).toInt();
	m_strStyle = settings.value( "qtstyle", "" ).toString();

	// ATDEBUG( "Read Settings:" );
	// ATDEBUG( "	Pos: %d, %d", pos.x(), pos.y() );
	// ATDEBUG( "	Size: %dx%d", size.width(), size.height() );
	// ATDEBUG( "	Maximized: %d", bMax );
	// ATDEBUG( "	Style: %s", qPrintable(m_strStyle) );

	if ( pos.x() && pos.y() ) move(pos);
	if ( !size.isEmpty() ) resize(size);
	if ( bMax ) showMaximized();
}

//++
// Details:	None
// Args:	None
// Return:	None
//--
void ATMainWindow_c::writeSettings()
{
	int bMax = isMaximized();

	QSettings settings( g_strIniFile, QSettings::IniFormat);
	if ( !bMax )
	{
		settings.setValue( "pos", pos() );
		settings.setValue( "size", size() );
	}
	settings.setValue( "maximized", bMax );
	settings.setValue( "qtstyle", m_strStyle );

	// ATDEBUG( "Write Settings:" );
	// ATDEBUG( "	Pos: %d, %d", pos().x(), pos().y() );
	// ATDEBUG( "	Size: %dx%d", size().width(), size().height() );
	// ATDEBUG( "	Maximized: %d", bMax );
	// ATDEBUG( "	Style: %s", qPrintable(m_strStyle) );

	settings.sync();
}

//++
// Details:	None
// Args:	None
// Return:	None
//--
void ATMainWindow_c::slotChangeStyle( QString strStyle )
{
	ATDEBUG( __FUNCTION__ );

	m_strStyle = strStyle;

	if ( !m_strStyle.isEmpty() )
	{
		QApplication::setPalette( QPalette() );
		QApplication::setStyle( strStyle );
	}
}

void ATMainWindow_c::closeEvent( QCloseEvent * event )
{
	m_pMainWindow->onClose();

	event->accept();
}

void ATMainWindow_c::slotShowAbout()
{
	QString strTitle = QString( "About %1" ).arg( APP_NICE_NAME );

	QString strAbout;
	
	strAbout += QString( "ezTunnel SSH %1 - Freeware\n" ).arg( APP_VERSION );
	strAbout += "2007 Lionel Lemarié <eztunnel@hikey.org>\n";
	strAbout += "http://eztunnelssh.hikey.org\n";
	strAbout += "Qt " QT_VERSION_STR " trolltech.com";

	QMessageBox::about( this, strTitle, strAbout );
}
