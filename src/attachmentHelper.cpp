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

#include "vmime/attachmentHelper.hpp"

#include "vmime/bodyPartAttachment.hpp"
#include "vmime/parsedMessageAttachment.hpp"
#include "vmime/generatedMessageAttachment.hpp"

#include "vmime/disposition.hpp"
#include "vmime/emptyContentHandler.hpp"
#include "vmime/factory.hpp"


namespace vmime
{


// static
bool attachmentHelper::isBodyPartAnAttachment
	(std::shared_ptr<const bodyPart> part, const unsigned int options)
{
	try
	{
		const contentDispositionField& cdf = dynamic_cast<contentDispositionField&>
			(*part->getHeader()->findField(fields::CONTENT_DISPOSITION));

		//const contentDisposition disp = *cdf.getValue()
		//	.dynamicCast <const contentDisposition>(); TODO shared_ptr
		const contentDisposition disp = *dynamic_cast<const
			contentDisposition*>(cdf.getValue().get());

		if (disp.getName() != contentDispositionTypes::INLINE)
			return true;

		if ((options & INLINE_OBJECTS) == 0)
		{
			// If the Content-Disposition is 'inline' and there is no
			// Content-Id or Content-Location field, it may be an attachment
			if (!part->getHeader()->hasField(vmime::fields::CONTENT_ID) &&
			    !part->getHeader()->hasField(vmime::fields::CONTENT_LOCATION))
			{
				// If this is the root part, it might not be an attachment
				if (part->getParentPart() == NULL)
					return false;

				return true;
			}

			return false;
		}
	}
	catch (exceptions::no_such_field&)
	{
		// Will try using Content-Type
	}

	// Assume "attachment" if type is not "text/..." or "multipart/...".
	mediaType type;

	try
	{
		const contentTypeField& ctf = dynamic_cast<contentTypeField&>
			(*part->getHeader()->findField(fields::CONTENT_TYPE));

		//type = *ctf.getValue().dynamicCast <const mediaType>(); TODO
		//shared_ptr
		type = *dynamic_cast<const mediaType*>(ctf.getValue().get());
	}
	catch (exceptions::no_such_field&)
	{
		// If this is the root part and no Content-Type field is present,
		// then this may not be a MIME message, so do not assume it is
		// an attachment
		if (part->getParentPart() == NULL)
			return false;

		// No "Content-type" field: assume "application/octet-stream".
		type = mediaType(mediaTypes::APPLICATION,
				     mediaTypes::APPLICATION_OCTET_STREAM);
	}

	if (type.getType() != mediaTypes::TEXT &&
	    type.getType() != mediaTypes::MULTIPART)
	{
		if ((options & INLINE_OBJECTS) == 0)
		{
			// If a "Content-Id" field is present, it might be an
			// embedded object (MHTML messages)
			if (part->getHeader()->hasField(vmime::fields::CONTENT_ID))
				return false;
		}

		return true;
	}

	return false;
}


// static
std::shared_ptr<const attachment> attachmentHelper::getBodyPartAttachment
	(std::shared_ptr<const bodyPart> part, const unsigned int options)
{
	if (!isBodyPartAnAttachment(part, options))
		return NULL;

	mediaType type;

	try
	{
		const contentTypeField& ctf = dynamic_cast<const contentTypeField&>
			(*part->getHeader()->findField(fields::CONTENT_TYPE));

		//type = *ctf.getValue().dynamicCast <const mediaType>(); TODO shared_ptr
		type = *dynamic_cast<const mediaType*>(ctf.getValue().get());
	}
	catch (exceptions::no_such_field&)
	{
		// No "Content-type" field: assume "application/octet-stream".
		type = mediaType(mediaTypes::APPLICATION,
		                 mediaTypes::APPLICATION_OCTET_STREAM);
	}

	if (type.getType() == mediaTypes::MESSAGE &&
	    type.getSubType() == mediaTypes::MESSAGE_RFC822)
	{
		return vmime::factory<generatedMessageAttachment>::create(part);
	}
	else
	{
		return std::dynamic_pointer_cast<const attachment>(
				vmime::factory<bodyPartAttachment>::create(part)
			);
	}
}


// static
const std::vector <std::shared_ptr<const attachment> >
	attachmentHelper::findAttachmentsInMessage
		(std::shared_ptr<const message> msg, const unsigned int options)
{
	return findAttachmentsInBodyPart(msg, options);
}


// static
const std::vector <std::shared_ptr<const attachment> >
	attachmentHelper::findAttachmentsInBodyPart
		(std::shared_ptr<const bodyPart> part, const unsigned int options)
{
	std::vector <std::shared_ptr<const attachment> > atts;

	// Test this part
	if (isBodyPartAnAttachment(part, options))
	{
		atts.push_back(getBodyPartAttachment(part, options));
	}
	// Find in sub-parts
	else
	{
		std::shared_ptr<const body> bdy = part->getBody();

		for (int i = 0 ; i < bdy->getPartCount() ; ++i)
		{
			std::vector <std::shared_ptr<const attachment> > partAtts =
				findAttachmentsInBodyPart(bdy->getPartAt(i), options);

			std::copy(partAtts.begin(), partAtts.end(), std::back_inserter(atts));
		}
	}

	return atts;
}


// static
void attachmentHelper::addAttachment(std::shared_ptr<message> msg, std::shared_ptr<attachment> att)
{
	// We simply search for a "multipart/mixed" part. If no one exists,
	// create it in the root part. This (very simple) algorithm should
	// work in the most cases.

	vmime::mediaType mpMixed(vmime::mediaTypes::MULTIPART,
	                         vmime::mediaTypes::MULTIPART_MIXED);

	std::shared_ptr<bodyPart> part = findBodyPart(msg, mpMixed);

	if (part == NULL)  // create it
	{
		if (msg->getBody()->getPartCount() != 0)
		{
			// Create a new container part for the parts that were in
			// the root part of the message
			std::shared_ptr<bodyPart> container =
				vmime::factory<bodyPart>::create();

			try
			{
				if (msg->getHeader()->hasField(fields::CONTENT_TYPE))
				{
					container->getHeader()->ContentType()->setValue
						(msg->getHeader()->ContentType()->getValue());
				}

				if (msg->getHeader()->hasField(fields::CONTENT_TRANSFER_ENCODING))
				{
					container->getHeader()->ContentTransferEncoding()->setValue
						(msg->getHeader()->ContentTransferEncoding()->getValue());
				}
			}
			catch (exceptions::no_such_field&)
			{
				// Ignore
			}

			// Move parts from the root part to this new part
			const std::vector <std::shared_ptr<bodyPart> > partList =
				msg->getBody()->getPartList();

			msg->getBody()->removeAllParts();

			for (unsigned int i = 0 ; i < partList.size() ; ++i)
				container->getBody()->appendPart(partList[i]);

			msg->getBody()->appendPart(container);
		}
		else
		{
			// The message is a simple (RFC-822) message, and do not
			// contains any MIME part. Move the contents from the
			// root to a new child part.
			std::shared_ptr<bodyPart> child = vmime::factory<bodyPart>::create();

			if (msg->getHeader()->hasField(fields::CONTENT_TYPE))
			{
				child->getHeader()->ContentType()->setValue
					(msg->getHeader()->ContentType()->getValue());
			}

			if (msg->getHeader()->hasField(fields::CONTENT_TRANSFER_ENCODING))
			{
				child->getHeader()->ContentTransferEncoding()->setValue
					(msg->getHeader()->ContentTransferEncoding()->getValue());
			}

			child->getBody()->setContents(msg->getBody()->getContents());
			msg->getBody()->setContents(vmime::factory<emptyContentHandler>::create());

			msg->getBody()->appendPart(child);
		}

		// Set the root part to 'multipart/mixed'
		msg->getHeader()->ContentType()->setValue(mpMixed);

		msg->getHeader()->removeAllFields(vmime::fields::CONTENT_DISPOSITION);
		msg->getHeader()->removeAllFields(vmime::fields::CONTENT_TRANSFER_ENCODING);

		part = msg;
	}

	// Generate the attachment part
	att->generateIn(part);
}


// static
std::shared_ptr<bodyPart> attachmentHelper::findBodyPart
	(std::shared_ptr<bodyPart> part, const mediaType& type)
{
	if (part->getBody()->getContentType() == type)
		return part;

	// Try in sub-parts
	std::shared_ptr<body> bdy = part->getBody();

	for (int i = 0 ; i < bdy->getPartCount() ; ++i)
	{
		std::shared_ptr<bodyPart> found =
			findBodyPart(bdy->getPartAt(i), type);

		if (found != NULL)
			return found;
	}

	return NULL;
}


// static
void attachmentHelper::addAttachment(std::shared_ptr<message> msg, std::shared_ptr<message> amsg)
{
	std::shared_ptr<attachment> att =
		vmime::factory<parsedMessageAttachment>::create(amsg);
	addAttachment(msg, att);
}


} // vmime

