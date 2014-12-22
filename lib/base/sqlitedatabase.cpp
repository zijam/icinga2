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

#include "base/sqlitedatabase.hpp"

using namespace icinga;

SqliteDatabase::SqliteDatabase(const String& filename, SqliteDatabase::FactoryKey)
{
	if (sqlite3_open(filename.CStr(), &m_Database) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError("Could not open database"));

	sqlite3_busy_timeout(m_Database, 15000);
}

SqliteDatabase::~SqliteDatabase(void)
{
	if (sqlite3_close(m_Database) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Database)));
}

boost::shared_ptr<SqliteDatabase> SqliteDatabase::Open(const String& filename)
{
	return boost::make_shared<SqliteDatabase>(filename);
}

void SqliteDatabase::BeginTransaction(void)
{
	if (sqlite3_exec(m_Database, "BEGIN", NULL, NULL, NULL) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Database)));
}

void SqliteDatabase::CommitTransaction(void)
{
	if (sqlite3_exec(m_Database, "COMMIT", NULL, NULL, NULL) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Database)));
}
void SqliteDatabase::RollbackTransaction(void)
{
	if (sqlite3_exec(m_Database, "ROLLBACK", NULL, NULL, NULL) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Database)));
}

std::pair<SqliteRowIterator, SqliteRowIterator> SqliteDatabase::Exec(const String& stmt)
{
	boost::shared_ptr<SqliteStatement> sstmt = PrepareStatement(stmt);
	return sstmt->Exec();
}

boost::shared_ptr<SqliteStatement> SqliteDatabase::PrepareStatement(const String& stmt)
{
	return boost::make_shared<SqliteStatement>(shared_from_this(), stmt);
}

int SqliteDatabase::GetChanges(void)
{
	return sqlite3_changes(m_Database);
}

sqlite3 *SqliteDatabase::GetHandle(void) const
{
	return m_Database;
}

SqliteCursor::SqliteCursor(const boost::shared_ptr<SqliteStatement>& stmt)
	: m_Statement(stmt), m_EndReached(false)
{ }

SqliteCursor::~SqliteCursor(void)
{
	/* drain remaining rows */
	while (!m_EndReached)
		FetchRow(NULL);
}

bool SqliteCursor::FetchRow(SqliteRow *row)
{
	ASSERT(!m_EndReached);

	int rc = sqlite3_step(m_Statement->GetHandle());

	if (rc == SQLITE_DONE) {
		m_EndReached = true;
		return false;
	}

	if (rc != SQLITE_ROW) {
		m_EndReached = true;
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Statement->GetDatabase()->GetHandle())));
	}

	if (row) {
		*row = SqliteRow();

		for (int col = 0; col < sqlite3_column_count(m_Statement->GetHandle()); col++) {
			int type = sqlite3_column_type(m_Statement->GetHandle(), col);
			int size;
			const void *data;
			switch (type) {
				case SQLITE_INTEGER:
				case SQLITE_FLOAT:
					row->push_back(sqlite3_column_double(m_Statement->GetHandle(), col));
				case SQLITE_NULL:
					row->push_back(Empty);
				case SQLITE_BLOB:
				case SQLITE3_TEXT:
				default:
					size = sqlite3_column_bytes(m_Statement->GetHandle(), col);
					data = sqlite3_column_blob(m_Statement->GetHandle(), col);
					row->push_back(String(reinterpret_cast<const char *>(data), reinterpret_cast<const char*>(data) + size));
			}
		}
		/* TODO: Build result row */
	}

	return true;
}

SqliteStatement::SqliteStatement(const boost::shared_ptr<SqliteDatabase>& database, const String& stmt)
	: m_Database(database)
{
	if (sqlite3_prepare_v2(m_Database->GetHandle(), stmt.CStr(), -1, &m_Statement, NULL) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Database->GetHandle())));
}

SqliteStatement::~SqliteStatement(void)
{
	sqlite3_finalize(m_Statement);
}

void SqliteStatement::Reset(void)
{
	if (sqlite3_reset(m_Statement) != SQLITE_OK)
		BOOST_THROW_EXCEPTION(SqliteError(sqlite3_errmsg(m_Database->GetHandle())));
}

void SqliteStatement::Bind(int arg, const Value& value)
{
	String str;

	switch (value.GetType()) {
		case ValueEmpty:
			sqlite3_bind_null(m_Statement, arg);
			break;
		case ValueNumber:
			sqlite3_bind_double(m_Statement, arg, static_cast<double>(value));
			break;
		case ValueBoolean:
			sqlite3_bind_int(m_Statement, arg, static_cast<bool>(value));
			break;
		case ValueString:
			str = static_cast<String>(value);
			sqlite3_bind_blob(m_Statement, arg, str.CStr(), str.GetLength(), SQLITE_TRANSIENT);
			break;
		default:
			ASSERT(!"Invalid variant.");
	}
}

std::pair<SqliteRowIterator, SqliteRowIterator> SqliteStatement::Exec(void)
{
	boost::shared_ptr<SqliteCursor> cursor = boost::make_shared<SqliteCursor>(shared_from_this());

	return std::make_pair(SqliteRowIterator(cursor), SqliteRowIterator());
}

boost::shared_ptr<SqliteDatabase> SqliteStatement::GetDatabase(void) const
{
	return m_Database;
}

sqlite3_stmt *SqliteStatement::GetHandle(void) const
{
	return m_Statement;
}

SqliteError::SqliteError(const String& message)
	: m_Message(message)
{ }

SqliteError::~SqliteError(void) throw()
{ }

const char *SqliteError::what(void) const throw()
{
	return m_Message.CStr();
}
