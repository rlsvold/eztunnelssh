#ifndef ATSKELETONWINDOW_H
#define ATSKELETONWINDOW_H

#include "pch.h"
#include "List.h"
#include "ui_ATSkeleton.h"

class ATSkeletonWindow;

class ATTunnelConnector_c : public QObject
{
	Q_OBJECT

public:
	ATTunnelConnector_c( ATSkeletonWindow *pParent, int iTunnelIndex );

	ATSkeletonWindow *m_pParent;
	int m_iTunnelIndex;

public slots:
	void slotProcessReadStandardOutput();
	void slotProcessReadStandardError();
	void slotProcessError(QProcess::ProcessError error);
	void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

signals:
	void finished(int iTunnelIndex);

protected:
	void processPlinkOutput( const char *str );
};

class Tunnel_c
{
public:
	Tunnel_c();

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
};

class ATSkeletonWindow : public QWidget
{
	Q_OBJECT

public:
	ATSkeletonWindow(QWidget *parent = 0);
	~ATSkeletonWindow();

	void onClose();

	void AddToLog( int iTunnelIndex, const char *format, ... );

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
	void slotConnectorFinished( int iTunnelIndex );
	void slotAutoConnect( int iIndex );
	void slotBrowseKeyFile();

signals:
	void signalSetTrayIcon( int iIndex );
	void signalAutoConnect( int iIndex );

private:
	Ui::ATSkeletonWindowClass ui;

	void wireSignals();
	void readSettings();
	void writeSettings();
	void updateControls();

	void connectTunnel( int iTunnelIndex );
	void disconnectTunnel( int iTunnelIndex );
	void connected( int iTunnelIndex );

	void saveTunnel();

	bool m_bDisableChanges;
	bool detectTunnelChange();
	void confirmSaveTunnel();

	bool askforPassword( int iTunnelIndex );

	void populateEditUIFromStruct( Tunnel_c *t );

	int m_iEditIndex;

	QTreeWidgetItem *getTreeItemFromTunnelIndex( int iTunnelIndex );

	List_c<Tunnel_c> m_listTunnels;

	friend class ATTunnelConnector_c;
};

#endif 
