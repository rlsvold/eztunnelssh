#ifndef __AT_NAMED_ACTION_H__
#define __AT_NAMED_ACTION_H__

#include "pch.h"

class ATNamedAction : public QAction
{
	Q_OBJECT

public:
	ATNamedAction( const QString &strName, QObject * parent );
	~ATNamedAction();

public slots:
	void slotCustomTrigger();

signals:
	void signalTriggerNamed( QString );

private:
	QString m_strName;
};

#endif
