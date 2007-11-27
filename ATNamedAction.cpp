
#include "ATNamedAction.h"

ATNamedAction::ATNamedAction( const QString &strName, QObject * parent )
	: QAction( strName, parent )
{
	m_strName = strName;

	bool bRet = connect(this, SIGNAL( triggered() ), this, SLOT( slotCustomTrigger() ) );
	ATASSERT( bRet );
}

ATNamedAction::~ATNamedAction()
{
}

void ATNamedAction::slotCustomTrigger()
{
	ATDEBUG( __FUNCTION__ );

	emit signalTriggerNamed( m_strName );
}


