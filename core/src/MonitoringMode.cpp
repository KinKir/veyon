/*
 * MonitoringMode.cpp - implementation of MonitoringMode class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QtConcurrent>

#include "MonitoringMode.h"
#include "PlatformUserFunctions.h"
#include "VeyonServerInterface.h"


MonitoringMode::MonitoringMode( QObject* parent ) :
	QObject( parent ),
	m_monitoringModeFeature( QLatin1String( staticMetaObject.className() ),
							 Feature::Mode | Feature::Master | Feature::Builtin,
							 Feature::Uid( "edad8259-b4ef-4ca5-90e6-f238d0fda694" ),
							 Feature::Uid(),
							 tr( "Monitoring" ), tr( "Monitoring" ),
							 tr( "This mode allows you to monitor all computers at one or more locations." ),
							 QStringLiteral( ":/core/presentation-none.png" ) ),
	m_queryLoggedOnUserInfoFeature( QStringLiteral("UserSessionInfo"),
									Feature::Session | Feature::Service | Feature::Worker | Feature::Builtin,
									Feature::Uid( "79a5e74d-50bd-4aab-8012-0e70dc08cc72" ),
									Feature::Uid(), {}, {}, {} ),
	m_features( { m_monitoringModeFeature, m_queryLoggedOnUserInfoFeature } )
{
}



bool MonitoringMode::queryLoggedOnUserInfo( const ComputerControlInterfaceList& computerControlInterfaces )
{
	return sendFeatureMessage( FeatureMessage( m_queryLoggedOnUserInfoFeature.uid(), FeatureMessage::DefaultCommand ),
							   computerControlInterfaces, false );
}



bool MonitoringMode::handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
										   ComputerControlInterface::Pointer computerControlInterface )
{
	Q_UNUSED(master)

	if( message.featureUid() == m_queryLoggedOnUserInfoFeature.uid() )
	{
		computerControlInterface->setUserLoginName( message.argument( UserLoginName ).toString() );
		computerControlInterface->setUserFullName( message.argument( UserFullName ).toString() );

		return true;
	}

	return false;
}



bool MonitoringMode::handleFeatureMessage( VeyonServerInterface& server,
										   const MessageContext& messageContext,
										   const FeatureMessage& message )
{
	if( m_queryLoggedOnUserInfoFeature.uid() == message.featureUid() )
	{
		FeatureMessage reply( message.featureUid(), message.command() );

		m_userDataLock.lockForRead();
		if( m_userLoginName.isEmpty() )
		{
			queryUserInformation();
			reply.addArgument( UserLoginName, QString() );
			reply.addArgument( UserFullName, QString() );
		}
		else
		{
			reply.addArgument( UserLoginName, m_userLoginName );
			reply.addArgument( UserFullName, m_userFullName );
		}
		m_userDataLock.unlock();

		return server.sendFeatureMessageReply( messageContext, reply );
	}

	return false;
}



void MonitoringMode::queryUserInformation()
{
	// asynchronously query information about logged on user (which might block
	// due to domain controller queries and timeouts etc.)
	QtConcurrent::run( [=]() {
		const auto userLoginName = VeyonCore::platform().userFunctions().currentUser();
		const auto userFullName = VeyonCore::platform().userFunctions().fullName( userLoginName );
		m_userDataLock.lockForWrite();
		m_userLoginName = userLoginName;
		m_userFullName = userFullName;
		m_userDataLock.unlock();
	} );
}
