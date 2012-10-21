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

#include "vmime/net/pop3/POP3Folder.hpp"

#include "vmime/net/pop3/POP3Store.hpp"
#include "vmime/net/pop3/POP3Message.hpp"

#include "vmime/net/pop3/POP3Utils.hpp"

#include "vmime/exception.hpp"


namespace vmime {
namespace net {
namespace pop3 {


POP3Folder::POP3Folder(const folder::path& path, 
	std::shared_ptr<POP3Store> store)
	: m_store(store), m_path(path),
	  m_name(path.isEmpty() ? folder::path::component("") : path.getLastComponent()),
	  m_mode(-1), m_open(false)
{
	store->registerFolder(this);
}


POP3Folder::~POP3Folder()
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (store)
	{
		if (m_open)
			close(false);

		store->unregisterFolder(this);
	}
	else if (m_open)
	{
		onClose();
	}
}


int POP3Folder::getMode() const
{
	if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	return (m_mode);
}


int POP3Folder::getType()
{
	if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	if (m_path.isEmpty())
		return (TYPE_CONTAINS_FOLDERS);
	else if (m_path.getSize() == 1 && m_path[0].getBuffer() == "INBOX")
		return (TYPE_CONTAINS_MESSAGES);
	else
		throw exceptions::folder_not_found();
}


int POP3Folder::getFlags()
{
	return (0);
}


const folder::path::component POP3Folder::getName() const
{
	return (m_name);
}


const folder::path POP3Folder::getFullPath() const
{
	return (m_path);
}


void POP3Folder::open(const int mode, bool failIfModeIsNotAvailable)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");

	if (m_path.isEmpty())
	{
		if (mode != MODE_READ_ONLY && failIfModeIsNotAvailable)
			throw exceptions::operation_not_supported();

		m_open = true;
		m_mode = mode;

		m_messageCount = 0;
	}
	else if (m_path.getSize() == 1 && m_path[0].getBuffer() == "INBOX")
	{
		store->sendRequest("STAT");

		string response;
		store->readResponse(response, false);

		if (!store->isSuccessResponse(response))
			throw exceptions::command_error("STAT", response);

		store->stripResponseCode(response, response);

		std::istringstream iss(response);
		iss >> m_messageCount;

		if (iss.fail())
			throw exceptions::invalid_response("STAT", response);

		m_open = true;
		m_mode = mode;
	}
	else
	{
		throw exceptions::folder_not_found();
	}
}

void POP3Folder::close(const bool expunge)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared
	//POP3Store* store = m_store;

	if (!store)
		throw exceptions::illegal_state("Store disconnected");

	if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	if (!expunge)
	{
		store->sendRequest("RSET");

		string response;
		store->readResponse(response, false);
	}

	m_open = false;
	m_mode = -1;

	onClose();
}


void POP3Folder::onClose()
{
	for (MessageMap::iterator it = m_messages.begin() ; it != m_messages.end() ; ++it)
		(*it).first->onFolderClosed();

	m_messages.clear();
}


void POP3Folder::create(const int /* type */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::destroy()
{
	throw exceptions::operation_not_supported();
}


bool POP3Folder::exists()
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");

	return (m_path.isEmpty() || (m_path.getSize() == 1 && m_path[0].getBuffer() == "INBOX"));
}


bool POP3Folder::isOpen() const
{
	return (m_open);
}


std::shared_ptr<message> POP3Folder::getMessage(const int num)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");
	else if (num < 1 || num > m_messageCount)
		throw exceptions::message_not_found();

	// TODO shared
	//return std::make_shared<POP3Message>(std::dynamic_pointer_cast<POP3Folder>(thisRef()), num);
	return vmime::factory<POP3Message>::create(
		std::dynamic_pointer_cast<POP3Folder>(thisRef()), num);
}


std::vector <std::shared_ptr<message> > POP3Folder::getMessages(const int from, const int to)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	const int to2 = (to == -1 ? m_messageCount : to);

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");
	else if (to2 < from || from < 1 || to2 < 1 || from > m_messageCount || to2 > m_messageCount)
		throw exceptions::message_not_found();

	std::vector <std::shared_ptr<message> > v;
	//std::shared_ptr<POP3Folder> thisFolder = thisRef().dynamicCast <POP3Folder>(); TODO shared
	std::shared_ptr<POP3Folder> thisFolder = std::dynamic_pointer_cast<POP3Folder>(thisRef());

	for (int i = from ; i <= to2 ; ++i) {
		//v.push_back(std::make_shared<POP3Message>(thisFolder, i));
		v.push_back(vmime::factory<POP3Message>::create(thisFolder, i));
	}

	return (v);
}


