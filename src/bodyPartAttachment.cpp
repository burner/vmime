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

#include "vmime/bodyPartAttachment.hpp"


namespace vmime
{


bodyPartAttachment::bodyPartAttachment(std::shared_ptr<const bodyPart> part)
	: m_part(part)
{
}


const mediaType bodyPartAttachment::getType() const
{
	mediaType type;

	try
	{
		//type = *getContentType()->getValue().dynamicCast <const
		//mediaType>(); TODO
		type = *std::dynamic_pointer_cast<const mediaType>(getContentType()->getValue());
	}
	catch (exceptions::no_such_field&)
	{
		// No "Content-type" field: assume "application/octet-stream".
		type = mediaType(mediaTypes::APPLICATION,
				 mediaTypes::APPLICATION_OCTET_STREAM);
	}

	return type;
}


const word bodyPartAttachment::getName() const
{
	word name;

	// Try the 'filename' parameter of 'Content-Disposition' field
	try
	{
		name = getContentDisposition()->getFilename();
	}
	catch (exceptions::no_such_field&)
	{
		// No 'Content-Disposition' field
	}
	catch (exceptions::no_such_parameter&)
	{
		// No 'filename' parameter
	}

	// Try the 'name' parameter of 'Content-Type' field
	if (name.getBuffer().empty())
	{
		try
		{
			std::shared_ptr<parameter> prm = getContentType()->findParameter("name");

			if (prm != NULL)
				name = prm->getValue();
		}
		catch (exceptions::no_such_field&)
		{
			// No 'Content-Type' field
		}
		catch (exceptions::no_such_parameter&)
		{
			// No attachment name available
		}
	}

	return name;
}


const text bodyPartAttachment::getDescription() const
{
	text description;

	try
	{
		std::shared_ptr<const headerField> cd =
			getHeader()->findField(fields::CONTENT_DESCRIPTION);

		//description = *cd->getValue().dynamicCast <const text>(); TODO
		description = *std::dynamic_pointer_cast<const text>(cd->getValue());
	}
	catch (exceptions::no_such_field&)
	{
		// No description available.
	}

	return description;
}


const encoding bodyPartAttachment::getEncoding() const
{
	return m_part->getBody()->getEncoding();
}


const std::shared_ptr<const contentHandler> bodyPartAttachment::getData() const
{
	return m_part->getBody()->getContents();
}


std::shared_ptr<const object> bodyPartAttachment::getPart() const
{
	return m_part;
}


std::shared_ptr<const header> bodyPartAttachment::getHeader() const
{
	return m_part->getHeader();
}


std::shared_ptr<const contentDispositionField> bodyPartAttachment::getContentDisposition() const
{
	return std::dynamic_pointer_cast<const contentDispositionField>(getHeader()->
			findField(fields::CONTENT_DISPOSITION));
}


std::shared_ptr<const contentTypeField> bodyPartAttachment::getContentType() const
{
	return std::dynamic_pointer_cast<const
		contentTypeField>(getHeader()->findField(fields::CONTENT_TYPE));
}


void bodyPartAttachment::generateIn(std::shared_ptr<bodyPart> /* parent */) const
{
	// Not used
}


} // vmime

