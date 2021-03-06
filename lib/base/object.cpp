/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/object.hpp"
#include "base/value.hpp"
#include "base/dictionary.hpp"
#include "base/primitivetype.hpp"
#include "base/utility.hpp"

using namespace icinga;

REGISTER_PRIMITIVE_TYPE(Object, Object::GetPrototype());

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
	: m_References(0)
#ifdef I2_DEBUG
	, m_LockOwner(0)
#endif /* I2_DEBUG */
{ }

/**
 * Destructor for the Object class.
 */
Object::~Object(void)
{ }

/**
 * Returns a string representation for the object.
 */
String Object::ToString(void) const
{
	return "Object of type '" + Utility::GetTypeName(typeid(*this)) + "'";
}

#ifdef I2_DEBUG
/**
 * Checks if the calling thread owns the lock on this object.
 *
 * @returns True if the calling thread owns the lock, false otherwise.
 */
bool Object::OwnsLock(void) const
{
#ifdef _WIN32
	DWORD tid = InterlockedExchangeAdd(&m_LockOwner, 0);

	return (tid == GetCurrentThreadId());
#else /* _WIN32 */
	pthread_t tid = __sync_fetch_and_add(&m_LockOwner, 0);

	return (tid == pthread_self());
#endif /* _WIN32 */
}
#endif /* I2_DEBUG */

void Object::InflateMutex(void)
{
	m_Mutex.Inflate();
}

void Object::SetField(int id, const Value&)
{
	if (id == 0)
		BOOST_THROW_EXCEPTION(std::runtime_error("Prototype field cannot be set."));
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

Value Object::GetField(int id) const
{
	if (id == 0)
		return Empty;
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

