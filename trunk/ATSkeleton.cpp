#include "ATSkeleton.h"

#define PAGE_CONNECT	(0)
#define PAGE_EDIT		(1)
#define PAGE_OPTIONS	(2)

#define MAX_BUFFER_SIZE (4*1024)
#define WAIT_FOR_FINISHED_TIMEOUT (100)
#define CONNECTION_RETRIES (10)

#define INVALID_PASSWORD "ÇÇÇÇÇ"

ATSkeletonWindow::ATSkeletonWindow(QWidget *parent)
: QWidget(parent)
{
	m_pTunnelEdit = NULL;
	m_bDisableChanges = false;
	m_timerReadOptions.setInterval( 200 );

	ui.setupUi(this);

	ui.tabWidget->setCurrentIndex( PAGE_CONNECT );

	wireSignals();

	readSettings();

	if ( ui.treeTunnels->topLevelItemCount() == 0 )
		slotAddTunnel();

	updateControls();

	emit signalAutoConnect(NULL); // auto connect all
}

ATSkeletonWindow::~ATSkeletonWindow()
{
	writeSettings();

	// Make sure we won't try to reconnect
	ATVERIFY( disconnect( this, SIGNAL( signalAutoConnect(Tunnel_c*) ), 0, 0 ) );

	for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
	{
		disconnectTunnel( *it );
		ATASSERT( it->pProcess == NULL );
	}
}

void ATSkeletonWindow::wireSignals()
{
	// Connect buttons
	ATVERIFY( connect( ui.btnAddTunnel,    SIGNAL( clicked() ), this, SLOT( slotAddTunnel() ) ) );
	ATVERIFY( connect( ui.btnSave,         SIGNAL( clicked() ), this, SLOT( slotSave() ) ) );
	ATVERIFY( connect( ui.btnEditTunnel,   SIGNAL( clicked() ), this, SLOT( slotEditTunnel() ) ) );
	ATVERIFY( connect( ui.btnDuplicate,    SIGNAL( clicked() ), this, SLOT( slotDuplicateTunnel() ) ) );
	ATVERIFY( connect( ui.btnDeleteTunnel, SIGNAL( clicked() ), this, SLOT( slotDeleteTunnel() ) ) );
	ATVERIFY( connect( ui.btnConnect,      SIGNAL( clicked() ), this, SLOT( slotConnect() ) ) );
	ATVERIFY( connect( ui.btnDisconnect,   SIGNAL( clicked() ), this, SLOT( slotDisconnect() ) ) );
	ATVERIFY( connect( ui.btnBrowseSSHKeyFile,   SIGNAL( clicked() ), this, SLOT( slotBrowseKeyFile() ) ) );

	// Connect option related controls
	ATVERIFY( connect( &m_timerReadOptions,		SIGNAL( timeout() ),					this, SLOT( slotReadOptions() ) ) );
	ATVERIFY( connect( ui.editHotkey,			SIGNAL( textChanged(const QString&) ),	this, SLOT( slotDelayReadOptions() ) ) );
	ATVERIFY( connect( ui.comboHotkey1,			SIGNAL( currentIndexChanged(int) ),		this, SLOT( slotDelayReadOptions() ) ) );
	ATVERIFY( connect( ui.comboHotkey2,			SIGNAL( currentIndexChanged(int) ),		this, SLOT( slotDelayReadOptions() ) ) );
	ATVERIFY( connect( ui.checkMinimizeToTray,	SIGNAL( stateChanged(int) ),			this, SLOT( slotDelayReadOptions() ) ) );
	ATVERIFY( connect( ui.checkConfirmOnQuit,	SIGNAL( stateChanged(int) ),			this, SLOT( slotDelayReadOptions() ) ) );
	ATVERIFY( connect( ui.groupHotkey,			SIGNAL( toggled(bool) ),				this, SLOT( slotDelayReadOptions() ) ) );

	// Connect tab widget
	ATVERIFY( connect( ui.tabWidget, SIGNAL( currentChanged(int) ), this, SLOT( slotTabChanged() ) ) );

	// Connect tree
	ATVERIFY( connect( ui.treeTunnels, SIGNAL( itemSelectionChanged() ), this, SLOT( slotSelectTunnel() ) ) );
	ATVERIFY( connect( ui.treeTunnels, SIGNAL( activated(const QModelIndex &) ), this, SLOT( slotItemActivated() ) ) );

	// Connect buttons
	ATVERIFY( connect( this, SIGNAL( signalAutoConnect(Tunnel_c*) ), this, SLOT( slotAutoConnect(Tunnel_c*) ), Qt::QueuedConnection ) );
}

