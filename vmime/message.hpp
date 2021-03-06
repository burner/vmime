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

#ifndef VMIME_MESSAGE_HPP_INCLUDED
#define VMIME_MESSAGE_HPP_INCLUDED


#include "vmime/bodyPart.hpp"
#include "vmime/options.hpp"


namespace vmime
{


/** A MIME message.
  */

class message : public bodyPart
{
public:

	message();
	void initAfterCreate();

public:

	// Override default generate() functions so that we can change
	// the default 'maxLineLength' value
	void generate
		(utility::outputStream& os,
		 const string::size_type maxLineLength = options::getInstance()->message.maxLineLength(),
		 const string::size_type curLinePos = 0,
		 string::size_type* newLinePos = NULL) const;

	const string generate
		(const string::size_type maxLineLength = options::getInstance()->message.maxLineLength(),
		 const string::size_type curLinePos = 0) const;

	void generate
		(std::shared_ptr<utility::outputStream> os,
		 const string::size_type maxLineLength = lineLengthLimits::infinite,
		 const string::size_type curLinePos = 0,
		 string::size_type* newLinePos = NULL) const;
};



} // vmime


#endif // VMIME_MESSAGE_HPP_INCLUDED
