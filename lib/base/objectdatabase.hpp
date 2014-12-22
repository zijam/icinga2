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

#ifndef OBJECTDATABASE_H
#define OBJECTDATABASE_H

#include "base/sqlitedatabase.hpp"
#include "base/dynamicobject.hpp"

namespace icinga
{

/**
 * An objects database.
 */
class I2_BASE_API ObjectDatabase
{
public:
	ObjectDatabase(const String& filename);

	void BeginTransaction(void);
	void CommitTransaction(void);
	void RollbackTransaction(void);

	void DeactivateAllObjects(void);

	void SaveObjectState(const DynamicObject::Ptr& object);
	void RestoreObjectState(const DynamicObject::Ptr& object);

	void SaveObjectConfig(const DynamicObject::Ptr& object, const Dictionary::Ptr& dhint);

	void SaveObjectsState(void);
	void RestoreObjectsState(void);

	void Cleanup(void);

	void ImportStateFile(const String& filename);

private:
	boost::shared_ptr<SqliteDatabase> m_Database;
	boost::shared_ptr<SqliteStatement> m_UpdateStateStmt;
	boost::shared_ptr<SqliteStatement> m_UpdateConfigStmt;
	boost::shared_ptr<SqliteStatement> m_InsertConfigStmt;
	boost::shared_ptr<SqliteStatement> m_SelectOneStateStmt;
	boost::shared_ptr<SqliteStatement> m_SelectAllStateStmt;

	void InternalSaveObjectState(const String& type, const String& name, const String& attrsJson);
	static void InternalRestoreObjectState(const String& type, const String& name, const String& attrsJson);
};

}

#endif /* OBJECTDATABASE_H */