void ATSkeletonWindow::readSettings()
{
	QSettings settings( g_strIniFile, QSettings::IniFormat);

	m_listTunnels.clear();
	ui.treeTunnels->clear();

	ui.checkConfirmOnQuit->setChecked( settings.value( "ConfirmOnQuit", true ).toBool() );

	int iCount = settings.value( "NumberOfTunnels", 0 ).toInt();

	Tunnel_c tunnel;

	for ( int i=0; i<iCount; i++ )
	{
		QString strGroup = QString( "Tunnel%1" ).arg( i );
		settings.beginGroup( strGroup );

		tunnel.strName           = settings.value( "Name" ).toString();
		tunnel.strSSHHost        = settings.value( "SSHHost" ).toString();
		tunnel.strRemoteHost     = settings.value( "RemoteHost" ).toString();
		tunnel.strUsername       = settings.value( "Username" ).toString();
		tunnel.strSSHKeyFile     = settings.value( "SSHKeyFile" ).toString();
		tunnel.strExtraArguments = settings.value( "ExtraArguments" ).toString();
		tunnel.iLocalPort        = settings.value( "LocalPort" ).toInt();
		tunnel.iRemotePort       = settings.value( "RemotePort" ).toInt();
		tunnel.iDirection        = settings.value( "Direction" ).toInt();
		tunnel.bAutoConnect      = settings.value( "AutoConnect" ).toBool();
		tunnel.bCompression      = settings.value( "Compression" ).toBool();
		tunnel.bDoKeepAlivePing  = settings.value( "DoKeepAlivePing" ).toBool();
		tunnel.bAutoReconnect    = settings.value( "AutoReconnect" ).toBool();
		tunnel.iSSH1or2          = settings.value( "SSH1or2" ).toInt();

		settings.endGroup();

		tunnel.twi = new QTreeWidgetItem( ui.treeTunnels );
		tunnel.twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
		tunnel.twi->setText( 0, tunnel.strName );

		m_listTunnels.push_back( tunnel );
	}

	if ( ui.treeTunnels->topLevelItemCount() > 0 )
	{
		QTreeWidgetItem *twi = ui.treeTunnels->topLevelItem( 0 );
		ui.treeTunnels->setCurrentItem( twi );
	}
}

void ATSkeletonWindow::writeSettings()
{
	QSettings settings( g_strIniFile, QSettings::IniFormat);

	settings.setValue( "ConfirmOnQuit", ui.checkConfirmOnQuit->isChecked() );

	settings.setValue( "NumberOfTunnels", m_listTunnels.size() );

	int i=0;
	for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
	{
		QString strGroup = QString( "Tunnel%1" ).arg( i );
		settings.beginGroup( strGroup );

		settings.setValue( "Name",            it->strName );
		settings.setValue( "SSHHost",         it->strSSHHost );
		settings.setValue( "LocalPort",       it->iLocalPort );
		settings.setValue( "RemoteHost",      it->strRemoteHost );
		settings.setValue( "RemotePort",      it->iRemotePort );
		settings.setValue( "Username",        it->strUsername );
		settings.setValue( "SSHKeyFile",      it->strSSHKeyFile );
		settings.setValue( "Direction",       it->iDirection );
		settings.setValue( "AutoConnect",     it->bAutoConnect );
		settings.setValue( "Compression",     it->bCompression );
		settings.setValue( "DoKeepAlivePing", it->bDoKeepAlivePing );
		settings.setValue( "AutoReconnect",   it->bAutoReconnect );
		settings.setValue( "SSH1or2",         it->iSSH1or2 );
		settings.setValue( "ExtraArguments",  it->strExtraArguments );

		settings.endGroup();

		i++;
	}

	settings.sync();
}

void ATSkeletonWindow::updateControls()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();

	ui.btnEditTunnel->setEnabled( twi != NULL );
	ui.btnDuplicate->setEnabled( twi != NULL );
	ui.btnDeleteTunnel->setEnabled( twi != NULL );
	ui.btnConnect->setEnabled( twi != NULL );
	ui.btnDisconnect->setEnabled( twi != NULL );
}

void ATSkeletonWindow::slotTabChanged()
{
	int iTabIndex = ui.tabWidget->currentIndex();
	if ( iTabIndex == PAGE_CONNECT )
	{
		if ( detectTunnelChange() )
		{
			confirmSaveTunnel();
		}
	}
}

void ATSkeletonWindow::slotEditTunnel()
{
	ui.tabWidget->setCurrentIndex( PAGE_EDIT );
}

