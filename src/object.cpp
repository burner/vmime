//
// VMime library (http://www.vmime.org)
// Copyright (C) 2002-2009 Vincent Richard <vincent@vincent-richard.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library.  Thus, the terms and conditions of
// the GNU General Public License cover the whole combination.
//

#include "vmime/types.hpp"
#include "vmime/object.hpp"
#include <iostream>


#ifndef VMIME_BUILDING_DOC


namespace vmime
{


object::object() : enable_shared_from_this()
	// : m_refMgr(utility::refManager::create(this)) TODO shared
	//: m_thisRef(std::make_shared<object>(this))
	//: m_thisRef(std::shared_ptr<object>(this))
	//: thisShr(std::shared_ptr<object>(this))
{
}


object::object(const object&): enable_shared_from_this()
	// : m_refMgr(utility::refManager::create(this)) TODO shared
	//: m_thisRef(std::make_shared<object>(this))
	//: m_thisRef(std::shared_ptr<object>(this))
	//: thisShr(std::shared_ptr<object>(this))
{
}

object::object(object* const) : enable_shared_from_this()
	// : m_refMgr(utility::refManager::create(this)) TODO shared
	//: m_thisRef(std::make_shared<object>(this))
	//: m_thisRef(std::shared_ptr<object>(this))
	//: thisShr(std::shared_ptr<object>(this))
{
}

object& object::operator=(const object&)
{
	// Do _NOT_ copy 'm_refMgr'
	return *this;
}

object::~object() { }


std::shared_ptr<object> object::thisRef()
{
	// m_refMgr->addStrong(); TODO shared
	//return m_thisRef.lock();
	/*if(thisShr) {
		std::shared_ptr<object> ret(this->thisShr);
		this->thisWeak = this->thisShr;
		this->thisShr.reset();
		return ret;
	}
	return this->thisWeak.lock();*/
	return this->shared_from_this();
}


/*std::shared_ptr<const object> object::thisRef() 
{
	// m_refMgr->addStrong(); TODO shared
	//return m_thisRef.lock();
	if(thisShr) {
		std::shared_ptr<const object> ret(this->thisShr);
		this->thisWeak = this->thisShr;
		this->thisShr.reset();
		return ret;
	}
	return std::dynamic_pointer_cast<const object>(this->thisWeak.lock());
}


std::weak_ptr<object> object::thisWeakRef()
{
	return std::weak_ptr<object>(m_thisRef); // TODO shared
}


std::weak_ptr<const object> object::thisWeakRef() const
{
	return std::weak_ptr<const object>(m_thisRef); // TODO shared
}


void object::setRefManager(utility::refManager* mgr)
{
	// m_refMgr = mgr; TODO shared
	std::err<<__FILE__<<':'<<__LINE__<<" very bad place check"<<std::endl;
}*/


/* utility::refManager* object::getRefManager() const
{
	// return m_refMgr; TODO shared
	std::err<<__FILE__<<':'<<__LINE__<<" very bad place check"<<std::endl;
	return NULL;
}*/


} // vmime


#endif // VMIME_BUILDING_DOC

