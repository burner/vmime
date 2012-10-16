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

#ifndef VMIME_BODYPARTATTACHMENT_HPP_INCLUDED
#define VMIME_BODYPARTATTACHMENT_HPP_INCLUDED


#ifndef VMIME_BUILDING_DOC  // implementation detail


#include "vmime/attachment.hpp"

#include "vmime/contentDispositionField.hpp"
#include "vmime/contentTypeField.hpp"


namespace vmime
{


/** An attachment related to a local body part.
  */
class bodyPartAttachment : public attachment
{
	friend class creator;

//protected: TODO shared_ptr constructor was protected


public:
	bodyPartAttachment(std::shared_ptr<const bodyPart> part); // TODO shared

	const mediaType getType() const;
	const word getName() const;
	const text getDescription() const;
	const encoding getEncoding() const;

	const std::shared_ptr<const contentHandler> getData() const;

	std::shared_ptr<const object> getPart() const;
	std::shared_ptr<const header> getHeader() const;

private:

	void generateIn(std::shared_ptr<bodyPart> parent) const;

	std::shared_ptr<const contentDispositionField> getContentDisposition() const;
	std::shared_ptr<const contentTypeField> getContentType() const;


	std::shared_ptr<const bodyPart> m_part;
};


} // vmime


#endif // VMIME_BUILDING_DOC


#endif // VMIME_BODYPARTATTACHMENT_HPP_INCLUDED