std::shared_ptr<message> POP3Folder::getMessageByUID(const message::uid& /* uid */)
{
	throw exceptions::operation_not_supported();
}


std::vector <std::shared_ptr<message> > POP3Folder::getMessagesByUID(const std::vector <message::uid>& /* uids */)
{
	throw exceptions::operation_not_supported();
}


std::vector <std::shared_ptr<message> > POP3Folder::getMessages(const std::vector <int>& nums)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	std::vector <std::shared_ptr<message> > v;
	//std::shared_ptr<POP3Folder> thisFolder = thisRef().dynamicCast <POP3Folder>(); TODO shared
	std::shared_ptr<POP3Folder> thisFolder = std::dynamic_pointer_cast<POP3Folder>(thisRef());

	for (std::vector <int>::const_iterator it = nums.begin() ; it != nums.end() ; ++it)
	{
		if (*it < 1|| *it > m_messageCount)
			throw exceptions::message_not_found();

		//v.push_back(std::make_shared<POP3Message>(thisFolder, *it));
		v.push_back(vmime::factory<POP3Message>::create(thisFolder, *it));
	}

	return (v);
}


int POP3Folder::getMessageCount()
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store) {
		throw exceptions::illegal_state("Store disconnected");
	} else if (!isOpen()) {
		throw exceptions::illegal_state("Folder not open");
	}

	return (m_messageCount);
}


std::shared_ptr<folder> POP3Folder::getFolder(const folder::path::component& name)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");

	//return std::make_shared<POP3Folder>(m_path / name, store);
	return vmime::factory<POP3Folder>::create(m_path / name, store);
}


std::vector <std::shared_ptr<folder> > POP3Folder::getFolders(const bool /* recursive */)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");

	if (m_path.isEmpty())
	{
		std::vector <std::shared_ptr<folder> > v;
		//v.push_back(std::make_shared<POP3Folder>(folder::path::component("INBOX"), store));
		v.push_back(vmime::factory<POP3Folder>::create(
			folder::path::component("INBOX"), store)
		);
		return (v);
	}
	else
	{
		std::vector <std::shared_ptr<folder> > v;
		return (v);
	}
}


void POP3Folder::fetchMessages(std::vector <std::shared_ptr<message> >& msg, const int options,
                               utility::progressListener* progress)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store) {
		throw exceptions::illegal_state("Store disconnected");
	} else if (!isOpen()) {
		throw exceptions::illegal_state("Folder not open");
	}

	const int total = msg.size();
	int current = 0;

	if (progress)
		progress->start(total);

	for (std::vector <std::shared_ptr<message> >::iterator it = msg.begin() ;
	     it != msg.end() ; ++it)
	{
		//(*it).dynamicCast <POP3Message>()->fetch TODO shared
			//(thisRef().dynamicCast <POP3Folder>(), options); TODO shared
		std::dynamic_pointer_cast<POP3Message>(*it)->fetch(
			std::dynamic_pointer_cast<POP3Folder>(thisRef()), options);

		if (progress)
			progress->progress(++current, total);
	}

	if (options & FETCH_SIZE)
	{
		// Send the "LIST" command
		std::ostringstream command;
		command << "LIST";

		store->sendRequest(command.str());

		// Get the response
		string response;
		store->readResponse(response, true, NULL);

		if (store->isSuccessResponse(response))
		{
			store->stripFirstLine(response, response, NULL);

			// C: LIST
			// S: +OK
			// S: 1 47548
			// S: 2 12653
			// S: .
			std::map <int, string> result;
			POP3Utils::parseMultiListOrUidlResponse(response, result);

			for (std::vector <std::shared_ptr<message> >::iterator it = msg.begin() ;
			     it != msg.end() ; ++it)
			{
				//std::shared_ptr<POP3Message> m = (*it).dynamicCast <POP3Message>(); TODO shared
				std::shared_ptr<POP3Message> m = std::dynamic_pointer_cast<POP3Message>(*it);

				std::map <int, string>::const_iterator x = result.find(m->m_num);

				if (x != result.end())
				{
					int size = 0;

					std::istringstream iss((*x).second);
					iss >> size;

					m->m_size = size;
				}
			}
		}

	}

	if (options & FETCH_UID)
	{
		// Send the "UIDL" command
		std::ostringstream command;
		command << "UIDL";

		store->sendRequest(command.str());

		// Get the response
		string response;
		store->readResponse(response, true, NULL);

		if (store->isSuccessResponse(response))
		{
			store->stripFirstLine(response, response, NULL);

			// C: UIDL
			// S: +OK
			// S: 1 whqtswO00WBw418f9t5JxYwZ
			// S: 2 QhdPYR:00WBw1Ph7x7
			// S: .
			std::map <int, string> result;
			POP3Utils::parseMultiListOrUidlResponse(response, result);

			for (std::vector <std::shared_ptr<message> >::iterator it = msg.begin() ;
			     it != msg.end() ; ++it)
			{
				// std::shared_ptr<POP3Message> m = (*it).dynamicCast
				// <POP3Message>(); TODO shared
				std::shared_ptr<POP3Message> m =
					std::dynamic_pointer_cast<POP3Message>(*it);

				std::map <int, string>::const_iterator x = result.find(m->m_num);

				if (x != result.end())
					m->m_uid = (*x).second;
			}
		}
	}

	if (progress)
		progress->stop(total);
}


