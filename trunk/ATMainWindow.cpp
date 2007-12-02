#include "pch.h"

#include "ATMainWindow.h"
#include "ATNamedAction.h"
#include "ATSkeleton.h"

#define MIN_TRIGGER_TIMER (400)
#define EZ_HOTKEY (10)

ATMainWindow_c::ATMainWindow_c( QWidget *vpParent ):
QMainWindow( vpParent ),
m_iconConnected( QPixmap( ":connected.png" ) ),
m_iconConnecting( QPixmap( ":connecting.png" ) ),
m_iconDisconnected( QPixmap( ":disconnected.png" ) ),
m_bHotkeyEnabled( false ),
m_bMinimizeToTray( false )
{
	m_pMainWindow = new ATSkeletonWindow(this);
	setCentralWidget( m_pMainWindow );

	ATVERIFY( connect( m_pMainWindow, SIGNAL( signalReadOptions() ), this, SLOT( slotReadOptions() ) ) );

	setWindowIcon( m_iconDisconnected );
	setWindowTitle( QString( "%1 %2" ).arg( APP_NICE_NAME ).arg( APP_VERSION ) );

	InitMenusAndActions();

	createTrayIcon();

	readSettings();

	updateOptions();

	DoRegisterHotKey();

	slotChangeStyle( m_strStyle );

	m_lastTrayTrigger.start();
}

void ATMainWindow_c::DoRegisterHotKey()
{
#ifdef _WIN32
	UnregisterHotKey( winId(), EZ_HOTKEY );

	if ( m_bHotkeyEnabled )
	{
		int iModifier = 0;
		int iKey = 0;

		if ( m_strHotkeyMod1 == "Ctrl" )	iModifier |= MOD_CONTROL;
		if ( m_strHotkeyMod1 == "Alt" )		iModifier |= MOD_ALT;
		if ( m_strHotkeyMod2 == "Shift" )	iModifier |= MOD_SHIFT;

		if ( !m_strHotkeyKey.isEmpty() )
		{
			iKey = m_strHotkeyKey.toUpper().at( 0 ).toLatin1();
			if ( iKey < 'A' || iKey > 'Z' ) iKey = 0;
		}

		if ( iModifier && iKey )
			RegisterHotKey( winId(), EZ_HOTKEY, iModifier, iKey );
	}
#endif
}

void ATMainWindow_c::createTrayIcon()
{
	QAction *aboutAction = new QAction(tr("&About..."), this);
	ATVERIFY( connect(aboutAction, SIGNAL(triggered()), this, SLOT(slotShowAbout())) );

	QAction *restoreAction = new QAction(tr("&Restore"), this);
	ATVERIFY( connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal())) );

	QAction *quitAction = new QAction(tr("&Quit"), this);
	ATVERIFY( connect(quitAction, SIGNAL(triggered()), this, SLOT(slotTentativeQuit())) );

	m_trayIconMenu = new QMenu( this );
	m_trayIconMenu->addAction(aboutAction);
	m_trayIconMenu->addAction(restoreAction);
	m_trayIconMenu->addAction(quitAction);

	m_trayIcon = new QSystemTrayIcon( this );
	m_trayIcon->setContextMenu( m_trayIconMenu );
	m_trayIcon->setIcon( m_iconDisconnected );
	m_trayIcon->show();

	ATVERIFY( connect( m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason))) );

	ATVERIFY( connect(m_pMainWindow, SIGNAL(signalSetTrayIcon(int)), this, SLOT(slotSetTrayIcon(int))) );
}

void ATMainWindow_c::slotSetTrayIcon( int iIndex )
{
	m_trayIcon->setIcon( iIndex ? m_iconConnected : m_iconDisconnected );
	setWindowIcon( iIndex ? m_iconConnected : m_iconDisconnected );
}

void ATMainWindow_c::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
	case QSystemTrayIcon::Trigger:
		if ( m_lastTrayTrigger.elapsed() > MIN_TRIGGER_TIMER )
		{
			if ( isVisible() )
			{
				hideWindow();
			}
			else
			{
				showNormal();
				IF_WIN32( ::SetForegroundWindow( winId() ) );
			}
			m_lastTrayTrigger.restart();
		}
		break;
	//case QSystemTrayIcon::DoubleClick:
	//	break;
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
				if ( m_bMinimizeToTray )
				{
					if ( m->wParam == SIZE_MINIMIZED )
					{
						ShowWindow(winId(), SW_HIDE);
						return true;
					}
				}
			}
			break;
		case WM_HOTKEY:
			{
				HotKey(LOWORD(m->lParam), HIWORD(m->lParam));
				return true;
			}
		default:
			break;
		}
	}

	return QWidget::winEvent(m, result);
} 
#endif

void ATMainWindow_c::HotKey( unsigned short /*lo*/, unsigned short /*hi*/ )
{
	if ( isVisible() )
	{
		hideWindow();
	}
	else
	{
		showNormal();
		IF_WIN32( ::SetForegroundWindow( winId() ) );
	}
}