void ATSkeletonWindow::slotDeleteTunnel()
{
 	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	ATASSERT( twi );
 	if ( twi == NULL ) return;
 
	Tunnel_c *pt = getTunnelFromTreeItem( twi );
	ATASSERT( pt );
	if ( pt == NULL ) return;

 	QString strQuestion = QString( "Do you want to delete the tunnel \"%1\"?" ).arg( twi->text( 0 ) );
 	QMessageBox::StandardButton iRet = QMessageBox::question( this, APP_NICE_NAME, strQuestion, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
 
 	if ( iRet == QMessageBox::Yes )
 	{
 		m_bDisableChanges = true;
 
 		delete twi;
 		disconnectTunnel( *pt );
 
		for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
		{
			if ( it->twi == twi )
			{
				m_listTunnels.erase( it );
				break;
			}
		}
 	}
}

void ATSkeletonWindow::slotConnect()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	ATASSERT( twi );
	if ( twi == NULL ) return;

	Tunnel_c *pt = getTunnelFromTreeItem( twi );
	ATASSERT( twi );

	connectTunnel( *pt );
}

void ATSkeletonWindow::slotDisconnect()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	ATASSERT( twi );
	if ( twi == NULL ) return;

	Tunnel_c *pt = getTunnelFromTreeItem( twi );
	ATASSERT( pt );
	pt->iShouldReconnect = 0;

	disconnectTunnel( *pt );
}

void ATSkeletonWindow::slotItemActivated()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi == NULL ) return;

	ui.tabWidget->setCurrentIndex( PAGE_CONNECT );

	const Tunnel_c *pt = getTunnelFromTreeItem( twi );
	ATASSERT( pt );

	if (pt->pProcess)
		slotDisconnect();
	else
		slotConnect();
}

void ATSkeletonWindow::slotBrowseKeyFile()
{
	QString strFilename = QFileDialog::getOpenFileName( this, "Please specify the SSH key file to use" );

	if ( !strFilename.isNull() )
	{
		ui.editSSHKeyFile->setText( strFilename );
	}
}

void ATSkeletonWindow::slotConnectorFinished( Tunnel_c *pt )
{
	if ( pt ) disconnectTunnel( *pt );
}

Tunnel_c *ATSkeletonWindow::getTunnelFromTreeItem( const QTreeWidgetItem *twi )
{
	if ( twi == NULL ) return NULL;

	for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
	{
		if ( it->twi == twi )
			return &(*it);
	}

	return NULL;
}

void ATSkeletonWindow::connected( Tunnel_c &tunnel )
{
 	if ( tunnel.twi != NULL )
 	{
 		tunnel.twi->setIcon( 0, QPixmap( ":connected.png" ) );
 		emit signalSetTrayIcon( 1 );
 	}
 
	tunnel.iShouldReconnect = tunnel.bAutoReconnect ? CONNECTION_RETRIES : 0;

 	AddToLog( tunnel, "Connected." );
}

