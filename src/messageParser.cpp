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

#include "vmime/messageParser.hpp"

#include "vmime/attachmentHelper.hpp"

#include "vmime/defaultAttachment.hpp"
#include "vmime/textPartFactory.hpp"

#include "vmime/relay.hpp"
#include "vmime/contentTypeField.hpp"
#include "vmime/contentDispositionField.hpp"


namespace vmime
{


messageParser::messageParser(const string& buffer)
{
	std::shared_ptr<message> msg = std::make_shared<message>();
	msg->parse(buffer);

	parse(msg);
}


messageParser::messageParser(std::shared_ptr<const message> msg)
{
	parse(msg);
}


messageParser::~messageParser()
{
}


void messageParser::parse(std::shared_ptr<const message> msg)
{
	// Header fields (if field is present, copy its value, else do nothing)
#ifndef VMIME_BUILDING_DOC

	// TODO shared changed the macro a bit
#define TRY_FIELD(var, type, name) \
	try { var = *std::dynamic_pointer_cast<type>(msg->getHeader()->findField(name)->getValue()); } \
	catch (exceptions::no_such_field) { }

	TRY_FIELD(m_from, mailbox, fields::FROM);

	TRY_FIELD(m_to, addressList, fields::TO);
	TRY_FIELD(m_cc, addressList, fields::CC);
	TRY_FIELD(m_bcc, addressList, fields::BCC);

	TRY_FIELD(m_subject, text, fields::SUBJECT);

#undef TRY_FIELD

#endif // VMIME_BUILDING_DOC

	// Date
	try
	{
		const headerField& recv = *msg->getHeader()->findField(fields::RECEIVED);
		// m_date = recv.getValue().dynamicCast <const relay>()->getDate();
		// TODO shared
		m_date = std::dynamic_pointer_cast<const relay>(recv.getValue())->getDate();
	}
	catch (vmime::exceptions::no_such_field&)
	{
		try
		{
			const headerField& date = *msg->getHeader()->findField(fields::DATE);
			// m_date = *date.getValue().dynamicCast <const datetime>(); TODO
			// shared
			m_date = *std::dynamic_pointer_cast<const datetime>(date.getValue());
		}
		catch (vmime::exceptions::no_such_field&)
		{
			m_date = datetime::now();
		}
	}

	// Attachments
	findAttachments(msg);

	// Text parts
	findTextParts(msg, msg);
}


void messageParser::findAttachments(std::shared_ptr<const message> msg)
{
	m_attach = attachmentHelper::findAttachmentsInMessage(msg);
}


void messageParser::findTextParts(std::shared_ptr<const bodyPart> msg, std::shared_ptr<const bodyPart> part)
{
	// Handle the case in which the message is not multipart: if the body part is
	// "text/*", take this part.
	if (part->getBody()->getPartCount() == 0)
	{
		mediaType type(mediaTypes::TEXT, mediaTypes::TEXT_PLAIN);
		bool accept = false;

		try
		{
			const contentTypeField& ctf = dynamic_cast<contentTypeField&>
				(*msg->getHeader()->findField(fields::CONTENT_TYPE));

			const mediaType ctfType =
				// *ctf.getValue().dynamicCast <const mediaType>(); TODO
				// shared
				*std::dynamic_pointer_cast<const mediaType>(ctf.getValue());

			if (ctfType.getType() == mediaTypes::TEXT)
			{
				type = ctfType;
				accept = true;
			}
		}
		catch (exceptions::no_such_field&)
		{
			// No "Content-type" field: assume "text/plain".
			accept = true;
		}

		if (accept)
		{
			std::shared_ptr<textPart> txtPart = textPartFactory::getInstance()->create(type);
			txtPart->parse(msg, msg, msg);

			m_textParts.push_back(txtPart);
		}
	}
	// Multipart message
	else
	{
		findSubTextParts(msg, part);
	}
}


bool messageParser::findSubTextParts(std::shared_ptr<const bodyPart> msg, std::shared_ptr<const bodyPart> part)
{
	// In general, all the text parts are contained in parallel in the same
	// parent part (or message).
	// So, wherever the text parts are, all we have to do is to find the first
	// MIME part which is a text part.

	std::vector <std::shared_ptr<const bodyPart> > textParts;

	for (int i = 0 ; i < part->getBody()->getPartCount() ; ++i)
	{
		const std::shared_ptr<const bodyPart> p = part->getBody()->getPartAt(i);

		try
		{
			const contentTypeField& ctf = dynamic_cast <const contentTypeField&>
				(*(p->getHeader()->findField(fields::CONTENT_TYPE)));

			// const mediaType type = *ctf.getValue().dynamicCast <const
			// mediaType>(); TODO shared
			const mediaType type = *std::dynamic_pointer_cast<const
				mediaType>(ctf.getValue());
			contentDisposition disp; // default should be inline

			if (type.getType() == mediaTypes::TEXT)
			{
				try
				{
					//std::shared_ptr<const contentDispositionField> cdf = p->getHeader()->
						// findField(fields::CONTENT_DISPOSITION).dynamicCast
						// <const contentDispositionField>(); TODO shared
					std::shared_ptr<const contentDispositionField> cdf = 
						std::dynamic_pointer_cast<const contentDispositionField>(
							p->getHeader()->findField(fields::CONTENT_DISPOSITION)
						);

					// disp = *cdf->getValue().dynamicCast <const
					// contentDisposition>(); TODO shared
					disp = *std::dynamic_pointer_cast<const
						contentDisposition>(cdf->getValue());
				}
				catch (exceptions::no_such_field&)
				{
					// No "Content-Disposition" field, assume default
				}

				if (disp.getName() == contentDispositionTypes::INLINE)
					textParts.push_back(p);
			}
		}
		catch (exceptions::no_such_field&)
		{
			// No "Content-type" field.
		}
	}

	if (textParts.size())
	{
		// Okay. So we have found at least one text part
		for (std::vector <std::shared_ptr<const bodyPart> >::const_iterator p = textParts.begin() ;
		     p != textParts.end() ; ++p)
		{
			const contentTypeField& ctf = dynamic_cast <const contentTypeField&>
				(*((*p)->getHeader()->findField(fields::CONTENT_TYPE)));

			// const mediaType type = *ctf.getValue().dynamicCast <const
			// mediaType>(); TODO shared
			const mediaType type = *std::dynamic_pointer_cast<const
				mediaType>(ctf.getValue());

			try
			{
				std::shared_ptr<textPart> txtPart = textPartFactory::getInstance()->create(type);
				txtPart->parse(msg, part, *p);

				m_textParts.push_back(txtPart);
			}
			catch (exceptions::no_factory_available& e)
			{
				// Content-type not recognized.
			}
		}
	}

	bool found = false;

	for (int i = 0 ; !found && (i < part->getBody()->getPartCount()) ; ++i)
	{
		found = findSubTextParts(msg, part->getBody()->getPartAt(i));
	}

	return found;
}


const mailbox& messageParser::getExpeditor() const
{
	return (m_from);
}


const addressList& messageParser::getRecipients() const
{
	return (m_to);
}


const addressList& messageParser::getCopyRecipients() const
{
	return (m_cc);
}


const addressList& messageParser::getBlindCopyRecipients() const
{
	return (m_bcc);
}


const text& messageParser::getSubject() const
{
	return (m_subject);
}


const datetime& messageParser::getDate() const
{
	return (m_date);
}


const std::vector <std::shared_ptr<const attachment> > messageParser::getAttachmentList() const
{
	return m_attach;
}


int messageParser::getAttachmentCount() const
{
	return (m_attach.size());
}


const std::shared_ptr<const attachment> messageParser::getAttachmentAt(const int pos) const
{
	return (m_attach[pos]);
}


const std::vector <std::shared_ptr<const textPart> > messageParser::getTextPartList() const
{
	std::vector <std::shared_ptr<const textPart> > res;

	res.reserve(m_textParts.size());

	for (std::vector <std::shared_ptr<textPart> >::const_iterator it = m_textParts.begin() ;
	     it != m_textParts.end() ; ++it)
	{
		res.push_back(*it);
	}

	return (res);
}


int messageParser::getTextPartCount() const
{
	return (m_textParts.size());
}


const std::shared_ptr<const textPart> messageParser::getTextPartAt(const int pos) const
{
	return (m_textParts[pos]);
}


} // vmime