bool ATMainWindow_c::InitMenusAndActions()
{
	QAction *pAction;

	QMenu *pFileMenu	= new QMenu(this);
	pAction				= new QAction(QObject::tr("&Quit"), this);
	pAction->setShortcut(QObject::tr("CTRL+Q"));
	bool bRet			= QObject::connect(pAction, SIGNAL(triggered()), this, SLOT(slotTentativeQuit()));
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

ATMainWindow_c::~ATMainWindow_c()
{
	writeSettings();
}

void ATMainWindow_c::readSettings()
{
	QSettings settings( g_strIniFile, QSettings::IniFormat);
	QPoint pos = settings.value( "pos", QPoint(0, 0) ).toPoint();
	QSize size = settings.value( "size", QSize(800, 480) ).toSize();
	int bMax = settings.value( "maximized", 0 ).toInt();
	m_strStyle = settings.value( "qtstyle", "" ).toString();
	m_strHotkeyMod1 = settings.value( "hotkey_mod1", "Ctrl" ).toString();
	m_strHotkeyMod2 = settings.value( "hotkey_mod2", "Shift" ).toString();
	m_strHotkeyKey  = settings.value( "hotkey_key",  "E" ).toString();
	m_bHotkeyEnabled = settings.value( "hotkey_enabled", true ).toBool();
	m_bMinimizeToTray = settings.value( "minimize_to_tray", true ).toBool();

	if ( pos.x() && pos.y() ) move(pos);
	if ( !size.isEmpty() ) resize(size);
	if ( bMax ) showMaximized();
}

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
	settings.setValue( "hotkey_mod1", m_strHotkeyMod1 );
	settings.setValue( "hotkey_mod2", m_strHotkeyMod2 );
	settings.setValue( "hotkey_key", m_strHotkeyKey );
	settings.setValue( "hotkey_enabled", m_bHotkeyEnabled );
	settings.setValue( "minimize_to_tray", m_bMinimizeToTray );

	settings.sync();
}

void ATMainWindow_c::slotChangeStyle( QString strStyle )
{
	qDebug( __FUNCTION__ );

	m_strStyle = strStyle;

	if ( !m_strStyle.isEmpty() )
	{
		QApplication::setPalette( QPalette() );
		QApplication::setStyle( strStyle );
	}
}

void ATMainWindow_c::closeEvent( QCloseEvent * event )
{
	if ( m_bMinimizeToTray )
	{
		hideWindow();
		event->ignore();
	}
	else
	{
		if ( m_pMainWindow->onClose() )
			event->accept();
		else
			event->ignore();
	}
}

void ATMainWindow_c::slotTentativeQuit()
{
	if ( m_pMainWindow->onClose() )
	{
		qApp->quit();
	}
}

void ATMainWindow_c::slotShowAbout()
{
	QString strTitle = QString( "About %1" ).arg( APP_NICE_NAME );

	QString strAbout;
	
	strAbout += QString( "ezTunnel SSH %1 - Freeware\n" ).arg( APP_VERSION );
	strAbout += "2007 Lionel Lemarié <eztunnel@hikey.org>\n";
	strAbout += "http://eztunnelssh.hikey.org\n";
	strAbout += "Qt " QT_VERSION_STR " trolltech.com";

	showNormal();
	QMessageBox::about( this, strTitle, strAbout );
}

namespace {
	void setComboString( QComboBox *pCombo, const QString &str )
	{
		ATASSERT( pCombo );
		if ( pCombo == NULL ) return;

		int iIndex = pCombo->findText( str );
		if ( iIndex != -1 )
			pCombo->setCurrentIndex( iIndex );
	}
}

void ATMainWindow_c::slotReadOptions()
{
	qDebug( "%s", __FUNCTION__ );

	QString strHotkeyMod1 = m_pMainWindow->ui.comboHotkey1->currentText();
	QString strHotkeyMod2 = m_pMainWindow->ui.comboHotkey2->currentText();
	QString strHotkeyKey  = m_pMainWindow->ui.editHotkey->text();
	bool bHotkeyEnabled = m_pMainWindow->ui.groupHotkey->isChecked();

	if ( (strHotkeyKey!=m_strHotkeyKey) || (strHotkeyMod1!=m_strHotkeyMod1) || (strHotkeyMod2!=m_strHotkeyMod2) || (bHotkeyEnabled!=m_bHotkeyEnabled) )
	{
		m_strHotkeyKey = strHotkeyKey;
		m_strHotkeyMod1 = strHotkeyMod1;
		m_strHotkeyMod2 = strHotkeyMod2;
		m_bHotkeyEnabled = bHotkeyEnabled;

		DoRegisterHotKey();
	}

	m_bMinimizeToTray = m_pMainWindow->ui.checkMinimizeToTray->isChecked();
}

void ATMainWindow_c::updateOptions()
{
	m_pMainWindow->ui.groupHotkey->setChecked( m_bHotkeyEnabled );
	setComboString( m_pMainWindow->ui.comboHotkey1, m_strHotkeyMod1 );
	setComboString( m_pMainWindow->ui.comboHotkey2, m_strHotkeyMod2 );
	m_pMainWindow->ui.editHotkey->setText( m_strHotkeyKey );
	m_pMainWindow->ui.checkMinimizeToTray->setChecked( m_bMinimizeToTray );
}

void ATMainWindow_c::hideWindow()
{
	if ( m_bMinimizeToTray )
	{
		hide();
	}
	else
	{
		showMinimized();
	}
}