void ATSkeletonWindow::connectTunnel( Tunnel_c &tunnel )
{
 	Tunnel_c *pt = &tunnel;
 
 	AddToLog( tunnel, "Connecting tunnel %s...", qPrintable( pt->strName ) );
 	AddToLog( tunnel, "Host: %s - Tunnel: %d:%s:%d", qPrintable( pt->strSSHHost ), pt->iLocalPort, qPrintable( pt->strRemoteHost ), pt->iRemotePort );
 
 	if ( pt->strSSHHost.isEmpty() )
 	{
 		AddToLog( tunnel, "Error: Tunnel %s has no host, please check the settings.", qPrintable( pt->strName ) );
 		return;
 	}
 
 #ifdef WIN32
 	QString strPlink = "plink.exe";
 
 	// Check that the executable is found
 	{
 		QDir dir( argv0 );
 		dir.cdUp();
 		if ( !dir.exists( strPlink ) )
 		{
 			const char *ptr = argv0, *ptr2 = argv0;
 			while ( *ptr2 ) { if ( *ptr2 == '\\' ) ptr = ptr2+1; ptr2++; }
 			AddToLog( tunnel, "Error: Could not find %s, please check that it is in the same directory as %s.", qPrintable( strPlink ), ptr );
 			return;
 		}
 	}
 #else
 	QString strPlink = "ssh";
 #endif
 
 	if ( pt->pProcess != NULL ) return; // already connected?

	if ( tunnel.twi ) tunnel.twi->setIcon( 0, QPixmap( ":connecting.png" ) );
  	//QTreeWidgetItem *twi = getTreeItemFromTunnelIndex( iTunnelIndex );
  	//if ( twi ) twi->setIcon( 0, QPixmap( ":connecting.png" ) );
 
 	ATASSERT( pt->pConnector == NULL );
 	pt->pConnector = new ATTunnelConnector_c( this, &tunnel );
 
 	pt->pProcess = new QProcess;
 	pt->pProcess->setProcessChannelMode( QProcess::MergedChannels );
 
 	QString strCommand;
 
 	strCommand += strPlink + " ";
 	strCommand += "-v ";
 
 #ifdef _WIN32
 	QStringList strListSSHHost = pt->strSSHHost.split( ':', QString::SkipEmptyParts );
 	if ( strListSSHHost.count() == 1 ) strListSSHHost << "22";
 
 	strCommand += strListSSHHost.at(0) + " -P " + strListSSHHost.at(1) + " ";
 #else
 	strCommand += pt->strSSHHost + " ";
 #endif
 
 	if ( !pt->strUsername.isEmpty() ) strCommand += QString( "-l %1 " ).arg( pt->strUsername );
 
 	if ( pt->bCompression ) strCommand += "-C ";
 
 	strCommand += ( pt->iSSH1or2 == 1 ) ? "-1 " : "-2 ";
 
 	if ( pt->iDirection == 0 ) // Local -> Remote
 	{
 		IF_NWIN32( strCommand += "-g " );
 		strCommand += QString( "-L %1:%2:%3 " ).arg( pt->iLocalPort ).arg( pt->strRemoteHost).arg( pt->iRemotePort );		
 	}
 	else // Remote -> Local
 	{
 		strCommand += QString( "-R %1:%2:%3 " ).arg( pt->iLocalPort ).arg( pt->strRemoteHost).arg( pt->iRemotePort );		
 	}
 
 	if ( !pt->strSSHKeyFile.isEmpty() ) strCommand += QString( "-i %1 " ).arg( pt->strSSHKeyFile );
 
 	strCommand += pt->strExtraArguments;
 
 	ATVERIFY( connect( pt->pProcess, SIGNAL( readyReadStandardOutput() ), pt->pConnector, SLOT( slotProcessReadStandardOutput() ) ) );
 	ATVERIFY( connect( pt->pProcess, SIGNAL( readyReadStandardError() ), pt->pConnector, SLOT( slotProcessReadStandardError() ) ) );
 	ATVERIFY( connect( pt->pProcess, SIGNAL( error(QProcess::ProcessError) ), pt->pConnector, SLOT( slotProcessError(QProcess::ProcessError) ) ) );
 	ATVERIFY( connect( pt->pProcess, SIGNAL( finished(int, QProcess::ExitStatus) ), pt->pConnector, SLOT( slotProcessFinished(int, QProcess::ExitStatus) ) ) );
 	ATVERIFY( connect( pt->pConnector, SIGNAL( finished(Tunnel_c*) ), this, SLOT( slotConnectorFinished(Tunnel_c*) ), Qt::QueuedConnection ) );
 
 	AddToLog( tunnel, "%s", qPrintable( strCommand ) );
 
 	pt->pProcess->start( strCommand );
}

void ATSkeletonWindow::disconnectTunnel( Tunnel_c &tunnel )
{
	Tunnel_c *pt = &tunnel;

	if ( pt->pProcess == NULL ) return; // not connected?

	AddToLog( tunnel, "Disconnecting tunnel %s...", qPrintable( pt->strName ) );
	qApp->processEvents();

	pt->pProcess->kill();
	bool bOk = pt->pProcess->waitForFinished( WAIT_FOR_FINISHED_TIMEOUT );
	Q_UNUSED( bOk );
	delete pt->pProcess;
	pt->pProcess = NULL;

	delete pt->pConnector;
	pt->pConnector = NULL;

	AddToLog( tunnel, "Disconnected." );

	if ( tunnel.twi ) tunnel.twi->setIcon( 0, QPixmap( ":disconnected.png" ) );

	int iConnectedCount = 0;

	for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
		if ( it->pProcess ) iConnectedCount++;

	if ( iConnectedCount == 0 )
		emit signalSetTrayIcon( 0 );

	if ( pt->iShouldReconnect > 0 )
	{
		AddToLog( tunnel, "Connection lost, reconnecting... (%d)", pt->iShouldReconnect );
		pt->iShouldReconnect--;
		emit signalAutoConnect( &tunnel );
	}
	else
	{
		AddToLog( tunnel, "Connection lost, giving up." );
	}
}

void ATSkeletonWindow::slotSave()
{
	saveTunnel();

	ui.tabWidget->setCurrentIndex( PAGE_CONNECT );
}