void POP3Folder::fetchMessage(std::shared_ptr<message> msg, const int options)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	// msg.dynamicCast <POP3Message>()->fetch TODO shared
		// (thisRef().dynamicCast <POP3Folder>(), options); TODO shared
	std::dynamic_pointer_cast<POP3Message>(msg)->fetch(
		std::dynamic_pointer_cast<POP3Folder>(thisRef()), options);

	if (options & FETCH_SIZE)
	{
		// Send the "LIST" command
		std::ostringstream command;
		command.imbue(std::locale::classic());

		command << "LIST " << msg->getNumber();

		store->sendRequest(command.str());

		// Get the response
		string response;
		store->readResponse(response, false, NULL);

		if (store->isSuccessResponse(response))
		{
			store->stripResponseCode(response, response);

			// C: LIST 2
			// S: +OK 2 4242
			string::iterator it = response.begin();

			while (it != response.end() && (*it == ' ' || *it == '\t')) ++it;
			while (it != response.end() && !(*it == ' ' || *it == '\t')) ++it;
			while (it != response.end() && (*it == ' ' || *it == '\t')) ++it;

			if (it != response.end())
			{
				int size = 0;

				std::istringstream iss(string(it, response.end()));
				iss >> size;

				// msg.dynamicCast <POP3Message>()->m_size = size; TODO shared
				std::dynamic_pointer_cast<POP3Message>(msg)->m_size = size;
			}
		}
	}

	if (options & FETCH_UID)
	{
		// Send the "UIDL" command
		std::ostringstream command;
		command.imbue(std::locale::classic());

		command << "UIDL " << msg->getNumber();

		store->sendRequest(command.str());

		// Get the response
		string response;
		store->readResponse(response, false, NULL);

		if (store->isSuccessResponse(response))
		{
			store->stripResponseCode(response, response);

			// C: UIDL 2
			// S: +OK 2 QhdPYR:00WBw1Ph7x7
			string::iterator it = response.begin();

			while (it != response.end() && (*it == ' ' || *it == '\t')) ++it;
			while (it != response.end() && !(*it == ' ' || *it == '\t')) ++it;
			while (it != response.end() && (*it == ' ' || *it == '\t')) ++it;

			if (it != response.end())
			{
				// msg.dynamicCast <POP3Message>()->m_uid = TODO shared
					// string(it, response.end()); TODO shared
				std::dynamic_pointer_cast<POP3Message>(msg)->m_uid =
					string(it, response.end());
			}
		}
	}
}


int POP3Folder::getFetchCapabilities() const
{
	return (FETCH_ENVELOPE | FETCH_CONTENT_INFO |
	        FETCH_SIZE | FETCH_FULL_HEADER | FETCH_UID |
	        FETCH_IMPORTANCE);
}


