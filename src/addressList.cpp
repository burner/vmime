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

#include "vmime/addressList.hpp"
#include "vmime/parserHelpers.hpp"
#include "vmime/exception.hpp"
#include "vmime/mailboxList.hpp"
#include "vmime/mailboxGroup.hpp"


namespace vmime
{


addressList::addressList()
{
}


addressList::addressList(const addressList& addrList)
	: headerFieldValue()
{
	copyFrom(addrList);
}


addressList::~addressList()
{
	removeAllAddresses();
}


void addressList::parseImpl(const string& buffer, const string::size_type position,
	const string::size_type end, string::size_type* newPosition)
{
	removeAllAddresses();

	string::size_type pos = position;

	while (pos < end)
	{
		std::shared_ptr<address> parsedAddress = address::parseNext(buffer, pos, end, &pos);

		if (parsedAddress != NULL)
			m_list.push_back(parsedAddress);
	}

	setParsedBounds(position, end);

	if (newPosition)
		*newPosition = end;
}


void addressList::generateImpl(utility::outputStream& os, const string::size_type maxLineLength,
	const string::size_type curLinePos, string::size_type* newLinePos) const
{
	string::size_type pos = curLinePos;

	if (!m_list.empty())
	{
		for (std::vector <std::shared_ptr<address> >::const_iterator i = m_list.begin() ; ; )
		{
			(*i)->generate(os, maxLineLength - 2, pos, &pos);

			if (++i == m_list.end())
				break;

			os << ", ";
			pos += 2;
		}
	}

	if (newLinePos)
		*newLinePos = pos;
}


void addressList::copyFrom(const component& other)
{
	const addressList& addrList = dynamic_cast <const addressList&>(other);

	removeAllAddresses();

	for (std::vector <std::shared_ptr<address> >::const_iterator it = addrList.m_list.begin() ;
	     it != addrList.m_list.end() ; ++it)
	{
		//m_list.push_back((*it)->clone().dynamicCast <address>());
		auto t = (*it)->clone();
		m_list.push_back(std::dynamic_pointer_cast<address>(t));
	}
}


addressList& addressList::operator=(const addressList& other)
{
	copyFrom(other);
	return (*this);
}


addressList& addressList::operator=(const mailboxList& other)
{
	removeAllAddresses();

	for (int i = 0 ; i < other.getMailboxCount() ; ++i) {
		//m_list.push_back(other.getMailboxAt(i)->clone().dynamicCast <address>()); TODO: shared_ptr
		auto c = other.getMailboxAt(i)->clone();
		m_list.push_back(std::dynamic_pointer_cast<address>(c));
		//m_list.push_back(other.getMailboxAt(i)->clone().dynamicCast <address>());
	}

	return (*this);
}


std::shared_ptr<component> addressList::clone() const
{
	return std::make_shared<addressList>(*this);
}


void addressList::appendAddress(std::shared_ptr<address> addr)
{
	m_list.push_back(addr);
}


void addressList::insertAddressBefore(std::shared_ptr<address> beforeAddress, std::shared_ptr<address> addr)
{
	const std::vector <std::shared_ptr<address> >::iterator it = std::find
		(m_list.begin(), m_list.end(), beforeAddress);

	if (it == m_list.end())
		throw exceptions::no_such_address();

	m_list.insert(it, addr);
}


void addressList::insertAddressBefore(const int pos, std::shared_ptr<address> addr)
{
	m_list.insert(m_list.begin() + pos, addr);
}


void addressList::insertAddressAfter(std::shared_ptr<address> afterAddress, std::shared_ptr<address> addr)
{
	const std::vector <std::shared_ptr<address> >::iterator it = std::find
		(m_list.begin(), m_list.end(), afterAddress);

	if (it == m_list.end())
		throw exceptions::no_such_address();

	m_list.insert(it + 1, addr);
}


void addressList::insertAddressAfter(const int pos, std::shared_ptr<address> addr)
{
	m_list.insert(m_list.begin() + pos + 1, addr);
}


void addressList::removeAddress(std::shared_ptr<address> addr)
{
	const std::vector <std::shared_ptr<address> >::iterator it = std::find
		(m_list.begin(), m_list.end(), addr);

	if (it == m_list.end())
		throw exceptions::no_such_address();

	m_list.erase(it);
}


void addressList::removeAddress(const int pos)
{
	const std::vector <std::shared_ptr<address> >::iterator it = m_list.begin() + pos;

	m_list.erase(it);
}


void addressList::removeAllAddresses()
{
	m_list.clear();
}


size_t addressList::getAddressCount() const
{
	return (m_list.size());
}


bool addressList::isEmpty() const
{
	return (m_list.empty());
}


std::shared_ptr<address> addressList::getAddressAt(const int pos)
{
	return (m_list[pos]);
}


const std::shared_ptr<const address> addressList::getAddressAt(const int pos) const
{
	return (m_list[pos]);
}


const std::vector <std::shared_ptr<const address> > addressList::getAddressList() const
{
	std::vector <std::shared_ptr<const address> > list;

	list.reserve(m_list.size());

	for (std::vector <std::shared_ptr<address> >::const_iterator it = m_list.begin() ;
	     it != m_list.end() ; ++it)
	{
		list.push_back(*it);
	}

	return (list);
}


const std::vector <std::shared_ptr<address> > addressList::getAddressList()
{
	return (m_list);
}


const std::vector <std::shared_ptr<component> > addressList::getChildComponents()
{
	std::vector <std::shared_ptr<component> > list;

	copy_vector(m_list, list);

	return (list);
}


std::shared_ptr<mailboxList> addressList::toMailboxList() const
{
	std::shared_ptr<mailboxList> res = std::make_shared<mailboxList>();

	for (std::vector <std::shared_ptr<address> >::const_iterator it = m_list.begin() ;
	     it != m_list.end() ; ++it)
	{
		std::shared_ptr<const address> addr = *it;

		if (addr->isGroup())
		{
			//const std::vector <std::shared_ptr<const mailbox> > mailboxes = TODO std::shared_ptr
			//	addr.dynamicCast <const mailboxGroup>()->getMailboxList();
			const std::vector <std::shared_ptr<const mailbox> > mailboxes = 
				std::dynamic_pointer_cast<const mailboxGroup>(addr)->getMailboxList();

			for (std::vector <std::shared_ptr<const mailbox> >::const_iterator jt = mailboxes.begin() ;
			     jt != mailboxes.end() ; ++jt)
			{
				//res->appendMailbox(vmime::clone(*jt)); TODO shared_ptr
				res->appendMailbox(std::dynamic_pointer_cast<mailbox>((*jt)->clone()));
				//res->appendMailbox(vmime::clone(*jt));
			}
		}
		else
		{
			//res->appendMailbox(addr->clone().dynamicCast <mailbox>()); TODO: check std::shared_ptr
			res->appendMailbox(std::dynamic_pointer_cast<mailbox>(addr->clone()));
		}
	}

	return res;
}


} // vmime
