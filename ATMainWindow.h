#ifndef AT_MAINWINDOW_C
#define AT_MAINWINDOW_C

#include "pch.h"

class QAction;
class ATSkeletonWindow;

class ATMainWindow_c : public QMainWindow
{
	Q_OBJECT

public:
	ATMainWindow_c( QWidget *vpParent = NULL );
	~ATMainWindow_c();

	bool InitMenusAndActions();

signals:

public slots:
	void slotChangeStyle( QString );
	void slotShowAbout();
	void iconActivated(QSystemTrayIcon::ActivationReason reason);
	void slotSetTrayIcon(int iIndex);
	void slotReadOptions();
	void slotTentativeQuit();

protected:
	void closeEvent ( QCloseEvent * event );

private:
	IF_WIN32( bool winEvent( MSG *m, long *result ) );
	void HotKey( unsigned short lo, unsigned short hi );

	ATSkeletonWindow *m_pMainWindow;
	QString m_strStyle;

	QSystemTrayIcon *m_trayIcon;
	QMenu *m_trayIconMenu;

	void createTrayIcon();

	void readSettings();
	void writeSettings();

	QTime m_lastTrayTrigger;

	QIcon m_iconConnected;
	QIcon m_iconConnecting;
	QIcon m_iconDisconnected;

	void updateOptions();

	void DoRegisterHotKey();
	QString m_strHotkeyMod1;
	QString m_strHotkeyMod2;
	QString m_strHotkeyKey;
	bool m_bHotkeyEnabled;

	bool m_bMinimizeToTray;
	void hideWindow();
};

#endif
