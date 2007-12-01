#include "ATSkeleton.h"

#define PAGE_CONNECT (0)
#define PAGE_EDIT    (1)

#define MAX_BUFFER_SIZE (4*1024)
#define WAIT_FOR_FINISHED_TIMEOUT (100)
#define CONNECTION_RETRIES (10)

#define INVALID_PASSWORD "ÇÇÇÇÇ"

ATSkeletonWindow::ATSkeletonWindow(QWidget *parent)
: QWidget(parent)
{
	m_iEditIndex = -1;
	m_bDisableChanges = false;

	ui.setupUi(this);

	ui.tabWidget->setCurrentIndex( PAGE_CONNECT );

	wireSignals();

	readSettings();

	if ( ui.treeTunnels->topLevelItemCount() == 0 )
		slotAddTunnel();

	updateControls();

	emit signalAutoConnect(-1); // auto connect all
}

ATSkeletonWindow::~ATSkeletonWindow()
{
	writeSettings();

	for ( int i=0; i<m_listTunnels.Count(); i++ )
	{
		slotConnectorFinished( i );
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

	// Connect tab widget
	ATVERIFY( connect( ui.tabWidget, SIGNAL( currentChanged(int) ), this, SLOT( slotTabChanged() ) ) );

	// Connect tree
	ATVERIFY( connect( ui.treeTunnels, SIGNAL( itemSelectionChanged() ), this, SLOT( slotSelectTunnel() ) ) );
	ATVERIFY( connect( ui.treeTunnels, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int) ), this, SLOT( slotItemDoubleClicked(QTreeWidgetItem*) ) ) );

	// Connect buttons
	ATVERIFY( connect( this, SIGNAL( signalAutoConnect(int) ), this, SLOT( slotAutoConnect(int) ), Qt::QueuedConnection ) );
}

