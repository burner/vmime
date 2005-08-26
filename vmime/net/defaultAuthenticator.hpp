//
// VMime library (http://www.vmime.org)
// Copyright (C) 2002-2005 Vincent Richard <vincent@vincent-richard.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef VMIME_NET_DEFAULTAUTHENTICATOR_HPP_INCLUDED
#define VMIME_NET_DEFAULTAUTHENTICATOR_HPP_INCLUDED


#include "vmime/net/authenticator.hpp"
#include "vmime/propertySet.hpp"


namespace vmime {
namespace net {


class session;


/** Default implementation for authenticator. It simply returns
  * the credentials set in the session properties (named 'username'
  * and 'password'). This is the default implementation used if
  * you do not write your own authenticator object.
  */

class defaultAuthenticator : public authenticator
{
public:

	defaultAuthenticator(weak_ref <session> session, const string& prefix);

	const authenticationInfos requestAuthInfos() const;

private:

	weak_ref <session> m_session;
	const string m_prefix;
};


} // net
} // vmime


#endif // VMIME_NET_DEFAULTAUTHENTICATOR_HPP_INCLUDED
