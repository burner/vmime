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

#include "vmime/bodyPart.hpp"


namespace vmime
{


bodyPart::bodyPart()
	: m_header(vmime::factory<header>::create()),
	  m_body(vmime::factory<body>::create())
	  //, m_parent(NULL) TODO shared
{
	//m_body->setParentPart(thisRef().dynamicCast <bodyPart>()); TODO shared
	m_body->setParentPart(std::dynamic_pointer_cast<bodyPart>(thisRef()));
}


bodyPart::bodyPart(std::weak_ptr<vmime::bodyPart> parentPart)
	: m_header(vmime::factory<header>::create()),
	  m_body(vmime::factory<body>::create()),
	  m_parent(parentPart)
{
	// m_body->setParentPart(thisRef().dynamicCast <bodyPart>()); TODO shared
	m_body->setParentPart(std::dynamic_pointer_cast<bodyPart>(thisRef()));
}


void bodyPart::parseImpl
	(std::shared_ptr<utility::parserInputStreamAdapter> parser,
	 const utility::stream::size_type position,
	 const utility::stream::size_type end,
	 utility::stream::size_type* newPosition)
{
	// Parse the headers
	string::size_type pos = position;
	m_header->parse(parser, pos, end, &pos);

	// Parse the body contents
	m_body->parse(parser, pos, end, NULL);

	setParsedBounds(position, end);

	if (newPosition)
		*newPosition = end;
}


void bodyPart::generateImpl(utility::outputStream& os, const string::size_type maxLineLength,
	const string::size_type /* curLinePos */, string::size_type* newLinePos) const
{
	m_header->generate(os, maxLineLength);

	os << CRLF;

	m_body->generate(os, maxLineLength);

	if (newLinePos)
		*newLinePos = 0;
}


std::shared_ptr<component> bodyPart::clone() const
{
	std::shared_ptr<bodyPart> p = vmime::factory<bodyPart>::create();

	//p->m_parent = null; TODO shared

	p->m_header->copyFrom(*m_header);
	p->m_body->copyFrom(*m_body);

	return (p);
}


void bodyPart::copyFrom(const component& other)
{
	const bodyPart& bp = dynamic_cast <const bodyPart&>(other);

	m_header->copyFrom(*(bp.m_header));
	m_body->copyFrom(*(bp.m_body));
}


bodyPart& bodyPart::operator=(const bodyPart& other)
{
	copyFrom(other);
	return (*this);
}


const std::shared_ptr<const header> bodyPart::getHeader() const
{
	return (m_header);
}


std::shared_ptr<header> bodyPart::getHeader()
{
	return (m_header);
}


const std::shared_ptr<const body> bodyPart::getBody() const
{
	return (m_body);
}


std::shared_ptr<body> bodyPart::getBody()
{
	return (m_body);
}


std::shared_ptr<bodyPart> bodyPart::getParentPart()
{
	//return m_parent.acquire(); TODO shared
	return m_parent.lock();
}


std::shared_ptr<const bodyPart> bodyPart::getParentPart() const
{
	// return m_parent.acquire(); TODO shared
	return m_parent.lock();
}


const std::vector <std::shared_ptr<component> > bodyPart::getChildComponents()
{
	std::vector <std::shared_ptr<component> > list;

	list.push_back(m_header);
	list.push_back(m_body);

	return (list);
}


} // vmime

