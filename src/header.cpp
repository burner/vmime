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

#include "vmime/header.hpp"
#include "vmime/parserHelpers.hpp"

#include <algorithm>
#include <assert.h>


namespace vmime
{


header::header()
{
}


header::~header()
{
	removeAllFields();
}


/*

 RFC #822:
 3.2.  HEADER FIELD DEFINITIONS

field       =  field-name ":" [ field-body ] CRLF

field-name  =  1*<any CHAR, excluding CTLs, SPACE, and ":">

field-body  =  field-body-contents
		[CRLF LWSP-char field-body]

field-body-contents =
		<the ASCII characters making up the field-body, as
		 defined in the following sections, and consisting
		 of combinations of atom, quoted-string, and
		 specials tokens, or else consisting of texts>
*/

void header::parseImpl(const string& buffer, const string::size_type position,
	const string::size_type end, string::size_type* newPosition)
{
	string::size_type pos = position;

	removeAllFields();

	while (pos < end)
	{
		std::shared_ptr<headerField> field = headerField::parseNext(buffer, pos, end, &pos);
		if (field == NULL) break;

		m_fields.push_back(field);
	}

	setParsedBounds(position, pos);

	if (newPosition)
		*newPosition = pos;
}


void header::generateImpl(utility::outputStream& os, const string::size_type maxLineLength,
	const string::size_type /* curLinePos */, string::size_type* newLinePos) const
{
	// Generate the fields
	for (std::vector <std::shared_ptr<headerField> >::const_iterator it = m_fields.begin() ;
	     it != m_fields.end() ; ++it)
	{
		(*it)->generate(os, maxLineLength);
		os << CRLF;
	}

	if (newLinePos)
		*newLinePos = 0;
}


std::shared_ptr<component> header::clone() const
{
	std::shared_ptr<header> hdr = vmime::factory<header>::create();

	hdr->m_fields.reserve(m_fields.size());

	for (std::vector <std::shared_ptr<headerField> >::const_iterator it = m_fields.begin() ;
	     it != m_fields.end() ; ++it)
	{
		// hdr->m_fields.push_back((*it)->clone().dynamicCast
		// <headerField>()); TODO shared
		hdr->m_fields.push_back(std::dynamic_pointer_cast<headerField>((*it)->clone()));
	}

	return (hdr);
}


void header::copyFrom(const component& other)
{
	const header& h = dynamic_cast <const header&>(other);

	std::vector <std::shared_ptr<headerField> > fields;

	fields.reserve(h.m_fields.size());

	for (std::vector <std::shared_ptr<headerField> >::const_iterator it = h.m_fields.begin() ;
	     it != h.m_fields.end() ; ++it)
	{
		// fields.push_back((*it)->clone().dynamicCast <headerField>()); TODO
		// shared
		fields.push_back(std::dynamic_pointer_cast<headerField>((*it)->clone()));
	}

	m_fields.clear();
	m_fields.resize(fields.size());

	std::copy(fields.begin(), fields.end(), m_fields.begin());
}


header& header::operator=(const header& other)
{
	copyFrom(other);
	return (*this);
}


bool header::hasField(const string& fieldName) const
{
	std::vector <std::shared_ptr<headerField> >::const_iterator pos =
		std::find_if(m_fields.begin(), m_fields.end(),
		             fieldHasName(utility::stringUtils::toLower(fieldName)));

	return (pos != m_fields.end());
}


std::shared_ptr<headerField> header::findField(const string& fieldName) const
{
	// Find the first field that matches the specified name
	std::vector <std::shared_ptr<headerField> >::const_iterator pos =
		std::find_if(m_fields.begin(), m_fields.end(),
		             fieldHasName(utility::stringUtils::toLower(fieldName)));

	// No field with this name can be found
	if (pos == m_fields.end())
	{
		throw exceptions::no_such_field();
	}
	// Else, return a reference to the existing field
	else
	{
		return (*pos);
	}
}


std::vector <std::shared_ptr<headerField> > header::findAllFields(const string& fieldName)
{
	std::vector <std::shared_ptr<headerField> > result;
	std::back_insert_iterator <std::vector <std::shared_ptr<headerField> > > back(result);

	std::remove_copy_if(m_fields.begin(), m_fields.end(), back,
	                    fieldHasNotName(utility::stringUtils::toLower(fieldName)));

	return result;
}


std::shared_ptr<headerField> header::getField(const string& fieldName)
{
	const string name = utility::stringUtils::toLower(fieldName);

	// Find the first field that matches the specified name
	std::vector <std::shared_ptr<headerField> >::const_iterator pos = m_fields.begin();
	const std::vector <std::shared_ptr<headerField> >::const_iterator end = m_fields.end();

	while (pos != end && utility::stringUtils::toLower((*pos)->getName()) != name)
		++pos;

	// If no field with this name can be found, create a new one
	if (pos == end)
	{
		std::shared_ptr<headerField> field = headerFieldFactory::getInstance()->create(fieldName);

		appendField(field);

		// Return a reference to the new field
		return (field);
	}
	// Else, return a reference to the existing field
	else
	{
		return (*pos);
	}
}


void header::appendField(std::shared_ptr<headerField> field)
{
	m_fields.push_back(field);
}


void header::insertFieldBefore(std::shared_ptr<headerField> beforeField, std::shared_ptr<headerField> field)
{
	const std::vector <std::shared_ptr<headerField> >::iterator it = std::find
		(m_fields.begin(), m_fields.end(), beforeField);

	if (it == m_fields.end())
		throw exceptions::no_such_field();

	m_fields.insert(it, field);
}


void header::insertFieldBefore(const int pos, std::shared_ptr<headerField> field)
{
	m_fields.insert(m_fields.begin() + pos, field);
}


void header::insertFieldAfter(std::shared_ptr<headerField> afterField, std::shared_ptr<headerField> field)
{
	const std::vector <std::shared_ptr<headerField> >::iterator it = std::find
		(m_fields.begin(), m_fields.end(), afterField);

	if (it == m_fields.end())
		throw exceptions::no_such_field();

	m_fields.insert(it + 1, field);
}


void header::insertFieldAfter(const int pos, std::shared_ptr<headerField> field)
{
	m_fields.insert(m_fields.begin() + pos + 1, field);
}


void header::removeField(std::shared_ptr<headerField> field)
{
	const std::vector <std::shared_ptr<headerField> >::iterator it = std::find
		(m_fields.begin(), m_fields.end(), field);

	if (it == m_fields.end())
		throw exceptions::no_such_field();

	m_fields.erase(it);
}


void header::removeField(const int pos)
{
	const std::vector <std::shared_ptr<headerField> >::iterator it = m_fields.begin() + pos;

	m_fields.erase(it);
}


void header::removeAllFields()
{
	m_fields.clear();
}


void header::removeAllFields(const string& fieldName)
{
	std::vector <std::shared_ptr<headerField> > fields = findAllFields(fieldName);

	for (unsigned int i = 0 ; i < fields.size() ; ++i)
		removeField(fields[i]);
}


size_t header::getFieldCount() const
{
	return (m_fields.size());
}


bool header::isEmpty() const
{
	return (m_fields.empty());
}


const std::shared_ptr<headerField> header::getFieldAt(const int pos)
{
	return (m_fields[pos]);
}


const std::shared_ptr<const headerField> header::getFieldAt(const int pos) const
{
	return (m_fields[pos]);
}


const std::vector <std::shared_ptr<const headerField> > header::getFieldList() const
{
	std::vector <std::shared_ptr<const headerField> > list;

	list.reserve(m_fields.size());

	for (std::vector <std::shared_ptr<headerField> >::const_iterator it = m_fields.begin() ;
	     it != m_fields.end() ; ++it)
	{
		list.push_back(*it);
	}

	return (list);
}


const std::vector <std::shared_ptr<headerField> > header::getFieldList()
{
	return (m_fields);
}


const std::vector <std::shared_ptr<component> > header::getChildComponents()
{
	std::vector <std::shared_ptr<component> > list;

	copy_vector(m_fields, list);

	return (list);
}



// Field search


header::fieldHasName::fieldHasName(const string& name)
	: m_name(name)
{
}

bool header::fieldHasName::operator() (const std::shared_ptr<const headerField>& field)
{
	assert(field);
	return utility::stringUtils::toLower(field->getName()) == m_name;
}


header::fieldHasNotName::fieldHasNotName(const string& name)
	: m_name(name)
{
}

bool header::fieldHasNotName::operator() (const std::shared_ptr<const headerField>& field)
{
	return utility::stringUtils::toLower(field->getName()) != m_name;
}


} // vmime
