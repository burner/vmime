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

#include <sstream>

#include <gsasl.h>

#include "vmime/security/sasl/SASLSession.hpp"

#include "vmime/security/sasl/SASLContext.hpp"
#include "vmime/security/sasl/SASLSocket.hpp"
#include "vmime/security/sasl/SASLAuthenticator.hpp"


namespace vmime {
namespace security {
namespace sasl {


SASLSession::SASLSession(const string& serviceName, std::shared_ptr<SASLContext> ctx,
                 std::shared_ptr<authenticator> auth, std::shared_ptr<SASLMechanism> mech)
	: m_serviceName(serviceName), m_context(ctx), m_auth(auth),
	  m_mech(mech), m_gsaslContext(0), m_gsaslSession(0)
{
	if (gsasl_init(&m_gsaslContext) != GSASL_OK)
		throw std::bad_alloc();

	gsasl_client_start(m_gsaslContext, mech->getName().c_str(), &m_gsaslSession);

	gsasl_callback_set(m_gsaslContext, gsaslCallback);
	gsasl_callback_hook_set(m_gsaslContext, this);
}


SASLSession::~SASLSession()
{
	gsasl_finish(m_gsaslSession);
	m_gsaslSession = 0;

	gsasl_done(m_gsaslContext);
	m_gsaslContext = 0;
}


void SASLSession::init()
{
	// std::shared_ptr<SASLAuthenticator> saslAuth = m_auth.dynamicCast
	// <SASLAuthenticator>(); TODO shared
	std::shared_ptr<SASLAuthenticator> saslAuth = std::dynamic_pointer_cast<SASLAuthenticator>(m_auth);

	if (saslAuth)
	{
		saslAuth->setSASLMechanism(m_mech);
		// saslAuth->setSASLSession(thisRef().dynamicCast <SASLSession>());
		// TODO shared
		saslAuth->setSASLSession(std::dynamic_pointer_cast<SASLSession>(thisRef()));
	}
}


std::shared_ptr<authenticator> SASLSession::getAuthenticator()
{
	return m_auth;
}


std::shared_ptr<SASLMechanism> SASLSession::getMechanism()
{
	return m_mech;
}


std::shared_ptr<SASLContext> SASLSession::getContext()
{
	return m_context;
}


bool SASLSession::evaluateChallenge
	(const byte_t* challenge, const int challengeLen,
	 byte_t** response, int* responseLen)
{
	// return m_mech->step(thisRef().dynamicCast <SASLSession>(), TODO shared
	return m_mech->step(std::dynamic_pointer_cast<SASLSession>(thisRef()),
		challenge, challengeLen, response, responseLen);
}


std::shared_ptr<net::socket> SASLSession::getSecuredSocket(std::shared_ptr<net::socket> sok)
{
	// <SASLSession>(), sok); TODO shared
	return vmime::factory<SASLSocket>::create(std::dynamic_pointer_cast<SASLSession>(thisRef()), sok);
}


const string SASLSession::getServiceName() const
{
	return m_serviceName;
}


// static
int SASLSession::gsaslCallback
	(Gsasl* ctx, Gsasl_session* sctx, Gsasl_property prop)
{
	SASLSession* sess = reinterpret_cast <SASLSession*>(gsasl_callback_hook_get(ctx));
	if (!sess) return GSASL_AUTHENTICATION_ERROR;

	std::shared_ptr<authenticator> auth = sess->getAuthenticator();

	try
	{
		string res;

		switch (prop)
		{
		case GSASL_AUTHID:

			res = auth->getUsername();
			break;

		case GSASL_PASSWORD:

			res = auth->getPassword();
			break;

		case GSASL_ANONYMOUS_TOKEN:

			res = auth->getAnonymousToken();
			break;

		case GSASL_HOSTNAME:

			res = auth->getHostname();
			break;

		case GSASL_SERVICE:

			res = auth->getServiceName();
			break;

		case GSASL_AUTHZID:
		case GSASL_GSSAPI_DISPLAY_NAME:
		case GSASL_PASSCODE:
		case GSASL_SUGGESTED_PIN:
		case GSASL_PIN:
		case GSASL_REALM:

		default:

			return GSASL_NO_CALLBACK;
		}

		gsasl_property_set(sctx, prop, res.c_str());

		return GSASL_OK;
	}
	//catch (exceptions::no_auth_information&)
	catch (...)
	{
		return GSASL_NO_CALLBACK;
	}
}


} // sasl
} // security
} // vmime