void ATSkeletonWindow::saveTunnel()
{
 	Tunnel_c *pt;
 
 	if ( m_pTunnelEdit ) pt = m_pTunnelEdit;
 	else pt = new Tunnel_c;
  
 	pt->strName = ui.editTunnelName->text();
 	pt->strSSHHost = ui.editSSHHost->text();
 	pt->iLocalPort = ui.editLocalPort->text().toInt();
 	pt->strRemoteHost = ui.editRemoteHost->text();
 	pt->iRemotePort = ui.editRemotePort->text().toInt();
 	pt->strExtraArguments = ui.editExtraArguments->text();
 	pt->strUsername = ui.editUsername->text();
 	pt->strSSHKeyFile = ui.editSSHKeyFile->text();
 	pt->iDirection = ui.comboDirection->currentIndex();
 	pt->bAutoConnect = ui.checkAutoConnect->isChecked();
 	pt->bCompression = ui.checkCompression->isChecked();
 	pt->bDoKeepAlivePing = ui.checkDoKeepAlivePing->isChecked();
 	pt->bAutoReconnect = ui.checkAutoReconnect->isChecked();
 	pt->iSSH1or2 = ui.radioSSH1->isChecked() ? 1 : 2;
 
 	if ( m_pTunnelEdit == NULL )
 	{
 		slotAddTunnel();
 		m_bDisableChanges = true;
 		qApp->processEvents();
 	}
 
 	if ( m_pTunnelEdit )
 	{
 		//m_listTunnels[m_iEditIndex] = *pt;
 
 		writeSettings();
 
 		if ( m_pTunnelEdit->twi != NULL ) m_pTunnelEdit->twi->setText( 0, pt->strName );
 		slotSelectTunnel();
 		qApp->processEvents();
 		m_bDisableChanges = false;
 	}
}

void ATSkeletonWindow::slotAddTunnel()
{
 	Tunnel_c tunnel;
 
 	QTreeWidgetItem *twi = new QTreeWidgetItem( ui.treeTunnels );
 	twi->setText( 0, tunnel.strName );
 	twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
	tunnel.twi = twi;

	m_listTunnels.push_back( tunnel );

 	ui.treeTunnels->setCurrentItem( twi );
 
 	ui.tabWidget->setCurrentIndex( PAGE_EDIT );
 	ui.editTunnelName->setFocus();
 	qApp->processEvents();
 	ui.editTunnelName->selectAll();
}

void ATSkeletonWindow::slotDuplicateTunnel()
{
 	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	ATASSERT( twi );
	if ( twi == NULL ) return;
 
	Tunnel_c *pt = getTunnelFromTreeItem( twi );
	ATASSERT( pt );
	if ( pt == NULL ) return;

	Tunnel_c tunnel;
	tunnel.copyFrom( pt );

 	twi = new QTreeWidgetItem( ui.treeTunnels );
 	twi->setText( 0, tunnel.strName );
 	twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
	tunnel.twi = twi;

	m_listTunnels.push_back( tunnel );

 	ui.treeTunnels->setCurrentItem( twi );
 
 	ui.tabWidget->setCurrentIndex( PAGE_EDIT );
 	ui.editTunnelName->setFocus();
 	qApp->processEvents();
 	ui.editTunnelName->selectAll();
}

void ATSkeletonWindow::slotSelectTunnel()
{
	if ( detectTunnelChange() ) confirmSaveTunnel();

	m_pTunnelEdit = NULL;

	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi == NULL )
	{
		populateEditUIFromStruct( NULL );
		return;
	}

	for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
	{
		if ( it->twi == twi )
		{
			populateEditUIFromStruct( &(*it) );

			// Must set it only after calling populateEditUIFromStruct
			m_pTunnelEdit = &(*it);
		}
	}

	updateControls();
}