void ATSkeletonWindow::readSettings()
{
	QSettings settings( g_strIniFile, QSettings::IniFormat);

	m_listTunnels.Clear();
	ui.treeTunnels->clear();

	int iCount = settings.value( "NumberOfTunnels", 0 ).toInt();

	for ( int i=0; i<iCount; i++ )
	{
		QString strGroup = QString( "Tunnel%1" ).arg( i );
		settings.beginGroup( strGroup );

		Tunnel_c *pt = m_listTunnels.AppendNewInstance();
		int iIndex = m_listTunnels.Count() - 1;

		pt->strName           = settings.value( "Name" ).toString();
		pt->strSSHHost        = settings.value( "SSHHost" ).toString();
		pt->strRemoteHost     = settings.value( "RemoteHost" ).toString();
		pt->strUsername       = settings.value( "Username" ).toString();
		pt->strSSHKeyFile     = settings.value( "SSHKeyFile" ).toString();
		pt->strExtraArguments = settings.value( "ExtraArguments" ).toString();
		pt->iLocalPort        = settings.value( "LocalPort" ).toInt();
		pt->iRemotePort       = settings.value( "RemotePort" ).toInt();
		pt->iDirection        = settings.value( "Direction" ).toInt();
		pt->bAutoConnect      = settings.value( "AutoConnect" ).toBool();
		pt->bCompression      = settings.value( "Compression" ).toBool();
		pt->bDoKeepAlivePing  = settings.value( "DoKeepAlivePing" ).toBool();
		pt->bAutoReconnect    = settings.value( "AutoReconnect" ).toBool();
		pt->iSSH1or2          = settings.value( "SSH1or2" ).toInt();

		settings.endGroup();

		QTreeWidgetItem *twi = new QTreeWidgetItem( ui.treeTunnels );
		twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
		twi->setText( 0, pt->strName );
		twi->setData( 0, Qt::UserRole, iIndex );
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

	settings.setValue( "NumberOfTunnels", m_listTunnels.Count() );

	for ( int i=0; i<m_listTunnels.Count(); i++ )
	{
		QString strGroup = QString( "Tunnel%1" ).arg( i );
		settings.beginGroup( strGroup );

		const Tunnel_c *pt = &m_listTunnels.at( i );

		settings.setValue( "Name", pt->strName );
		settings.setValue( "SSHHost", pt->strSSHHost );
		settings.setValue( "LocalPort", pt->iLocalPort );
		settings.setValue( "RemoteHost", pt->strRemoteHost );
		settings.setValue( "RemotePort", pt->iRemotePort );
		settings.setValue( "Username", pt->strUsername );
		settings.setValue( "SSHKeyFile", pt->strSSHKeyFile );
		settings.setValue( "Direction", pt->iDirection );
		settings.setValue( "AutoConnect", pt->bAutoConnect );
		settings.setValue( "Compression", pt->bCompression );
		settings.setValue( "DoKeepAlivePing", pt->bDoKeepAlivePing );
		settings.setValue( "AutoReconnect", pt->bAutoReconnect );
		settings.setValue( "SSH1or2", pt->iSSH1or2 );
		settings.setValue( "ExtraArguments", pt->strExtraArguments );

		settings.endGroup();
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
	if ( twi == NULL ) return;

	QString strQuestion = QString( "Do you want to delete the tunnel \"%1\"?" ).arg( twi->text( 0 ) );
	QMessageBox::StandardButton iRet = QMessageBox::question( this, APP_NICE_NAME, strQuestion, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );

	if ( iRet == QMessageBox::Yes )
	{
		m_bDisableChanges = true;

		int iIndex = twi->data( 0, Qt::UserRole ).toInt();

		delete twi;
		disconnectTunnel( iIndex );

		if ( iIndex >= 0 && iIndex < m_listTunnels.Count() )
			m_listTunnels.RemoveAt( iIndex );

		// Need to find all tree items that have a bigger index and decrement them
		for ( int i=0; i<ui.treeTunnels->topLevelItemCount(); i++ )
		{
			QTreeWidgetItem *twi = ui.treeTunnels->topLevelItem( i );
			if ( twi )
			{
				int iItemIndex = twi->data( 0, Qt::UserRole ).toInt();
				if ( iItemIndex > iIndex )
				{
					twi->setData( 0, Qt::UserRole, iItemIndex-1 );
				}
			}
		}

		// Need to find all later tunnel connectors and decrement indices
		for ( int i=iIndex; i<m_listTunnels.Count(); i++ )
		{
			if ( m_listTunnels.at(i).pConnector )
				m_listTunnels[i].pConnector->m_iTunnelIndex--;
		}
	}
}

void ATSkeletonWindow::slotConnect()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi == NULL ) return;

	int iIndex = twi->data( 0, Qt::UserRole ).toInt();

	connectTunnel( iIndex );
}

void ATSkeletonWindow::slotDisconnect()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi == NULL ) return;

	int iIndex = twi->data( 0, Qt::UserRole ).toInt();
	if ( iIndex < 0 || iIndex >= m_listTunnels.Count() ) return;

	Tunnel_c *pt = &m_listTunnels[iIndex];
	pt->iShouldReconnect = 0;

	disconnectTunnel( iIndex );
}

void ATSkeletonWindow::slotItemDoubleClicked(QTreeWidgetItem *twi)
{
	if ( twi == NULL ) return;

	ui.tabWidget->setCurrentIndex( PAGE_CONNECT );

	int iIndex = twi->data( 0, Qt::UserRole ).toInt();
	if ( iIndex >= 0 && iIndex < m_listTunnels.Count() )
	{
		const Tunnel_c *pt = &m_listTunnels.at(iIndex);

		if (pt->pProcess)
			slotDisconnect();
		else
			slotConnect();
	}
}

void ATSkeletonWindow::slotBrowseKeyFile()
{
	QString strFilename = QFileDialog::getOpenFileName( this, "Please specify the SSH key file to use" );

	if ( !strFilename.isNull() )
	{
		ui.editSSHKeyFile->setText( strFilename );
	}
}

void ATSkeletonWindow::slotConnectorFinished( int iTunnelIndex )
{
	disconnectTunnel( iTunnelIndex );
}

QTreeWidgetItem *ATSkeletonWindow::getTreeItemFromTunnelIndex( int iTunnelIndex )
{
	for ( int i=0; i<ui.treeTunnels->topLevelItemCount(); i++ )
	{
		QTreeWidgetItem *twi = ui.treeTunnels->topLevelItem( i );
		if ( twi->data( 0, Qt::UserRole ).toInt() == iTunnelIndex )
			return twi;
	}

	return NULL;
}

