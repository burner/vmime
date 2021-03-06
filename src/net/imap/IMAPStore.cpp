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

#include "vmime/net/imap/IMAPStore.hpp"
#include "vmime/net/imap/IMAPFolder.hpp"
#include "vmime/net/imap/IMAPConnection.hpp"

#include "vmime/exception.hpp"
#include "vmime/platform.hpp"

#include <map>


namespace vmime {
namespace net {
namespace imap {


IMAPStore::IMAPStore(std::shared_ptr<session> sess, std::shared_ptr<security::authenticator> auth, const bool secured)
	: store(sess, getInfosInstance(), auth), m_connection(NULL), m_isIMAPS(secured)
{
}


IMAPStore::~IMAPStore()
{
	try
	{
		if (isConnected())
			disconnect();
	}
	catch (vmime::exception&)
	{
		// Ignore
	}
}


const string IMAPStore::getProtocolName() const
{
	return "imap";
}


std::shared_ptr<folder> IMAPStore::getRootFolder()
{
	if (!isConnected())
		throw exceptions::illegal_state("Not connected");

	// TODO shared
	return vmime::factory<IMAPFolder>::create(folder::path(),
		std::dynamic_pointer_cast<IMAPStore>(thisRef()));
}


std::shared_ptr<folder> IMAPStore::getDefaultFolder()
{
	if (!isConnected())
		throw exceptions::illegal_state("Not connected");

	return vmime::factory<IMAPFolder>::create(folder::path::component("INBOX"),
		// thisRef().dynamicCast <IMAPStore>()); TODO shared
		std::dynamic_pointer_cast<IMAPStore>(thisRef()));
}


std::shared_ptr<folder> IMAPStore::getFolder(const folder::path& path)
{
	if (!isConnected())
		throw exceptions::illegal_state("Not connected");

	return vmime::factory<IMAPFolder>::create(path,
			std::dynamic_pointer_cast<IMAPStore>(thisRef()));
}


bool IMAPStore::isValidFolderName(const folder::path::component& /* name */) const
{
	return true;
}


void IMAPStore::connect()
{
	if (isConnected())
		throw exceptions::already_connected();

	m_connection = vmime::factory<IMAPConnection>::create
		// (thisRef().dynamicCast <IMAPStore>(), getAuthenticator()); TODO
		// shared
		(std::dynamic_pointer_cast<IMAPStore>(thisRef()), getAuthenticator());

	try
	{
		m_connection->connect();
	}
	catch (std::exception&)
	{
		m_connection = NULL;
		throw;
	}
}


bool IMAPStore::isConnected() const
{
	return (m_connection && m_connection->isConnected());
}


bool IMAPStore::isIMAPS() const
{
	return m_isIMAPS;
}


bool IMAPStore::isSecuredConnection() const
{
	if (m_connection == NULL)
		return false;

	return m_connection->isSecuredConnection();
}


std::shared_ptr<connectionInfos> IMAPStore::getConnectionInfos() const
{
	if (m_connection == NULL)
		return NULL;

	return m_connection->getConnectionInfos();
}


void IMAPStore::disconnect()
{
	if (!isConnected())
		throw exceptions::not_connected();

	for (std::list <IMAPFolder*>::iterator it = m_folders.begin() ;
	     it != m_folders.end() ; ++it)
	{
		(*it)->onStoreDisconnected();
	}

	m_folders.clear();


	m_connection->disconnect();

	m_connection = NULL;
}


void IMAPStore::noop()
{
	if (!isConnected())
		throw exceptions::not_connected();

	m_connection->send(true, "NOOP", true);

	std::shared_ptr<IMAPParser::response> resp(m_connection->readResponse());

	if (resp->isBad() || resp->response_done()->response_tagged()->
			resp_cond_state()->status() != IMAPParser::resp_cond_state::OK)
	{
		throw exceptions::command_error("NOOP", m_connection->getParser()->lastLine());
	}
}


std::shared_ptr<IMAPConnection> IMAPStore::connection()
{
	return (m_connection);
}


void IMAPStore::registerFolder(IMAPFolder* folder)
{
	m_folders.push_back(folder);
}


void IMAPStore::unregisterFolder(IMAPFolder* folder)
{
	std::list <IMAPFolder*>::iterator it = std::find(m_folders.begin(), m_folders.end(), folder);
	if (it != m_folders.end()) m_folders.erase(it);
}


int IMAPStore::getCapabilities() const
{
	return (CAPABILITY_CREATE_FOLDER |
	        CAPABILITY_RENAME_FOLDER |
	        CAPABILITY_ADD_MESSAGE |
	        CAPABILITY_COPY_MESSAGE |
	        CAPABILITY_DELETE_MESSAGE |
	        CAPABILITY_PARTIAL_FETCH |
	        CAPABILITY_MESSAGE_FLAGS |
	        CAPABILITY_EXTRACT_PART);
}



// Service infos

IMAPServiceInfos IMAPStore::sm_infos(false);


const serviceInfos& IMAPStore::getInfosInstance()
{
	return sm_infos;
}


const serviceInfos& IMAPStore::getInfos() const
{
	return sm_infos;
}


} // imap
} // net
} // vmime