void ATSkeletonWindow::populateEditUIFromStruct( Tunnel_c *pt )
{
	if ( pt == NULL )
	{
		ui.editTunnelName->clear();
		ui.editSSHHost->clear();
		ui.editRemoteHost->clear();
		ui.editExtraArguments->clear();
		ui.editUsername->clear();
		ui.editSSHKeyFile->clear();
		ui.comboDirection->setCurrentIndex( 0 );
		ui.checkAutoConnect->setChecked( false );
		ui.checkCompression->setChecked( false );
		ui.checkDoKeepAlivePing->setChecked( false );
		ui.checkAutoReconnect->setChecked( false );
		ui.radioSSH2->setChecked( false );
		ui.radioSSH1->setChecked( false );
		ui.editLocalPort->clear();
		ui.editRemotePort->clear();
		ui.textBrowser->clear();

		return;
	}

	ui.editTunnelName->setText( pt->strName );
	ui.editSSHHost->setText( pt->strSSHHost );
	ui.editRemoteHost->setText( pt->strRemoteHost );
	ui.editExtraArguments->setText( pt->strExtraArguments );
	ui.editUsername->setText( pt->strUsername );
	ui.editSSHKeyFile->setText( pt->strSSHKeyFile );
	ui.comboDirection->setCurrentIndex( pt->iDirection );
	ui.checkAutoConnect->setChecked( pt->bAutoConnect );
	ui.checkCompression->setChecked( pt->bCompression );
	ui.checkDoKeepAlivePing->setChecked( pt->bDoKeepAlivePing );
	ui.checkAutoReconnect->setChecked( pt->bAutoReconnect );
	ui.radioSSH1->setChecked( pt->iSSH1or2 == 1 );
	ui.radioSSH2->setChecked( pt->iSSH1or2 != 1 );

	if ( pt->iLocalPort )
		ui.editLocalPort->setText( QString::number( pt->iLocalPort ) );
	else ui.editLocalPort->clear();

	if ( pt->iRemotePort )
		ui.editRemotePort->setText( QString::number( pt->iRemotePort ) );
	else ui.editRemotePort->clear();

	ui.textBrowser->setHtml( pt->byteLog );
	ui.textBrowser->verticalScrollBar()->setValue( ui.textBrowser->verticalScrollBar()->maximum() );
}

bool ATSkeletonWindow::detectTunnelChange()
{
 	if ( m_bDisableChanges ) return false;
 
 	if ( m_pTunnelEdit == NULL ) return false;
 	const Tunnel_c *pt = m_pTunnelEdit;
 
 	if ( ui.editTunnelName->text()				!= pt->strName				) return true;
 	if ( ui.editSSHHost->text()					!= pt->strSSHHost			) return true;
 	if ( ui.editLocalPort->text().toInt()		!= pt->iLocalPort			) return true;
 	if ( ui.editRemoteHost->text()				!= pt->strRemoteHost		) return true;
 	if ( ui.editRemotePort->text().toInt()		!= pt->iRemotePort			) return true;
 	if ( ui.editExtraArguments->text()			!= pt->strExtraArguments	) return true;
 	if ( ui.editUsername->text()				!= pt->strUsername			) return true;
 	if ( ui.editSSHKeyFile->text()				!= pt->strSSHKeyFile		) return true;
 	if ( ui.comboDirection->currentIndex()		!= pt->iDirection			) return true;
 	if ( ui.checkAutoConnect->isChecked()		!= pt->bAutoConnect			) return true;
 	if ( ui.checkCompression->isChecked()		!= pt->bCompression			) return true;
 	if ( ui.checkDoKeepAlivePing->isChecked()	!= pt->bDoKeepAlivePing		) return true;
 	if ( ui.checkAutoReconnect->isChecked()		!= pt->bAutoReconnect		) return true;
 	if ( ui.radioSSH1->isChecked()				!= ( pt->iSSH1or2 == 1 )	) return true;
 	if ( ui.radioSSH2->isChecked()				!= ( pt->iSSH1or2 != 1 )	) return true;

	return false;
}

