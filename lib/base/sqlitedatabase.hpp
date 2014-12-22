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

#ifndef SQLITEDATABASE_H
#define SQLITEDATABASE_H

#include "base/dynamicobject.hpp"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>

#ifdef _WIN32
#define SQLITE_API I2_IMPORT
#endif /* _WIN32 */

#include <sqlite3.h>

namespace icinga
{

class I2_BASE_API SqliteError : virtual public std::exception, virtual public boost::exception
{
public:
	SqliteError(const String& message);
	~SqliteError(void) throw();

	virtual const char *what(void) const throw();

private:
	String m_Message;
};

class SqliteDatabase;
class SqliteStatement;

typedef std::vector<Value> SqliteRow;

class I2_BASE_API SqliteCursor
{
public:
	SqliteCursor(const boost::shared_ptr<SqliteStatement>& stmt);
	~SqliteCursor(void);

	bool FetchRow(SqliteRow *row);

private:
	boost::shared_ptr<SqliteStatement> m_Statement;
	bool m_EndReached;
};

class I2_BASE_API SqliteRowIterator : public boost::iterator_facade<SqliteRowIterator, const SqliteRow, boost::forward_traversal_tag>
{
public:
	SqliteRowIterator(void)
		: m_RowNum(-1)
	{ }

	SqliteRowIterator(const boost::shared_ptr<SqliteCursor>& cursor)
		: m_Cursor(cursor)
	{
		if (cursor->FetchRow(&m_Row))
			m_RowNum = 0;
		else
			m_RowNum = -1;
	}

private:
	friend class boost::iterator_core_access;

	boost::shared_ptr<SqliteCursor> m_Cursor;
	int m_RowNum;
	SqliteRow m_Row;

	void increment(void)
	{
		if (m_Cursor->FetchRow(&m_Row))
			m_RowNum++;
		else
			m_RowNum = -1;
	}

	bool equal(const SqliteRowIterator& other) const
	{
		/* is one of the cursors the 'end' cursor? */
		if ((!other.m_Cursor || other.m_RowNum == -1) && (!m_Cursor || m_RowNum == -1))
			return true;

		if (other.m_Row == m_Row)
			return true;

		return false;
	}

	const SqliteRow& dereference(void) const
	{
		return m_Row;
	}
};

/**
 * An SQLite prepared statement.
 */
class I2_BASE_API SqliteStatement : public boost::enable_shared_from_this<SqliteStatement>
{
public:
	SqliteStatement(const boost::shared_ptr<SqliteDatabase>& database, const String& stmt);
	~SqliteStatement(void);

	boost::shared_ptr<SqliteDatabase> GetDatabase(void) const;

	void Reset(void);
	void Bind(int arg, const Value& value);
	std::pair<SqliteRowIterator, SqliteRowIterator> Exec(void);

	sqlite3_stmt *GetHandle(void) const;

private:
	boost::shared_ptr<SqliteDatabase> m_Database;
	sqlite3_stmt *m_Statement;
};


/**
 * An SQLite database.
 */
class I2_BASE_API SqliteDatabase : public boost::enable_shared_from_this<SqliteDatabase>
{
private:
	struct FactoryKey { };

public:
	SqliteDatabase(const String& filename, FactoryKey k = FactoryKey());
	~SqliteDatabase(void);

	void BeginTransaction(void);
	void CommitTransaction(void);
	void RollbackTransaction(void);

	std::pair<SqliteRowIterator, SqliteRowIterator> Exec(const String& stmt);
	boost::shared_ptr<SqliteStatement> PrepareStatement(const String& stmt);
	int GetChanges(void);

	sqlite3 *GetHandle(void) const;

	static boost::shared_ptr<SqliteDatabase> Open(const String& filename);

private:
	sqlite3 *m_Database;
};

}

#endif /* SQLITEDATABASE_H */
