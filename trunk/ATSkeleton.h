#ifndef ATSKELETONWINDOW_H
#define ATSKELETONWINDOW_H

#include "pch.h"
#include "ui_ATSkeleton.h"

class ATSkeletonWindow;
class Tunnel_c;

class ATTunnelConnector_c : public QObject
{
	Q_OBJECT

public:
	ATTunnelConnector_c( ATSkeletonWindow *pParent, Tunnel_c *pTunnel );

	ATSkeletonWindow *m_pParent;
	Tunnel_c *m_pTunnel;

public slots:
	void slotProcessReadStandardOutput();
	void slotProcessReadStandardError();
	void slotProcessError(QProcess::ProcessError error);
	void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

signals:
	void finished( Tunnel_c * );

protected:
	void processPlinkOutput( const char *str );
};

class Tunnel_c
{
public:
	Tunnel_c();
	~Tunnel_c();
	// Use default copy contructor and assignment operator

	void init();
	void copyFrom( const Tunnel_c *orig );

	QString strName;
	QString strSSHHost;
	int     iLocalPort;
	QString strRemoteHost;
	int     iRemotePort;
	QString strUsername;
	QString strSSHKeyFile;
	int     iDirection;
	bool    bAutoConnect;
	bool    bCompression;
	bool    bDoKeepAlivePing;
	bool    bAutoReconnect;
	int     iSSH1or2;
	QString strExtraArguments;

	QProcess *pProcess;
	QByteArray byteLog;
	ATTunnelConnector_c *pConnector;
	QString strPassword;
	int iShouldReconnect;
	QTreeWidgetItem *twi;
};

class ATSkeletonWindow : public QWidget
{
	Q_OBJECT;
	typedef std::list<Tunnel_c>::iterator TunnelInterator;

public:
	ATSkeletonWindow(QWidget *parent = 0);
	~ATSkeletonWindow();

	bool onClose();

	void AddToLog( Tunnel_c &tunnel, const char *format, ... );

public slots:
	void slotAddTunnel();
	void slotEditTunnel();
	void slotDuplicateTunnel();
	void slotDeleteTunnel();
	void slotSelectTunnel();
	void slotSave();
	void slotTabChanged();
	void slotConnect();
	void slotDisconnect();
	void slotConnectorFinished( Tunnel_c* );
	void slotAutoConnect( Tunnel_c* );
	void slotBrowseKeyFile();
	void slotItemActivated();
	void slotDelayReadOptions();
	void slotReadOptions();

signals:
	void signalSetTrayIcon( int iIndex );
	void signalAutoConnect( Tunnel_c* );
	void signalReadOptions();

private:
	Ui::ATSkeletonWindowClass ui;

	void wireSignals();
	void readSettings();
	void writeSettings();
	void updateControls();

	void connectTunnel( Tunnel_c &tunnel );
	void disconnectTunnel( Tunnel_c &tunnel );
	void connected( Tunnel_c &tunnel );

	void saveTunnel();

	bool m_bDisableChanges;
	bool detectTunnelChange();
	void confirmSaveTunnel();

	bool askforPassword( Tunnel_c &tunnel );

	void populateEditUIFromStruct( Tunnel_c *t );

	Tunnel_c *m_pTunnelEdit;

	Tunnel_c *getTunnelFromTreeItem( const QTreeWidgetItem *twi );

	std::list<Tunnel_c> m_listTunnels;

	QTimer m_timerReadOptions;

	friend class ATTunnelConnector_c;
	friend class ATMainWindow_c;
};

#endif 