void ATSkeletonWindow::connected( int iTunnelIndex )
{
	QTreeWidgetItem *twi = getTreeItemFromTunnelIndex( iTunnelIndex );
	if ( twi != NULL )
	{
		twi->setIcon( 0, QPixmap( ":connected.png" ) );
		emit signalSetTrayIcon( 1 );
	}

	if ( iTunnelIndex > 0 && iTunnelIndex < m_listTunnels.Count() )
	{
		Tunnel_c *pt = &m_listTunnels[iTunnelIndex];
		pt->iShouldReconnect = pt->bAutoReconnect ? CONNECTION_RETRIES : 0;
	}

	AddToLog( iTunnelIndex, "Connected." );
}

void ATSkeletonWindow::connectTunnel( int iTunnelIndex )
{
	if ( iTunnelIndex < 0 || iTunnelIndex >= m_listTunnels.Count() ) return;

	Tunnel_c *pt = &m_listTunnels[iTunnelIndex];

	AddToLog( iTunnelIndex, "Connecting tunnel %s...", qPrintable( pt->strName ) );
	AddToLog( iTunnelIndex, "Host: %s - Tunnel: %d:%s:%d", qPrintable( pt->strSSHHost ), pt->iLocalPort, qPrintable( pt->strRemoteHost ), pt->iRemotePort );

	if ( pt->strSSHHost.isEmpty() )
	{
		AddToLog( iTunnelIndex, "Error: Tunnel %s has no host, please check the settings.", qPrintable( pt->strName ) );
		return;
	}

#ifdef WIN32
	QString strPlink = "plink.exe";

	// Check that the executable is found
	{
		QDir dir( argv0 );
		dir.cdUp();
		if ( !dir.exists( strPlink ) )
	//	QFile file(strPlink);
	//	if ( !file.exists() )
		{
			const char *ptr = argv0, *ptr2 = argv0;
			while ( *ptr2 ) { if ( *ptr2 == '\\' ) ptr = ptr2+1; ptr2++; }
			AddToLog( iTunnelIndex, "Error: Could not find %s, please check that it is in the same directory as %s.", qPrintable( strPlink ), ptr );
			return;
		}
	}
#else
	QString strPlink = "ssh";
#endif

	if ( pt->pProcess != NULL ) return; // already connected?

	QTreeWidgetItem *twi = getTreeItemFromTunnelIndex( iTunnelIndex );
	if ( twi ) twi->setIcon( 0, QPixmap( ":connecting.png" ) );

	ATASSERT( pt->pConnector == NULL );
	pt->pConnector = new ATTunnelConnector_c( this, iTunnelIndex );

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
	ATVERIFY( connect( pt->pConnector, SIGNAL( finished(int) ), this, SLOT( slotConnectorFinished(int) ), Qt::QueuedConnection ) );

	AddToLog( iTunnelIndex, "%s", qPrintable( strCommand ) );

	pt->pProcess->start( strCommand );
}