void ATSkeletonWindow::confirmSaveTunnel()
{
 	QMessageBox::StandardButton iRet = QMessageBox::question( this, APP_NICE_NAME, "Do you want to save the tunnel changes?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
 
 	if ( iRet == QMessageBox::Yes )
 	{
 		saveTunnel();
 	}
}

bool ATSkeletonWindow::askforPassword( Tunnel_c &tunnel )
{
	Tunnel_c *pt = &tunnel;

	// Early out if the user has already provided a password.
	if ( pt->strPassword != INVALID_PASSWORD )
		return true;

	QDialog dlg;

	dlg.setWindowTitle( APP_NICE_NAME );

	QLabel *pLabel = new QLabel( &dlg );
	pLabel->setText( QString( "Password for user '%1' on host '%2':" ).arg( pt->strUsername ).arg( pt->strSSHHost ) );

	QLineEdit *pEdit = new QLineEdit( &dlg );
	pEdit->setEchoMode( QLineEdit::Password );

	QPushButton *pButton = new QPushButton( &dlg );
	pButton->setText( "Ok" );
	pButton->setDefault( true );
	ATVERIFY( connect( pButton, SIGNAL( clicked() ), &dlg, SLOT( accept() ) ) );

	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget( pLabel );
	vbox->addWidget( pEdit );
	vbox->addWidget( pButton );
	dlg.setLayout( vbox );

	if ( dlg.exec() )
	{
		pt->strPassword = pEdit->text();
		return true;
	}

	return false;
}

bool ATSkeletonWindow::onClose()
{
	if ( detectTunnelChange() )
	{
		confirmSaveTunnel();
	}

	if ( ui.checkConfirmOnQuit->isChecked() )
	{
		int iConnectionCount = 0;
		for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
		{
			if ( it->pProcess )
			{
				iConnectionCount++;
			}
		}

		if ( iConnectionCount )
		{
			QString strQuestion;

			if ( iConnectionCount == 1 )
				strQuestion = "There is 1 active tunnel connection.";
			else
				strQuestion = QString( "There are %1 active tunnel connections." ).arg( iConnectionCount );

			strQuestion += "\nAre you sure you want to disconnect and quit?";


			QMessageBox::StandardButton iRet = QMessageBox::question( this, APP_NICE_NAME, strQuestion, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

			if ( iRet == QMessageBox::No )
				return false;
		}
	}

	return true;
}

static int safe_vsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
{
	if (count <= 0)
		return 0;

	IF_WIN32(  int nReturn = _vsnprintf_s(buffer, count, count, format, argptr) );
	IF_NWIN32( int nReturn = vsnprintf(buffer, count, format, argptr) );

	buffer[count-1] = '\0';

	return nReturn;
}

// void ATSkeletonWindow::AddToLog( int iTunnelIndex, const char *format, ... )
void ATSkeletonWindow::AddToLog( Tunnel_c &tunnel, const char *format, ... )
{
 	char buffer[8096];
 
 	va_list argptr;
 	va_start(argptr, format);
 
 	safe_vsnprintf(buffer, sizeof(buffer), format, argptr);
 
 	va_end(argptr);
 
 	QString str( buffer );
 	str.replace( '\n', "<br>" );
 
 	//Tunnel_c *pt = &m_listTunnels[iTunnelIndex];
	Tunnel_c *pt = &tunnel;
	pt->byteLog.append( str );
 	pt->byteLog.append( "<br>" );
 
 	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
 	if ( ( twi != NULL ) && ( twi != tunnel.twi ) )
 		twi = NULL;
 
 	if ( pt->byteLog.size() > (MAX_BUFFER_SIZE*2) )
 	{
 		pt->byteLog = pt->byteLog.right( MAX_BUFFER_SIZE );
 		if ( twi != NULL )
 		{
 			ui.textBrowser->setHtml( pt->byteLog );
 			ui.textBrowser->verticalScrollBar()->setValue( ui.textBrowser->verticalScrollBar()->maximum() );
 		}
 	}
 
 	if ( twi != NULL ) ui.textBrowser->append( str );
}

void ATSkeletonWindow::slotAutoConnect( Tunnel_c *pt )
{
	// If it's a valid index, auto connect it, otherwise auto connect all that were ticked in the settings.
	if ( pt )
	{
		connectTunnel( *pt );
		return;
	}

	bool bSelected = false;
	for ( TunnelInterator it = m_listTunnels.begin(); it != m_listTunnels.end(); ++it )
	{
		if ( it->bAutoConnect )
		{
			if ( !bSelected && it->twi )
			{
				bSelected = true;
				ui.treeTunnels->setCurrentItem( it->twi );
			}

			connectTunnel( *it );
		}
	}
}

void ATSkeletonWindow::slotDelayReadOptions()
{
	m_timerReadOptions.stop();
	m_timerReadOptions.start();
}

void ATSkeletonWindow::slotReadOptions()
{
	m_timerReadOptions.stop();
	emit signalReadOptions();
}




// Tunnel_c

Tunnel_c::Tunnel_c()
{
	pProcess = NULL;
	pConnector = NULL;
	strPassword = INVALID_PASSWORD;
	iShouldReconnect = 0;
	twi = NULL;

	init();
}

Tunnel_c::~Tunnel_c()
{
	// The process should be terminated before a tunnel is destroyed
	ATASSERT( pProcess == NULL );
	ATASSERT( pConnector == NULL );
}

void Tunnel_c::init()
{
	strName = "Untitled";
	bCompression = true;
	bDoKeepAlivePing = false;
	bAutoReconnect = true;
	iSSH1or2 = 2;
	iLocalPort = 0;
	iRemotePort = 0;
	iDirection = 0;
	bAutoConnect = false;
	strExtraArguments = "-N";
}

void Tunnel_c::copyFrom( const Tunnel_c *orig )
{
	strName = orig->strName + " Copy";
	strSSHHost = orig->strSSHHost;
	iLocalPort = orig->iLocalPort;
	strRemoteHost = orig->strRemoteHost;
	iRemotePort = orig->iRemotePort;
	strUsername = orig->strUsername;
	strSSHKeyFile = orig->strSSHKeyFile;
	iDirection = orig->iDirection;
	bAutoConnect = orig->bAutoConnect;
	bCompression = orig->bCompression;
	bDoKeepAlivePing = orig->bDoKeepAlivePing;
	bAutoReconnect = orig->bAutoReconnect;
	iSSH1or2 = orig->iSSH1or2;
	strExtraArguments = orig->strExtraArguments;

	pProcess = NULL;
	pConnector = NULL;
	strPassword = orig->strPassword;
	iShouldReconnect = 0;
}

// ATTunnelConnector_c

ATTunnelConnector_c::ATTunnelConnector_c( ATSkeletonWindow *pParent, Tunnel_c *pTunnel ) : QObject(),
m_pParent( pParent ),
m_pTunnel( pTunnel )
{
	ATASSERT( m_pParent );
	ATASSERT( m_pTunnel );
}

#ifndef _WIN32
#define strtok_s(a,b,c) strtok(a,b)
#endif

void ATTunnelConnector_c::slotProcessReadStandardOutput()
{
	ATASSERT( m_pParent );
	ATASSERT( m_pTunnel );

	Tunnel_c *pt = m_pTunnel;
	ATASSERT( pt );

	QByteArray b = pt->pProcess->readAllStandardOutput();

	char *ptrb = b.data();
	char *context;
	context = NULL;
	char *ptr = strtok_s( ptrb, "\r\n", &context );

	while ( ptr )
	{
		m_pParent->AddToLog( *pt, "1> %s", ptr );
		processPlinkOutput( ptr );
		ptr = strtok_s( NULL, "\r\n", &context );
	}
}

void ATTunnelConnector_c::slotProcessReadStandardError()
{
	ATASSERT( m_pParent );
	ATASSERT( m_pTunnel );

	Tunnel_c *pt = m_pTunnel;
	ATASSERT( pt );

	QByteArray b = pt->pProcess->readAllStandardError();

	char *ptrb = b.data();
	char *context;
	context = NULL;
	char *ptr = strtok_s( ptrb, "\r\n", &context );

	while ( ptr )
	{
		m_pParent->AddToLog( *pt, "2> %s", ptr );
		processPlinkOutput( ptr );
		ptr = strtok_s( NULL, "\r\n", &context );
	}
}

void ATTunnelConnector_c::slotProcessError(QProcess::ProcessError error)
{
	ATASSERT( m_pParent );
	ATASSERT( m_pTunnel );

	Tunnel_c *pt = m_pTunnel;
	ATASSERT( pt );

	m_pParent->AddToLog( *pt, "Process error: %d", error );
	
	emit finished( pt );
}

void ATTunnelConnector_c::slotProcessFinished(int exitCode, QProcess::ExitStatus /*exitStatus*/)
{
	ATASSERT( m_pParent );
	ATASSERT( m_pTunnel );

	Tunnel_c *pt = m_pTunnel;
	ATASSERT( pt );

	m_pParent->AddToLog( *pt, "Process exit: %d", exitCode );

	emit finished( pt );
}

void ATTunnelConnector_c::processPlinkOutput( const char *str )
{
	if ( !m_pParent ) return;

	if ( strstr( str, "assword:" ) )
	{
		if ( m_pTunnel )
		{
			if ( !m_pParent->askforPassword( *m_pTunnel ) )
			{
				emit finished( m_pTunnel );
			}
			else if ( m_pTunnel->pProcess )
			{
				QString strStarPassword( m_pTunnel->strPassword );
				for ( int i=0; i<strStarPassword.count(); i++ )
					strStarPassword[i] = '*';
				m_pParent->AddToLog( *m_pTunnel, "Sent password: %s", qPrintable( strStarPassword ) );
				m_pTunnel->pProcess->write( qPrintable( m_pTunnel->strPassword + "\r\n" ) );
			}
		}
	}
	else if ( strstr( str, "Store key in cache? (y/n)" ) )
	{
		if ( m_pTunnel && m_pTunnel->pProcess )
		{
			m_pTunnel->pProcess->write( "n\n" );
		}
	}
	else if ( strstr( str, "Access granted" ) )
	{
		m_pParent->connected( *m_pTunnel );
	}
	else if ( strstr( str, "Authentication succeeded" ) )
	{
		m_pParent->connected( *m_pTunnel );
	}
	else if ( strstr( str, "Access denied" ) )
	{
		if ( m_pTunnel ) m_pTunnel->strPassword = INVALID_PASSWORD;
	}
}

