/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "db_ido/endpointdbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "icinga/icingaapplication.h"
#include "base/objectlock.h"
#include "base/initialize.h"
#include "base/dynamictype.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include <boost/foreach.hpp>

using namespace icinga;


REGISTER_DBTYPE(Endpoint, "endpoint", DbObjectTypeEndpoint, "endpoint_object_id", EndpointDbObject);

INITIALIZE_ONCE(&EndpointDbObject::StaticInitialize);

void EndpointDbObject::StaticInitialize(void)
{
	Endpoint::OnConnected.connect(boost::bind(&EndpointDbObject::UpdateConnectedStatus, _1));
	Endpoint::OnDisconnected.connect(boost::bind(&EndpointDbObject::UpdateConnectedStatus, _1));
}

EndpointDbObject::EndpointDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr EndpointDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = make_shared<Dictionary>();
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(GetObject());

	fields->Set("identity", endpoint->GetName());
	fields->Set("node", IcingaApplication::GetInstance()->GetNodeName());

	return fields;
}

Dictionary::Ptr EndpointDbObject::GetStatusFields(void) const
{
	Dictionary::Ptr fields = make_shared<Dictionary>();
	Endpoint::Ptr endpoint = static_pointer_cast<Endpoint>(GetObject());

	fields->Set("identity", endpoint->GetName());
	fields->Set("node", IcingaApplication::GetInstance()->GetNodeName());
	fields->Set("is_connected", endpoint->IsConnected() ? 1 : 0);

	return fields;
}

void EndpointDbObject::UpdateConnectedStatus(const Endpoint::Ptr& endpoint)
{
	Log(LogDebug, "db_ido", "update is_connected for endpoint '" + endpoint->GetName() + "'");

	DbQuery query1;
	query1.Table = "endpointstatus";
	query1.Type = DbQueryUpdate;

	Dictionary::Ptr fields1 = make_shared<Dictionary>();
	fields1->Set("is_connected", endpoint->IsConnected() ? 1 : 0);
	fields1->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query1.Fields = fields1;

	query1.WhereCriteria = make_shared<Dictionary>();
	query1.WhereCriteria->Set("endpoint_object_id", endpoint);
	query1.WhereCriteria->Set("instance_id", 0); /* DbConnection class fills in real ID */

	OnQuery(query1);
}
