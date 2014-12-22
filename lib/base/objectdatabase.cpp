/******************************************************************************
* Icinga 2                                                                   *
* Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/objectdatabase.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include "base/dynamictype.hpp"
#include "base/workqueue.hpp"
#include "base/application.hpp"
#include "base/stdiostream.hpp"
#include "base/netstring.hpp"
#include <boost/foreach.hpp>
#include <fstream>

using namespace icinga;

ObjectDatabase::ObjectDatabase(const String& filename)
{
	m_Database = SqliteDatabase::Open(filename);

	m_Database->Exec("PRAGMA journal_mode = MEMORY");

	try {
		m_Database->Exec(
		    "CREATE TABLE `objects` ("
		    "    `type`	TEXT NOT NULL,"
		    "    `name`	TEXT NOT NULL,"
		    "    `config`	BLOB,"
		    "    `state`	BLOB,"
		    "    `debug_hint`	BLOB,"
		    "    `updated`	INTEGER NOT NULL,"
		    "    `active`	INTEGER DEFAULT 1 NOT NULL"
		    "    );"
		);
	} catch (const SqliteError&) {
		/* ignore */
	}

	try {
		m_Database->Exec("CREATE UNIQUE INDEX 'idx_type_name' ON 'objects' ('type', 'name')");
	} catch (const SqliteError&) {
		/* ignore */
	}

	try {
		m_Database->Exec("CREATE INDEX 'idx_active_updated' ON 'objects' ('active', 'updated')");
	} catch (const SqliteError&) {
		/* ignore */
	}

	m_UpdateStateStmt = m_Database->PrepareStatement("UPDATE objects SET state = ?, updated = ? WHERE type = ? AND name = ?");
	m_UpdateConfigStmt = m_Database->PrepareStatement("UPDATE objects SET config = ?, debug_hint = ?, updated = ?, active=1 WHERE type = ? AND name = ?");
	m_InsertConfigStmt = m_Database->PrepareStatement("INSERT OR IGNORE INTO objects (type, name, config, debug_hint, updated) VALUES (?, ?, ?, ?, ?)");
	m_SelectOneStateStmt = m_Database->PrepareStatement("SELECT state FROM objects WHERE type = ? AND name = ?");
	m_SelectAllStateStmt = m_Database->PrepareStatement("SELECT type, name, state FROM objects WHERE active = 1");
}

void ObjectDatabase::BeginTransaction(void)
{
	m_Database->BeginTransaction();
}

void ObjectDatabase::CommitTransaction(void)
{
	m_Database->CommitTransaction();
}
void ObjectDatabase::RollbackTransaction(void)
{
	m_Database->RollbackTransaction();
}

void ObjectDatabase::SaveObjectState(const DynamicObject::Ptr& object)
{
	Dictionary::Ptr update = Serialize(object, FAState);

	if (!update)
		return;

	String json = JsonEncode(update);

	String type = object->GetReflectionType()->GetName();
	String name = object->GetName();

	InternalSaveObjectState(type, name, json);
}

void ObjectDatabase::InternalSaveObjectState(const String& type, const String& name, const String& attrsJson)
{
	m_UpdateStateStmt->Reset();
	m_UpdateStateStmt->Bind(1, attrsJson);
	m_UpdateStateStmt->Bind(2, Utility::GetTime());
	m_UpdateStateStmt->Bind(3, type);
	m_UpdateStateStmt->Bind(4, name);
	m_UpdateStateStmt->Exec();
}

void ObjectDatabase::RestoreObjectState(const DynamicObject::Ptr& object)
{
	ASSERT(!object->GetStateLoaded());
		
	String type = object->GetReflectionType()->GetName();
	String name = object->GetName();

	m_SelectOneStateStmt->Reset();
	m_SelectOneStateStmt->Bind(1, type);
	m_SelectOneStateStmt->Bind(2, name);
	BOOST_FOREACH(const SqliteRow& row, m_SelectOneStateStmt->Exec()) {
		if (row[2].IsEmpty())
			continue;

		InternalRestoreObjectState(row[0], row[1], row[2]);
	}

	if (!object->GetStateLoaded()) {
		object->OnStateLoaded();
		object->SetStateLoaded(true);
	}
}