void ATSkeletonWindow::disconnectTunnel( int iTunnelIndex )
{
	if ( iTunnelIndex >=0 && iTunnelIndex < m_listTunnels.Count() )
	{
		Tunnel_c *pt = &m_listTunnels[iTunnelIndex];

		if ( pt->pProcess == NULL ) return; // not connected?

		AddToLog( iTunnelIndex, "Disconnecting tunnel %s...", qPrintable( pt->strName ) );
		qApp->processEvents();

		pt->pProcess->kill();
		pt->pProcess->waitForFinished( WAIT_FOR_FINISHED_TIMEOUT );
		delete pt->pProcess;
		pt->pProcess = NULL;

		delete pt->pConnector;
		pt->pConnector = NULL;

		AddToLog( iTunnelIndex, "Disconnected." );

		for ( int i=0; i<ui.treeTunnels->topLevelItemCount(); i++ )
		{
			QTreeWidgetItem *twi = ui.treeTunnels->topLevelItem( i );
			if ( twi->data( 0, Qt::UserRole ).toInt() == iTunnelIndex )
				twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
		}

		int iConnectedCount = 0;
		for ( int i=0; i<m_listTunnels.Count(); i++ )
		{
			if ( m_listTunnels.at(i).pProcess != NULL )
				iConnectedCount++;
		}
		if ( iConnectedCount == 0 )
			emit signalSetTrayIcon( 0 );

		if ( pt->iShouldReconnect > 0 )
		{
			AddToLog( iTunnelIndex, "Connection lost, reconnecting... (%d)", pt->iShouldReconnect );
			pt->iShouldReconnect--;
			emit signalAutoConnect( iTunnelIndex );
		}
		else
		{
			AddToLog( iTunnelIndex, "Connection lost, giving up." );
		}
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

	if ( m_iEditIndex >= 0 && m_iEditIndex < m_listTunnels.Count() )
	{
		pt = &m_listTunnels[m_iEditIndex];
	}
	else
	{
		pt = new Tunnel_c;
		pt->init();
	}
		

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

	if ( m_iEditIndex < 0 || m_iEditIndex >= m_listTunnels.Count() )
	{
		slotAddTunnel();
		m_bDisableChanges = true;
		qApp->processEvents();
	}

	if ( m_iEditIndex >= 0 && m_iEditIndex < m_listTunnels.Count() )
	{
		m_listTunnels[m_iEditIndex] = *pt;

		writeSettings();

		QTreeWidgetItem *twi = getTreeItemFromTunnelIndex( m_iEditIndex );
		if ( twi != NULL ) twi->setText( 0, pt->strName );
		slotSelectTunnel();
		qApp->processEvents();
		m_bDisableChanges = false;
	}
}

void ATSkeletonWindow::slotAddTunnel()
{
	Tunnel_c *pt = m_listTunnels.AppendNewInstance();
	pt->init();

	int iIndex = m_listTunnels.Count() - 1;

	QTreeWidgetItem *twi = new QTreeWidgetItem( ui.treeTunnels );
	twi->setText( 0, pt->strName );
	twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
	twi->setData( 0, Qt::UserRole, iIndex );

	ui.treeTunnels->setCurrentItem( twi );

	ui.tabWidget->setCurrentIndex( PAGE_EDIT );
	ui.editTunnelName->setFocus();
	qApp->processEvents();
	ui.editTunnelName->selectAll();
}

void ATSkeletonWindow::slotDuplicateTunnel()
{
	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi == NULL ) return;

	int iCurrentIndex = twi->data( 0, Qt::UserRole ).toInt();
	if ( iCurrentIndex < 0 || iCurrentIndex >= m_listTunnels.Count() ) return;

	const Tunnel_c *orig = &m_listTunnels.at( iCurrentIndex );

	Tunnel_c *pt = m_listTunnels.AppendNewInstance();
	pt->copyFrom( orig );

	int iIndex = m_listTunnels.Count() - 1;

	twi = new QTreeWidgetItem( ui.treeTunnels );
	twi->setText( 0, pt->strName );
	twi->setIcon( 0, QPixmap( ":disconnected.png" ) );
	twi->setData( 0, Qt::UserRole, iIndex );

	ui.treeTunnels->setCurrentItem( twi );

	ui.tabWidget->setCurrentIndex( PAGE_EDIT );
	ui.editTunnelName->setFocus();
	qApp->processEvents();
	ui.editTunnelName->selectAll();
}

