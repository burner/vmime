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

#include "vmime/messageIdSequence.hpp"
#include "vmime/exception.hpp"
#include <algorithm>


namespace vmime
{




messageIdSequence::messageIdSequence()
{
}


messageIdSequence::~messageIdSequence()
{
	removeAllMessageIds();
}


messageIdSequence::messageIdSequence(const messageIdSequence& midSeq)
	: headerFieldValue()
{
	copyFrom(midSeq);
}


std::shared_ptr<component> messageIdSequence::clone() const
{
	return vmime::factory<messageIdSequence>::create(*this);
}


void messageIdSequence::copyFrom(const component& other)
{
	const messageIdSequence& midSeq = dynamic_cast <const messageIdSequence&>(other);

	removeAllMessageIds();

	for (unsigned int i = 0 ; i < midSeq.m_list.size() ; ++i) {
		// m_list.push_back(midSeq.m_list[i]->clone().dynamicCast
		// <messageId>()); TODO shared
		m_list.push_back(std::dynamic_pointer_cast<messageId>(midSeq.m_list[i]->clone()));
	}
}


messageIdSequence& messageIdSequence::operator=(const messageIdSequence& other)
{
	copyFrom(other);
	return (*this);
}


const std::vector <std::shared_ptr<component> > messageIdSequence::getChildComponents()
{
	std::vector <std::shared_ptr<component> > res;

	copy_vector(m_list, res);

	return (res);
}


void messageIdSequence::parseImpl(const string& buffer, const string::size_type position,
	const string::size_type end, string::size_type* newPosition)
{
	removeAllMessageIds();

	string::size_type pos = position;

	while (pos < end)
	{
		std::shared_ptr<messageId> parsedMid = messageId::parseNext(buffer, pos, end, &pos);

		if (parsedMid != NULL)
			m_list.push_back(parsedMid);
	}

	setParsedBounds(position, end);

	if (newPosition)
		*newPosition = end;
}


void messageIdSequence::generateImpl(utility::outputStream& os, const string::size_type maxLineLength,
	const string::size_type curLinePos, string::size_type* newLinePos) const
{
	string::size_type pos = curLinePos;

	if (!m_list.empty())
	{
		for (std::vector <std::shared_ptr<messageId> >::const_iterator it = m_list.begin() ; ; )
		{
			(*it)->generate(os, maxLineLength - 2, pos, &pos);

			if (++it == m_list.end())
				break;

			os << " ";
			pos++;
		}
	}

	if (newLinePos)
		*newLinePos = pos;
}


void messageIdSequence::appendMessageId(std::shared_ptr<messageId> mid)
{
	m_list.push_back(mid);
}


void messageIdSequence::insertMessageIdBefore(std::shared_ptr<messageId> beforeMid, std::shared_ptr<messageId> mid)
{
	const std::vector <std::shared_ptr<messageId> >::iterator it = std::find
		(m_list.begin(), m_list.end(), beforeMid);

	if (it == m_list.end())
		throw exceptions::no_such_message_id();

	m_list.insert(it, mid);
}


void messageIdSequence::insertMessageIdBefore(const int pos, std::shared_ptr<messageId> mid)
{
	m_list.insert(m_list.begin() + pos, mid);
}


void messageIdSequence::insertMessageIdAfter(std::shared_ptr<messageId> afterMid, std::shared_ptr<messageId> mid)
{
	const std::vector <std::shared_ptr<messageId> >::iterator it = std::find
		(m_list.begin(), m_list.end(), afterMid);

	if (it == m_list.end())
		throw exceptions::no_such_message_id();

	m_list.insert(it + 1, mid);
}


void messageIdSequence::insertMessageIdAfter(const int pos, std::shared_ptr<messageId> mid)
{
	m_list.insert(m_list.begin() + pos + 1, mid);
}


void messageIdSequence::removeMessageId(std::shared_ptr<messageId> mid)
{
	const std::vector <std::shared_ptr<messageId> >::iterator it = std::find
		(m_list.begin(), m_list.end(), mid);

	if (it == m_list.end())
		throw exceptions::no_such_message_id();

	m_list.erase(it);
}


void messageIdSequence::removeMessageId(const int pos)
{
	const std::vector <std::shared_ptr<messageId> >::iterator it = m_list.begin() + pos;

	m_list.erase(it);
}


void messageIdSequence::removeAllMessageIds()
{
	m_list.clear();
}


int messageIdSequence::getMessageIdCount() const
{
	return (m_list.size());
}


bool messageIdSequence::isEmpty() const
{
	return (m_list.empty());
}


const std::shared_ptr<messageId> messageIdSequence::getMessageIdAt(const int pos)
{
	return (m_list[pos]);
}


const std::shared_ptr<const messageId> messageIdSequence::getMessageIdAt(const int pos) const
{
	return (m_list[pos]);
}


const std::vector <std::shared_ptr<const messageId> > messageIdSequence::getMessageIdList() const
{
	std::vector <std::shared_ptr<const messageId> > list;

	list.reserve(m_list.size());

	for (std::vector <std::shared_ptr<messageId> >::const_iterator it = m_list.begin() ;
	     it != m_list.end() ; ++it)
	{
		list.push_back(*it);
	}

	return (list);
}


const std::vector <std::shared_ptr<messageId> > messageIdSequence::getMessageIdList()
{
	return (m_list);
}


} // vmime