void ObjectDatabase::SaveObjectsState(void)
{
	Log(LogInformation, "ObjectsDatabase", "Dumping program state to objects database");

	BeginTransaction();

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			SaveObjectState(object);
		}
	}

	CommitTransaction();
}

void ObjectDatabase::RestoreObjectsState(void)
{
	unsigned long restored = 0;

	Log(LogInformation, "ObjectsDatabase", "Restoring program state");

	m_SelectAllStateStmt->Reset();
	BOOST_FOREACH(const SqliteRow& row, m_SelectAllStateStmt->Exec()) {
		if (row[2].IsEmpty())
			continue;

		InternalRestoreObjectState(row[0], row[1], row[2]);
		restored++;
	}

	unsigned long no_state = 0;

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes())
	{
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects())
		{
			if (!object->GetStateLoaded()) {
				object->OnStateLoaded();
				object->SetStateLoaded(true);

				no_state++;
			}
		}
	}

	Log(LogInformation, "ObjectsDatabase")
		<< "Restored " << restored << " objects. Loaded " << no_state << " new objects without state.";
}

void ObjectDatabase::InternalRestoreObjectState(const String& type, const String& name, const String& stateJson)
{
	Dictionary::Ptr state = JsonDecode(stateJson);

	DynamicType::Ptr dt = DynamicType::GetByName(type);

	if (!dt)
		return;

	DynamicObject::Ptr object = dt->GetObject(name);

	if (!object || object->GetStateLoaded())
		return;

	ASSERT(!object->IsActive());
#ifdef I2_DEBUG
	Log(LogDebug, "ObjectsDatabase")
		<< "Restoring object '" << name << "' of type '" << type << "'.";
#endif /* I2_DEBUG */
	Deserialize(object, state, false, FAState);
	object->OnStateLoaded();
	object->SetStateLoaded(true);
}

void ObjectDatabase::SaveObjectConfig(const DynamicObject::Ptr& object, const Dictionary::Ptr& dhint)
{
	Dictionary::Ptr config = Serialize(object, FAConfig);
	String jsonConfig = JsonEncode(config);
	String jsonDHint = JsonEncode(dhint);

	String type = object->GetReflectionType()->GetName();
	String name = object->GetName();

	m_UpdateConfigStmt->Reset();
	m_UpdateConfigStmt->Bind(1, jsonConfig);
	m_UpdateConfigStmt->Bind(2, jsonDHint);
	m_UpdateConfigStmt->Bind(3, Utility::GetTime());
	m_UpdateConfigStmt->Bind(4, type);
	m_UpdateConfigStmt->Bind(5, name);
	m_UpdateConfigStmt->Exec();

	if (m_Database->GetChanges() > 0)
		return;

	m_InsertConfigStmt->Reset();
	m_InsertConfigStmt->Bind(1, type);
	m_InsertConfigStmt->Bind(2, name);
	m_InsertConfigStmt->Bind(3, jsonConfig);
	m_InsertConfigStmt->Bind(4, jsonDHint);
	m_InsertConfigStmt->Bind(5, Utility::GetTime());
	m_InsertConfigStmt->Exec();
}

void ObjectDatabase::DeactivateAllObjects(void)
{
	m_Database->Exec("UPDATE objects SET active=0");
}

void ObjectDatabase::Cleanup(void)
{
	m_Database->Exec("DELETE FROM objects WHERE active = 0 AND updated < " + Convert::ToString(Utility::GetTime()));
}

void ObjectDatabase::ImportStateFile(const String& filename)
{
	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream(&fp, false);

	String message;
	while (NetString::ReadStringFromStream(sfp, &message)) {
		Dictionary::Ptr dict = JsonDecode(message);
		String type = dict->Get("type");
		String name = dict->Get("name");
		Dictionary::Ptr update = dict->Get("update");
		String jsonAttrs = JsonEncode(update);
		InternalSaveObjectState(type, name, jsonAttrs);
	}

	sfp->Close();
}