void ATSkeletonWindow::slotSelectTunnel()
{
	if ( detectTunnelChange() ) confirmSaveTunnel();

	m_iEditIndex = -1;

	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi == NULL )
	{
		populateEditUIFromStruct( NULL );
		return;
	}

	int iIndex = twi->data( 0, Qt::UserRole ).toInt();
	if ( iIndex >= 0 && iIndex < m_listTunnels.Count() )
	{
		Tunnel_c *pt = &m_listTunnels[iIndex];

		populateEditUIFromStruct( pt );

		m_iEditIndex = iIndex;
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

	if ( m_iEditIndex < 0 || m_iEditIndex >= m_listTunnels.Count() ) return false;
	const Tunnel_c *pt = &m_listTunnels.at( m_iEditIndex );

	if ( ui.editTunnelName->text() != pt->strName ) return true;
	if ( ui.editSSHHost->text() != pt->strSSHHost ) return true;
	if ( ui.editLocalPort->text().toInt() != pt->iLocalPort ) return true;
	if ( ui.editRemoteHost->text() != pt->strRemoteHost ) return true;
	if ( ui.editRemotePort->text().toInt() != pt->iRemotePort ) return true;
	if ( ui.editExtraArguments->text() != pt->strExtraArguments ) return true;
	if ( ui.editUsername->text() != pt->strUsername ) return true;
	if ( ui.editSSHKeyFile->text() != pt->strSSHKeyFile ) return true;
	if ( ui.comboDirection->currentIndex() != pt->iDirection ) return true;
	if ( ui.checkAutoConnect->isChecked() != pt->bAutoConnect ) return true;
	if ( ui.checkCompression->isChecked() != pt->bCompression ) return true;
	if ( ui.checkDoKeepAlivePing->isChecked() != pt->bDoKeepAlivePing ) return true;
	if ( ui.checkAutoReconnect->isChecked() != pt->bAutoReconnect ) return true;
	if ( ui.radioSSH1->isChecked() != ( pt->iSSH1or2 == 1 ) ) return true;
	if ( ui.radioSSH2->isChecked() != ( pt->iSSH1or2 != 1 ) ) return true;

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

bool ATSkeletonWindow::askforPassword( int iTunnelIndex )
{
	if ( iTunnelIndex < 0 || iTunnelIndex >= m_listTunnels.Count() ) return INVALID_PASSWORD;

	Tunnel_c *pt = &m_listTunnels[iTunnelIndex];

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

void ATSkeletonWindow::onClose()
{
	if ( detectTunnelChange() )
	{
		confirmSaveTunnel();
	}
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

void ATSkeletonWindow::AddToLog( int iTunnelIndex, const char *format, ... )
{
	if ( iTunnelIndex < 0 || iTunnelIndex >= m_listTunnels.Count() ) return;

	char buffer[8096];

	va_list argptr;
	va_start(argptr, format);

	safe_vsnprintf(buffer, sizeof(buffer), format, argptr);

	va_end(argptr);

	QString str( buffer );
	str.replace( '\n', "<br>" );

	Tunnel_c *pt = &m_listTunnels[iTunnelIndex];
	pt->byteLog.append( str );
	pt->byteLog.append( "<br>" );

	QTreeWidgetItem *twi = ui.treeTunnels->currentItem();
	if ( twi != NULL ) 
		if ( twi->data( 0, Qt::UserRole ).toInt() != iTunnelIndex )
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

void ATSkeletonWindow::slotAutoConnect( int iTunnelIndex )
{
	// If it's a valid index, auto connect it, otherwise auto connect all that were ticked in the settings.
	if ( iTunnelIndex >= 0 && iTunnelIndex < m_listTunnels.Count() )
	{
		connectTunnel( iTunnelIndex );

		return;
	}

	for ( int i=0; i<m_listTunnels.Count(); i++ )
	{
		const Tunnel_c *pt = &m_listTunnels.at(i);
		if ( pt->bAutoConnect )
		{
			connectTunnel( i );
		}
	}
}




// Tunnel_c

Tunnel_c::Tunnel_c()
{
	pProcess = NULL;
	pConnector = NULL;
	strPassword = INVALID_PASSWORD;
	iShouldReconnect = 0;
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

ATTunnelConnector_c::ATTunnelConnector_c( ATSkeletonWindow *pParent, int iTunnelIndex ) : QObject(),
m_pParent( pParent ),
m_iTunnelIndex( iTunnelIndex )
{
}

#ifndef _WIN32
#define strtok_s(a,b,c) strtok(a,b)
#endif

void ATTunnelConnector_c::slotProcessReadStandardOutput()
{
	ATASSERT( m_pParent );
	ATASSERT( m_iTunnelIndex >= 0 && m_iTunnelIndex < m_pParent->m_listTunnels.Count() );

	Tunnel_c *pt = &m_pParent->m_listTunnels[m_iTunnelIndex];
	ATASSERT( pt );

	QByteArray b = pt->pProcess->readAllStandardOutput();

	char *ptrb = b.data();
	char *context;
	context = NULL;
	char *ptr = strtok_s( ptrb, "\r\n", &context );

	while ( ptr )
	{
		m_pParent->AddToLog( m_iTunnelIndex, "1> %s", ptr );
		processPlinkOutput( ptr );
		ptr = strtok_s( NULL, "\r\n", &context );
	}

//	m_pParent->AddToLog( m_iTunnelIndex, b );
}

void ATTunnelConnector_c::slotProcessReadStandardError()
{
	ATASSERT( m_pParent );
	ATASSERT( m_iTunnelIndex >= 0 && m_iTunnelIndex < m_pParent->m_listTunnels.Count() );

	Tunnel_c *pt = &m_pParent->m_listTunnels[m_iTunnelIndex];
	ATASSERT( pt );

	QByteArray b = pt->pProcess->readAllStandardError();

	char *ptrb = b.data();
	char *context;
	context = NULL;
	char *ptr = strtok_s( ptrb, "\r\n", &context );

	while ( ptr )
	{
		m_pParent->AddToLog( m_iTunnelIndex, "2> %s", ptr );
		processPlinkOutput( ptr );
		ptr = strtok_s( NULL, "\r\n", &context );
	}

	//m_pParent->AddToLog( m_iTunnelIndex, b );
}

void ATTunnelConnector_c::slotProcessError(QProcess::ProcessError error)
{
	ATASSERT( m_pParent );
	ATASSERT( m_iTunnelIndex >= 0 && m_iTunnelIndex < m_pParent->m_listTunnels.Count() );

	Tunnel_c *pt = &m_pParent->m_listTunnels[m_iTunnelIndex];
	ATASSERT( pt );

	m_pParent->AddToLog( m_iTunnelIndex, "Process error: %d", error );
	
	emit finished( m_iTunnelIndex );
}

void ATTunnelConnector_c::slotProcessFinished(int exitCode, QProcess::ExitStatus /*exitStatus*/)
{
	ATASSERT( m_pParent );
	ATASSERT( m_iTunnelIndex >= 0 && m_iTunnelIndex < m_pParent->m_listTunnels.Count() );

	Tunnel_c *pt = &m_pParent->m_listTunnels[m_iTunnelIndex];
	ATASSERT( pt );

	m_pParent->AddToLog( m_iTunnelIndex, "Process exit: %d", exitCode );

	emit finished( m_iTunnelIndex );
}

void ATTunnelConnector_c::processPlinkOutput( const char *str )
{
	if ( !m_pParent ) return;

	if ( strstr( str, "assword:" ) )
	{
		if ( m_iTunnelIndex >= 0 && m_iTunnelIndex <= m_pParent->m_listTunnels.Count() )
		{
			const Tunnel_c *pt = &m_pParent->m_listTunnels.at( m_iTunnelIndex );
			if ( !m_pParent->askforPassword( m_iTunnelIndex ) )
			{
				emit finished( m_iTunnelIndex );
			}
			else if ( pt->pProcess )
			{
				QString strStarPassword( pt->strPassword );
				for ( int i=0; i<strStarPassword.count(); i++ )
					strStarPassword[i] = '*';
				m_pParent->AddToLog( m_iTunnelIndex, "Sent password: %s", qPrintable( strStarPassword ) );
				pt->pProcess->write( qPrintable( pt->strPassword + "\r\n" ) );
			}
		}
	}
	else if ( strstr( str, "Store key in cache? (y/n)" ) )
	{
		if ( m_iTunnelIndex >= 0 && m_iTunnelIndex <= m_pParent->m_listTunnels.Count() )
		{
			const Tunnel_c *pt = &m_pParent->m_listTunnels.at( m_iTunnelIndex );
			if ( pt->pProcess )
			{
				pt->pProcess->write( "n\n" );
			}
		}
	}
	else if ( strstr( str, "Access granted" ) )
	{
		m_pParent->connected( m_iTunnelIndex );
	}
	else if ( strstr( str, "Authentication succeeded" ) )
	{
		m_pParent->connected( m_iTunnelIndex );
	}
	else if ( strstr( str, "Access denied" ) )
	{
		if ( m_iTunnelIndex >= 0 && m_iTunnelIndex <= m_pParent->m_listTunnels.Count() )
		{
			Tunnel_c *pt = &m_pParent->m_listTunnels[m_iTunnelIndex];
			pt->strPassword = INVALID_PASSWORD;
		}
	}
}

