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

#include "base/dynamicobject.hpp"
#include "base/dynamictype.hpp"
#include "base/serializer.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/scriptfunction.hpp"
#include "base/initialize.hpp"
#include "base/workqueue.hpp"
#include "base/context.hpp"
#include "base/application.hpp"
#include "base/objectdatabase.hpp"
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

REGISTER_TYPE(DynamicObject);

boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStarted;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStopped;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnPaused;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnResumed;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStateChanged;

DynamicObject::DynamicObject(void)
{ }

DynamicType::Ptr DynamicObject::GetType(void) const
{
	return DynamicType::GetByName(GetTypeName());
}

DebugInfo DynamicObject::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

void DynamicObject::SetDebugInfo(const DebugInfo& di)
{
	m_DebugInfo = di;
}

bool DynamicObject::IsActive(void) const
{
	return GetActive();
}

bool DynamicObject::IsPaused(void) const
{
	return GetPaused();
}

void DynamicObject::SetExtension(const String& key, const Value& value)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions) {
		extensions = new Dictionary();
		SetExtensions(extensions);
	}

	extensions->Set(key, value);
}

Value DynamicObject::GetExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return Empty;

	return extensions->Get(key);
}

void DynamicObject::ClearExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return;

	extensions->Remove(key);
}

void DynamicObject::Register(void)
{
	ASSERT(!OwnsLock());

	DynamicType::Ptr dtype = GetType();
	dtype->RegisterObject(this);
}

void DynamicObject::Start(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStartCalled(true);
}

void DynamicObject::Activate(void)
{
	CONTEXT("Activating object '" + GetName() + "' of type '" + GetType()->GetName() + "'");

	ASSERT(!OwnsLock());

	Start();

	ASSERT(GetStartCalled());

	{
		ObjectLock olock(this);
		ASSERT(!IsActive());
		SetActive(true);
	}

	OnStarted(this);

	SetAuthority(true);
}

void DynamicObject::Stop(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStopCalled(true);
}

void DynamicObject::Deactivate(void)
{
	CONTEXT("Deactivating object '" + GetName() + "' of type '" + GetType()->GetName() + "'");

	ASSERT(!OwnsLock());

	SetAuthority(false);

	{
		ObjectLock olock(this);

		if (!IsActive())
			return;

		SetActive(false);
	}

	Stop();

	ASSERT(GetStopCalled());

	OnStopped(this);
}

void DynamicObject::OnConfigLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::OnAllConfigLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::OnStateLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::Pause(void)
{
	SetPauseCalled(true);
}

void DynamicObject::Resume(void)
{
	SetResumeCalled(true);
}

void DynamicObject::SetAuthority(bool authority)
{
	if (authority && GetPaused()) {
		SetResumeCalled(false);
		Resume();
		ASSERT(GetResumeCalled());
		SetPaused(false);
		OnResumed(this);
	} else if (!authority && !GetPaused()) {
		SetPauseCalled(false);
		Pause();
		ASSERT(GetPauseCalled());
		SetPaused(true);
		OnPaused(this);
	}
}

void DynamicObject::StopObjects(void)
{
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			object->Deactivate();
		}
	}
}

DynamicObject::Ptr DynamicObject::GetObject(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	return dtype->GetObject(name);
}
