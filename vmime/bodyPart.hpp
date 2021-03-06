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

#ifndef VMIME_BODYPART_HPP_INCLUDED
#define VMIME_BODYPART_HPP_INCLUDED


#include "vmime/base.hpp"
#include "vmime/component.hpp"

#include "vmime/header.hpp"
#include "vmime/body.hpp"


namespace vmime
{


/** A MIME part.
  */

class bodyPart : public component
{
	friend class body;

public:

	bodyPart();
	bodyPart(std::weak_ptr<vmime::bodyPart> parentPart);

	void initAfterCreate();

	/** Return the header section of this part.
	  *
	  * @return header section
	  */
	const std::shared_ptr<const header> getHeader() const;

	/** Return the header section of this part.
	  *
	  * @return header section
	  */
	std::shared_ptr<header> getHeader();

	/** Return the body section of this part.
	  *
	  * @return body section
	  */
	const std::shared_ptr<const body> getBody() const;

	/** Return the body section of this part.
	  *
	  * @return body section
	  */
	std::shared_ptr<body> getBody();

	/** Return the parent part of this part.
	  *
	  * @return parent part or NULL if not known
	  */
	std::shared_ptr<bodyPart> getParentPart();

	/** Return the parent part of this part (const version).
	  *
	  * @return parent part or NULL if not known
	  */
	std::shared_ptr<const bodyPart> getParentPart() const;


	std::shared_ptr<component> clone() const;
	void copyFrom(const component& other);
	bodyPart& operator=(const bodyPart& other);

	const std::vector <std::shared_ptr<component> > getChildComponents();

private:

	std::shared_ptr<header> m_header;
	std::shared_ptr<body> m_body;

	std::weak_ptr<bodyPart> m_parent;

protected:

	// Component parsing & assembling
	void parseImpl
		(std::shared_ptr<utility::parserInputStreamAdapter> parser,
		 const utility::stream::size_type position,
		 const utility::stream::size_type end,
		 utility::stream::size_type* newPosition = NULL);

	void generateImpl
		(utility::outputStream& os,
		 const string::size_type maxLineLength = lineLengthLimits::infinite,
		 const string::size_type curLinePos = 0,
		 string::size_type* newLinePos = NULL) const;
};


} // vmime


#endif // VMIME_BODYPART_HPP_INCLUDED