std::shared_ptr<folder> POP3Folder::getParent()
{
	if (m_path.isEmpty())
		return NULL;
	else {
		// return std::make_shared<POP3Folder>(m_path.getParent(),
		// m_store.acquire()); TODO shared
		return vmime::factory<POP3Folder>::create(m_path.getParent(),
				m_store.lock());
		//return std::make_shared<POP3Folder>(m_path.getParent(), m_store);
	}
}


std::shared_ptr<const store> POP3Folder::getStore() const
{
	// return m_store.acquire(); TODO shared
	return m_store.lock();
}


std::shared_ptr<store> POP3Folder::getStore()
{
	// return m_store.acquire(); TODO shared
	return m_store.lock();
}


void POP3Folder::registerMessage(POP3Message* msg)
{
	m_messages.insert(MessageMap::value_type(msg, msg->getNumber()));
}


void POP3Folder::unregisterMessage(POP3Message* msg)
{
	m_messages.erase(msg);
}


void POP3Folder::onStoreDisconnected()
{
	// m_store = NULL; TODO shared
	m_store.reset();
}


void POP3Folder::deleteMessage(const int num)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared
	//POP3Store* store = m_store;

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	std::ostringstream command;
	command.imbue(std::locale::classic());

	command << "DELE " << num;

	store->sendRequest(command.str());

	string response;
	store->readResponse(response, false);

	if (!store->isSuccessResponse(response))
		throw exceptions::command_error("DELE", response);

	// Update local flags
	for (std::map <POP3Message*, int>::iterator it =
	     m_messages.begin() ; it != m_messages.end() ; ++it)
	{
		POP3Message* msg = (*it).first;

		if (msg->getNumber() == num)
			msg->m_deleted = true;
	}

	// Notify message flags changed
	std::vector <int> nums;
	nums.push_back(num);

	//events::messageChangedEvent event TODO shared
	//	(thisRef().dynamicCast <folder>(),
	//	 events::messageChangedEvent::TYPE_FLAGS, nums);
	events::messageChangedEvent event
		(std::dynamic_pointer_cast<folder>(thisRef()),
		 events::messageChangedEvent::TYPE_FLAGS, nums);

	notifyMessageChanged(event);
}


void POP3Folder::deleteMessages(const int from, const int to)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared
	//POP3Store* store = m_store;

	if (from < 1 || (to < from && to != -1))
		throw exceptions::invalid_argument();

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	const int to2 = (to == -1 ? m_messageCount : to);

	for (int i = from ; i <= to2 ; ++i)
	{
		std::ostringstream command;
		command.imbue(std::locale::classic());

		command << "DELE " << i;

		store->sendRequest(command.str());

		string response;
		store->readResponse(response, false);

		if (!store->isSuccessResponse(response))
			throw exceptions::command_error("DELE", response);
	}

	// Update local flags
	for (std::map <POP3Message*, int>::iterator it =
	     m_messages.begin() ; it != m_messages.end() ; ++it)
	{
		POP3Message* msg = (*it).first;

		if (msg->getNumber() >= from && msg->getNumber() <= to2)
			msg->m_deleted = true;
	}

	// Notify message flags changed
	std::vector <int> nums;

	for (int i = from ; i <= to2 ; ++i)
		nums.push_back(i);

	//events::messageChangedEvent event TODO shared
	//	(thisRef().dynamicCast <folder>(),
	//	 events::messageChangedEvent::TYPE_FLAGS, nums);
	events::messageChangedEvent event
		(std::dynamic_pointer_cast<folder>(thisRef()),
		 events::messageChangedEvent::TYPE_FLAGS, nums);

	notifyMessageChanged(event);
}


