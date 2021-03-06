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

#include "vmime/net/session.hpp"
#include "vmime/net/serviceFactory.hpp"

#include "vmime/net/store.hpp"
#include "vmime/net/transport.hpp"


namespace vmime {
namespace net {


session::session()
{
}


session::session(const session& sess)
	: object(), m_props(sess.m_props)
{
}


session::session(const propertySet& props)
	: m_props(props)
{
}


session::~session()
{
}


std::shared_ptr<transport> session::getTransport(std::shared_ptr<security::authenticator> auth)
{
	return (getTransport(m_props["transport.protocol"], auth));
}


std::shared_ptr<transport> session::getTransport
	(const string& protocol, std::shared_ptr<security::authenticator> auth)
{
	//std::shared_ptr<session> sess = thisRef().dynamicCast <session>(); TODO shared
	std::shared_ptr<service> sv =
		serviceFactory::getInstance()->create(std::dynamic_pointer_cast<session>(thisRef()),
			  	protocol, auth); //TODO shared

	if (sv->getType() != service::TYPE_TRANSPORT)
		throw exceptions::no_service_available();

	return std::static_pointer_cast<transport>(sv);
}


std::shared_ptr<transport> session::getTransport
	(const utility::url& url, std::shared_ptr<security::authenticator> auth)
{
	//std::shared_ptr<session> sess = thisRef().dynamicCast <session>(); TODO shared
	std::shared_ptr<service> sv =
		serviceFactory::getInstance()->create(std::dynamic_pointer_cast<session>(thisRef()), url, auth);


	if (sv->getType() != service::TYPE_TRANSPORT)
		throw exceptions::no_service_available();

	return std::static_pointer_cast<transport>(sv);
}


std::shared_ptr<store> session::getStore(std::shared_ptr<security::authenticator> auth)
{
	return (getStore(m_props["store.protocol"], auth));
}


std::shared_ptr<store> session::getStore
	(const string& protocol, std::shared_ptr<security::authenticator> auth)
{
	//std::shared_ptr<session> sess = thisRef().dynamicCast <session>(); TODO shared
	std::shared_ptr<service> sv = serviceFactory::getInstance()->create(
		std::dynamic_pointer_cast<vmime::net::session>(thisRef()), protocol, auth);

	if (sv->getType() != service::TYPE_STORE)
		throw exceptions::no_service_available();

	//return sv.staticCast <store>(); TODO shared
	return std::static_pointer_cast<store>(sv);
}


std::shared_ptr<store> session::getStore
	(const utility::url& url, std::shared_ptr<security::authenticator> auth)
{
	//std::shared_ptr<session> sess = thisRef().dynamicCast <session>(); TODO shared
	std::shared_ptr<service> sv =
		serviceFactory::getInstance()->create(std::dynamic_pointer_cast<session>(thisRef()), url, auth); //TODO shared

	if (sv->getType() != service::TYPE_STORE)
		throw exceptions::no_service_available();

	return std::static_pointer_cast<store>(sv);
}


const propertySet& session::getProperties() const
{
	return (m_props);
}


propertySet& session::getProperties()
{
	return (m_props);
}


} // net
} // vmime
