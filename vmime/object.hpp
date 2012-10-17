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

#ifndef VMIME_OBJECT_HPP_INCLUDED
#define VMIME_OBJECT_HPP_INCLUDED


#include "vmime/types.hpp"

#include <vector>
#include <memory>
#include <utility>


namespace vmime
{

template< class D_ >
struct CreatoR
{
	template< class T_, typename... Args_ >
	static
	std::shared_ptr< D_ >
	crt( std::shared_ptr<T_> this_ptr, Args_&&... args )
	{
	  return std::make_shared< D_ >( this_ptr, std::forward<Args_>( args) ...  );
	}
};

/** Base object for all objects in the library. This implements
  * reference counting and auto-deletion.
  */

class object
{
	// template <class T> friend class utility::ref; TODO shared
	// template <class T> friend class utility::weak_ref;

	// friend class utility::refManager; // TODO shared
	std::shared_ptr<object> m_thisRef;


public: // TODO shared
	
	object();
	object(const object&);
	object(object* const);

	object& operator=(const object&);

	virtual ~object();

protected:
#ifndef VMIME_BUILDING_DOC

	/** Return a reference to this object.
	  *
	  * @return reference to self
	  */
	std::shared_ptr<object> thisRef();

	/** Return a reference to this object (const version).
	  *
	  * @return reference to self
	  */
	std::shared_ptr<const object> thisRef() const;

	/** Return a weak reference to this object.
	  *
	  * @return weak reference to self
	  */
	std::weak_ptr<object> thisWeakRef();

	/** Return a weak reference to this object (const version).
	  *
	  * @return weak reference to self
	  */
	std::weak_ptr<const object> thisWeakRef() const;


	 // void setRefManager(utility::refManager* mgr); // TODO shared
	 // utility::refManager* getRefManager() const;

#endif // VMIME_BUILDING_DOC

private:

	 // mutable utility::refManager* m_refMgr; // TODO shared
};


} // vmime


#endif // VMIME_OBJECT_HPP_INCLUDED

