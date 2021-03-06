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

#include "vmime/mailboxList.hpp"
#include "vmime/exception.hpp"


namespace vmime
{


mailboxList::mailboxList()
{
}


mailboxList::mailboxList(const mailboxList& mboxList)
	: headerFieldValue(), m_list(mboxList.m_list)
{
}


void mailboxList::appendMailbox(std::shared_ptr<mailbox> mbox)
{
	m_list.appendAddress(mbox);
}


void mailboxList::insertMailboxBefore(std::shared_ptr<mailbox> beforeMailbox, std::shared_ptr<mailbox> mbox)
{
	try
	{
		m_list.insertAddressBefore(beforeMailbox, mbox);
	}
	catch (exceptions::no_such_address&)
	{
		throw exceptions::no_such_mailbox();
	}
}


void mailboxList::insertMailboxBefore(const int pos, std::shared_ptr<mailbox> mbox)
{
	m_list.insertAddressBefore(pos, mbox);
}


void mailboxList::insertMailboxAfter(std::shared_ptr<mailbox> afterMailbox, std::shared_ptr<mailbox> mbox)
{
	try
	{
		m_list.insertAddressAfter(afterMailbox, mbox);
	}
	catch (exceptions::no_such_address&)
	{
		throw exceptions::no_such_mailbox();
	}
}


void mailboxList::insertMailboxAfter(const int pos, std::shared_ptr<mailbox> mbox)
{
	m_list.insertAddressAfter(pos, mbox);
}


void mailboxList::removeMailbox(std::shared_ptr<mailbox> mbox)
{
	try
	{
		m_list.removeAddress(mbox);
	}
	catch (exceptions::no_such_address&)
	{
		throw exceptions::no_such_mailbox();
	}
}


void mailboxList::removeMailbox(const int pos)
{
	m_list.removeAddress(pos);
}


void mailboxList::removeAllMailboxes()
{
	m_list.removeAllAddresses();
}


int mailboxList::getMailboxCount() const
{
	return (m_list.getAddressCount());
}


bool mailboxList::isEmpty() const
{
	return (m_list.isEmpty());
}


std::shared_ptr<mailbox> mailboxList::getMailboxAt(const int pos)
{
	// TODO shared
	return std::static_pointer_cast<mailbox>(m_list.getAddressAt(pos));
}


const std::shared_ptr<const mailbox> mailboxList::getMailboxAt(const int pos) const
{
	//TODO shared
	return std::static_pointer_cast<const mailbox>(m_list.getAddressAt(pos));
}


const std::vector <std::shared_ptr<const mailbox> > mailboxList::getMailboxList() const
{
	const std::vector <std::shared_ptr<const address> > addrList = m_list.getAddressList();
	std::vector <std::shared_ptr<const mailbox> > res;

	for (std::vector <std::shared_ptr<const address> >::const_iterator it = addrList.begin() ;
	     it != addrList.end() ; ++it)
	{
		// const std::shared_ptr<const mailbox> mbox = (*it).dynamicCast
		// <const mailbox>(); TODO shared
		const std::shared_ptr<const mailbox> mbox =
			std::dynamic_pointer_cast<const mailbox>(*it);

		if (mbox != NULL)
			res.push_back(mbox);
	}

	return (res);
}


const std::vector <std::shared_ptr<mailbox> > mailboxList::getMailboxList()
{
	const std::vector <std::shared_ptr<address> > addrList = m_list.getAddressList();
	std::vector <std::shared_ptr<mailbox> > res;

	for (std::vector <std::shared_ptr<address> >::const_iterator it = addrList.begin() ;
	     it != addrList.end() ; ++it)
	{
		// const std::shared_ptr<mailbox> mbox = (*it).dynamicCast
		// <mailbox>(); TODO shared
		const std::shared_ptr<mailbox> mbox = std::dynamic_pointer_cast<mailbox>(*it);

		if (mbox != NULL)
			res.push_back(mbox);
	}

	return (res);
}


std::shared_ptr<component> mailboxList::clone() const
{
	return vmime::factory<mailboxList>::create(*this);
}


void mailboxList::copyFrom(const component& other)
{
	const mailboxList& mboxList = dynamic_cast <const mailboxList&>(other);

	m_list = mboxList.m_list;
}


mailboxList& mailboxList::operator=(const mailboxList& other)
{
	copyFrom(other);
	return (*this);
}


const std::vector <std::shared_ptr<component> > mailboxList::getChildComponents()
{
	return (m_list.getChildComponents());
}


void mailboxList::parseImpl(const string& buffer, const string::size_type position,
	const string::size_type end, string::size_type* newPosition)
{
	m_list.parse(buffer, position, end, newPosition);
}


void mailboxList::generateImpl(utility::outputStream& os, const string::size_type maxLineLength,
	const string::size_type curLinePos, string::size_type* newLinePos) const
{
	m_list.generate(os, maxLineLength, curLinePos, newLinePos);
}


std::shared_ptr<addressList> mailboxList::toAddressList() const
{
	// return m_list.clone().dynamicCast <addressList>(); TODO shared
	return std::dynamic_pointer_cast<addressList>(m_list.clone());
}


} // vmime