void POP3Folder::deleteMessages(const std::vector <int>& nums)
{
	 std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared

	if (nums.empty())
		throw exceptions::invalid_argument();

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	for (std::vector <int>::const_iterator
	     it = nums.begin() ; it != nums.end() ; ++it)
	{
		std::ostringstream command;
		command.imbue(std::locale::classic());

		command << "DELE " << (*it);

		store->sendRequest(command.str());

		string response;
		store->readResponse(response, false);

		if (!store->isSuccessResponse(response))
			throw exceptions::command_error("DELE", response);
	}

	// Sort message list
	std::vector <int> list;

	list.resize(nums.size());
	std::copy(nums.begin(), nums.end(), list.begin());

	std::sort(list.begin(), list.end());

	// Update local flags
	for (std::map <POP3Message*, int>::iterator it =
	     m_messages.begin() ; it != m_messages.end() ; ++it)
	{
		POP3Message* msg = (*it).first;

		if (std::binary_search(list.begin(), list.end(), msg->getNumber()))
			msg->m_deleted = true;
	}

	// Notify message flags changed
	//events::messageChangedEvent event TODO shared
	//	(thisRef().dynamicCast <folder>(),
	//	 events::messageChangedEvent::TYPE_FLAGS, list);
	events::messageChangedEvent event
		(std::dynamic_pointer_cast<folder>(thisRef()),
		 events::messageChangedEvent::TYPE_FLAGS, list);

	notifyMessageChanged(event);
}


void POP3Folder::setMessageFlags(const int /* from */, const int /* to */,
	const int /* flags */, const int /* mode */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::setMessageFlags(const std::vector <int>& /* nums */,
	const int /* flags */, const int /* mode */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::rename(const folder::path& /* newPath */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::addMessage(std::shared_ptr<vmime::message> /* msg */, const int /* flags */,
	vmime::datetime* /* date */, utility::progressListener* /* progress */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::addMessage(utility::inputStream& /* is */, const int /* size */, const int /* flags */,
	vmime::datetime* /* date */, utility::progressListener* /* progress */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::copyMessage(const folder::path& /* dest */, const int /* num */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::copyMessages(const folder::path& /* dest */, const int /* from */, const int /* to */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::copyMessages(const folder::path& /* dest */, const std::vector <int>& /* nums */)
{
	throw exceptions::operation_not_supported();
}


void POP3Folder::status(int& count, int& unseen)
{
	std::shared_ptr<POP3Store> store = m_store.lock(); //TODO shared
	//POP3Store* store = m_store;

	if (!store)
		throw exceptions::illegal_state("Store disconnected");
	else if (!isOpen())
		throw exceptions::illegal_state("Folder not open");

	store->sendRequest("STAT");

	string response;
	store->readResponse(response, false);

	if (!store->isSuccessResponse(response))
		throw exceptions::command_error("STAT", response);

	store->stripResponseCode(response, response);

	std::istringstream iss(response);
	iss >> count;

	unseen = count;

	// Update local message count
	if (m_messageCount != count)
	{
		const int oldCount = m_messageCount;

		m_messageCount = count;

		if (count > oldCount)
		{
			std::vector <int> nums;
			nums.reserve(count - oldCount);

			for (int i = oldCount + 1, j = 0 ; i <= count ; ++i, ++j)
				nums[j] = i;

			// Notify message count changed
			//events::messageCountEvent event
			//	(thisRef().dynamicCast <folder>(),
			//	 events::messageCountEvent::TYPE_ADDED, nums);
			events::messageCountEvent event
				(std::dynamic_pointer_cast<folder>(thisRef()),
				 events::messageCountEvent::TYPE_ADDED, nums);

			notifyMessageCount(event);

			// Notify folders with the same path
			for (std::list <POP3Folder*>::iterator it = store->m_folders.begin() ;
			     it != store->m_folders.end() ; ++it)
			{
				if ((*it) != this && (*it)->getFullPath() == m_path)
				{
					(*it)->m_messageCount = count;

					//events::messageCountEvent event TODO shared
						//((*it)->thisRef().dynamicCast <folder>(),
						 //events::messageCountEvent::TYPE_ADDED, nums);
					events::messageCountEvent event
						(std::dynamic_pointer_cast<folder>((*it)->thisRef()),
						 events::messageCountEvent::TYPE_ADDED, nums);

					(*it)->notifyMessageCount(event);
				}
			}
		}
	}
}


void POP3Folder::expunge()
{
	// Not supported by POP3 protocol (deleted messages are automatically
	// expunged at the end of the session...).
}


std::vector <int> POP3Folder::getMessageNumbersStartingOnUID(const message::uid& /* uid */)
{
	throw exceptions::operation_not_supported();
}


} // pop3
} // net
} // vmime
