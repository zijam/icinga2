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

#ifndef CONFIGCOMPILERCONTEXT_H
#define CONFIGCOMPILERCONTEXT_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/stdiostream.hpp"
#include "base/dictionary.hpp"
#include "base/workqueue.hpp"
#include "base/objectdatabase.hpp"
#include <boost/thread/mutex.hpp>

namespace icinga
{

/*
 * @ingroup config
 */
class I2_CONFIG_API ConfigCompilerContext
{
public:
	void OpenObjectsFile(void);
	void WriteObject(const DynamicObject::Ptr& object, const DebugHint& dhint);
	void FinishObjectsFile(void);

	static ConfigCompilerContext *GetInstance(void);

private:
	boost::shared_ptr<WorkQueue> m_WorkQueue;
	boost::shared_ptr<ObjectDatabase> m_Database;

	mutable boost::mutex m_Mutex;

	void InternalWriteObject(const DynamicObject::Ptr& object, const DebugHint& dhint);
};

}

#endif /* CONFIGCOMPILERCONTEXT_H */
