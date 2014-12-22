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

#include "config/configcompilercontext.hpp"
#include "base/singleton.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <fstream>

using namespace icinga;

ConfigCompilerContext *ConfigCompilerContext::GetInstance(void)
{
	return Singleton<ConfigCompilerContext>::GetInstance();
}

void ConfigCompilerContext::OpenObjectsFile(void)
{
	m_Database = boost::make_shared<ObjectDatabase>(Application::GetDbPath());

	m_Database->BeginTransaction();
	m_Database->DeactivateAllObjects();

	m_WorkQueue = boost::make_shared<WorkQueue>(25000, 1);
}

void ConfigCompilerContext::WriteObject(const DynamicObject::Ptr& object, const DebugHint& dhint)
{
	if (!m_Database)
		return;

	m_WorkQueue->Enqueue(boost::bind(&ConfigCompilerContext::InternalWriteObject, this, object, dhint));
}

void ConfigCompilerContext::InternalWriteObject(const DynamicObject::Ptr& object, const DebugHint& dhint)
{
	m_Database->SaveObjectConfig(object, dhint.ToDictionary());
}

void ConfigCompilerContext::FinishObjectsFile(void)
{
	m_WorkQueue->Join();
	m_WorkQueue.reset();

	m_Database->Cleanup();
	m_Database->CommitTransaction();
	m_Database.reset();
}
