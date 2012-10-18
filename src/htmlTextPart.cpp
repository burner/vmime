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

#include "vmime/htmlTextPart.hpp"
#include "vmime/exception.hpp"

#include "vmime/contentTypeField.hpp"
#include "vmime/contentDisposition.hpp"
#include "vmime/text.hpp"

#include "vmime/emptyContentHandler.hpp"
#include "vmime/stringContentHandler.hpp"

#include "vmime/utility/outputStreamAdapter.hpp"


namespace vmime
{


htmlTextPart::htmlTextPart()
	: m_plainText(std::make_shared<emptyContentHandler>()),
	  m_text(std::make_shared<emptyContentHandler>())
{
}


htmlTextPart::~htmlTextPart()
{
}


const mediaType htmlTextPart::getType() const
{
	return mediaType(mediaTypes::TEXT, mediaTypes::TEXT_HTML);
}


int htmlTextPart::getPartCount() const
{
	return (m_plainText->isEmpty() ? 1 : 2);
}


void htmlTextPart::generateIn(std::shared_ptr<bodyPart> /* message */, std::shared_ptr<bodyPart> parent) const
{
	// Plain text
	if (!m_plainText->isEmpty())
	{
		// -- Create a new part
		// std::shared_ptr<bodyPart> part = std::make_shared<bodyPart>(); TODO
		// shared
		std::shared_ptr<bodyPart> part = bodyPart::construct();
		parent->getBody()->appendPart(part);

		// -- Set contents
		part->getBody()->setContents(m_plainText,
			mediaType(mediaTypes::TEXT, mediaTypes::TEXT_PLAIN), m_charset,
			encoding::decide(m_plainText, m_charset, encoding::USAGE_TEXT));
	}

	// HTML text
	// -- Create a new part
	// std::shared_ptr<bodyPart> htmlPart = std::make_shared<bodyPart>(); TODO
	// shared
	std::shared_ptr<bodyPart> htmlPart = bodyPart::construct();

	// -- Set contents
	htmlPart->getBody()->setContents(m_text,
		mediaType(mediaTypes::TEXT, mediaTypes::TEXT_HTML), m_charset,
		encoding::decide(m_text, m_charset, encoding::USAGE_TEXT));

	// Handle the case we have embedded objects
	if (!m_objects.empty())
	{
		// Create a "multipart/related" body part
		// std::shared_ptr<bodyPart> relPart = std::make_shared<bodyPart>();
		// TODO shared
		std::shared_ptr<bodyPart> relPart = bodyPart::construct();
		parent->getBody()->appendPart(relPart);

		relPart->getHeader()->ContentType()->
			setValue(mediaType(mediaTypes::MULTIPART, mediaTypes::MULTIPART_RELATED));

		// Add the HTML part into this part
		relPart->getBody()->appendPart(htmlPart);

		// Also add objects into this part
		for (std::vector <std::shared_ptr<embeddedObject> >::const_iterator it = m_objects.begin() ;
		     it != m_objects.end() ; ++it)
		{
			// std::shared_ptr<bodyPart> objPart =
			// std::make_shared<bodyPart>(); TODO shared
			std::shared_ptr<bodyPart> objPart = bodyPart::construct();
			relPart->getBody()->appendPart(objPart);

			string id = (*it)->getId();

			if (id.substr(0, 4) == "CID:")
				id = id.substr(4);

			objPart->getHeader()->ContentType()->setValue((*it)->getType());
			objPart->getHeader()->ContentId()->setValue(messageId("<" + id + ">"));
			objPart->getHeader()->ContentDisposition()->setValue(contentDisposition(contentDispositionTypes::INLINE));
			objPart->getHeader()->ContentTransferEncoding()->setValue((*it)->getEncoding());
				//encoding(encodingTypes::BASE64);

			objPart->getBody()->setContents((*it)->getData()->clone());
		}
	}
	else
	{
		// Add the HTML part into the parent part
		parent->getBody()->appendPart(htmlPart);
	}
}


void htmlTextPart::findEmbeddedParts(const bodyPart& part,
	std::vector <std::shared_ptr<const bodyPart> >& cidParts, std::vector <std::shared_ptr<const bodyPart> >& locParts)
{
	for (int i = 0 ; i < part.getBody()->getPartCount() ; ++i)
	{
		std::shared_ptr<const bodyPart> p = part.getBody()->getPartAt(i);

		// For a part to be an embedded object, it must have a
		// Content-Id field or a Content-Location field.
		try
		{
			p->getHeader()->findField(fields::CONTENT_ID);
			cidParts.push_back(p);
		}
		catch (exceptions::no_such_field)
		{
			// No "Content-id" field.
		}

		try
		{
			p->getHeader()->findField(fields::CONTENT_LOCATION);
			locParts.push_back(p);
		}
		catch (exceptions::no_such_field)
		{
			// No "Content-Location" field.
		}

		findEmbeddedParts(*p, cidParts, locParts);
	}
}


void htmlTextPart::addEmbeddedObject(const bodyPart& part, const string& id)
{
	// The object may already exists. This can happen if an object is
	// identified by both a Content-Id and a Content-Location. In this
	// case, there will be two embedded objects with two different IDs
	// but referencing the same content.

	mediaType type;

	try
	{
		const std::shared_ptr<const headerField> ctf = part.getHeader()->ContentType();
		// type = *ctf->getValue().dynamicCast <const mediaType>(); TODO
		// shared
		type = *std::dynamic_pointer_cast<const mediaType>(ctf->getValue());
	}
	catch (exceptions::no_such_field)
	{
		// No "Content-type" field: assume "application/octet-stream".
	}

	m_objects.push_back(std::make_shared<embeddedObject>
		// (part.getBody()->getContents()->clone().dynamicCast
		// <contentHandler>(), TODO shared
		(std::dynamic_pointer_cast<contentHandler>(part.getBody()->getContents()->clone()),
		 part.getBody()->getEncoding(), id, type));
}


void htmlTextPart::parse(std::shared_ptr<const bodyPart> message, std::shared_ptr<const bodyPart> parent, std::shared_ptr<const bodyPart> textPart)
{
	// Search for possible embedded objects in the _whole_ message.
	std::vector <std::shared_ptr<const bodyPart> > cidParts;
	std::vector <std::shared_ptr<const bodyPart> > locParts;

	findEmbeddedParts(*message, cidParts, locParts);

	// Extract HTML text
	std::ostringstream oss;
	utility::outputStreamAdapter adapter(oss);

	textPart->getBody()->getContents()->extract(adapter);

	const string data = oss.str();

	m_text = textPart->getBody()->getContents()->clone();

	try
	{
		// const std::shared_ptr<const contentTypeField> ctf = TODO shared
			//textPart->getHeader()->findField(fields::CONTENT_TYPE).dynamicCast <contentTypeField>();
		const std::shared_ptr<const contentTypeField> ctf =
			std::dynamic_pointer_cast<contentTypeField>(textPart->getHeader()->
					findField(fields::CONTENT_TYPE));

		m_charset = ctf->getCharset();
	}
	catch (exceptions::no_such_field)
	{
		// No "Content-type" field.
	}
	catch (exceptions::no_such_parameter)
	{
		// No "charset" parameter.
	}

	// Extract embedded objects. The algorithm is quite simple: for each previously
	// found inline part, we check if its CID/Location is contained in the HTML text.
	for (std::vector <std::shared_ptr<const bodyPart> >::const_iterator p = cidParts.begin() ; p != cidParts.end() ; ++p)
	{
		const std::shared_ptr<const headerField> midField =
			(*p)->getHeader()->findField(fields::CONTENT_ID);

		// const messageId mid = *midField->getValue().dynamicCast <const
		// messageId>(); TODO shared
		const messageId mid = *std::dynamic_pointer_cast<const messageId>(midField->getValue());

		if (data.find("CID:" + mid.getId()) != string::npos ||
		    data.find("cid:" + mid.getId()) != string::npos)
		{
			// This part is referenced in the HTML text.
			// Add it to the embedded object list.
			addEmbeddedObject(**p, mid.getId());
		}
	}

	for (std::vector <std::shared_ptr<const bodyPart> >::const_iterator p = locParts.begin() ; p != locParts.end() ; ++p)
	{
		const std::shared_ptr<const headerField> locField =
			(*p)->getHeader()->findField(fields::CONTENT_LOCATION);

		// const text loc = *locField->getValue().dynamicCast <const text>();
		// TODO shared
		const text loc = *std::dynamic_pointer_cast<const text>(locField->getValue());
		const string locStr = loc.getWholeBuffer();

		if (data.find(locStr) != string::npos)
		{
			// This part is referenced in the HTML text.
			// Add it to the embedded object list.
			addEmbeddedObject(**p, locStr);
		}
	}

	// Extract plain text, if any.
	if (!findPlainTextPart(*message, *parent, *textPart))
	{
		m_plainText = std::make_shared<emptyContentHandler>();
	}
}


bool htmlTextPart::findPlainTextPart(const bodyPart& part, const bodyPart& parent, const bodyPart& textPart)
{
	// We search for the nearest "multipart/alternative" part.
	try
	{
		const std::shared_ptr<const headerField> ctf =
			part.getHeader()->findField(fields::CONTENT_TYPE);

		//const mediaType type = *ctf->getValue().dynamicCast <const
		//mediaType>(); TODO shared
		const mediaType type = *std::dynamic_pointer_cast<const mediaType>(ctf->getValue());

		if (type.getType() == mediaTypes::MULTIPART &&
		    type.getSubType() == mediaTypes::MULTIPART_ALTERNATIVE)
		{
			//std::shared_ptr<const bodyPart> foundPart = NULL; TODO shared
			std::shared_ptr<const bodyPart> foundPart;

			for (int i = 0 ; i < part.getBody()->getPartCount() ; ++i)
			{
				const std::shared_ptr<const bodyPart> p = part.getBody()->getPartAt(i);

				// if (p == &parent ||     // if "text/html" is in
				// "multipart/related" TODO shared
				 // p == &textPart)     // if not... TODO shared
				if (p.get() == &parent ||     // if "text/html" is in "multipart/related"
				    p.get() == &textPart)     // if not...
				{
					foundPart = p;
				}
			}

			if (foundPart)
			{
				bool found = false;

				// Now, search for the alternative plain text part
				for (int i = 0 ; !found && i < part.getBody()->getPartCount() ; ++i)
				{
					const std::shared_ptr<const bodyPart> p = part.getBody()->getPartAt(i);

					try
					{
						const std::shared_ptr<const headerField> ctf =
							p->getHeader()->findField(fields::CONTENT_TYPE);

						// const mediaType type = *ctf->getValue().dynamicCast
						// <const mediaType>(); TODO shared
						const mediaType type = *dynamic_cast<const
							mediaType*>(ctf->getValue().get());

						if (type.getType() == mediaTypes::TEXT &&
						    type.getSubType() == mediaTypes::TEXT_PLAIN)
						{
							m_plainText = p->getBody()->getContents()->clone();
							found = true;
						}
					}
					catch (exceptions::no_such_field)
					{
						// No "Content-type" field.
					}
				}

				// If we don't have found the plain text part here, it means that
				// it does not exists (the MUA which built this message probably
				// did not include it...).
				return found;
			}
		}
	}
	catch (exceptions::no_such_field)
	{
		// No "Content-type" field.
	}

	bool found = false;

	for (int i = 0 ; !found && i < part.getBody()->getPartCount() ; ++i)
	{
		found = findPlainTextPart(*part.getBody()->getPartAt(i), parent, textPart);
	}

	return found;
}


const charset& htmlTextPart::getCharset() const
{
	return m_charset;
}


void htmlTextPart::setCharset(const charset& ch)
{
	m_charset = ch;
}


const std::shared_ptr<const contentHandler> htmlTextPart::getPlainText() const
{
	return m_plainText;
}


void htmlTextPart::setPlainText(std::shared_ptr<contentHandler> plainText)
{
	m_plainText = plainText->clone();
}


const std::shared_ptr<const contentHandler> htmlTextPart::getText() const
{
	return m_text;
}


void htmlTextPart::setText(std::shared_ptr<contentHandler> text)
{
	m_text = text->clone();
}


int htmlTextPart::getObjectCount() const
{
	return m_objects.size();
}


const std::shared_ptr<const htmlTextPart::embeddedObject> htmlTextPart::getObjectAt(const int pos) const
{
	return m_objects[pos];
}


const std::shared_ptr<const htmlTextPart::embeddedObject> htmlTextPart::findObject(const string& id_) const
{
	const string id = cleanId(id_);

	for (std::vector <std::shared_ptr<embeddedObject> >::const_iterator o = m_objects.begin() ;
	     o != m_objects.end() ; ++o)
	{
		if ((*o)->getId() == id)
			return *o;
	}

	throw exceptions::no_object_found();
}


bool htmlTextPart::hasObject(const string& id_) const
{
	const string id = cleanId(id_);

	for (std::vector <std::shared_ptr<embeddedObject> >::const_iterator o = m_objects.begin() ;
	     o != m_objects.end() ; ++o)
	{
		if ((*o)->getId() == id)
			return true;
	}

	return false;
}


const string htmlTextPart::addObject(std::shared_ptr<contentHandler> data,
	const vmime::encoding& enc, const mediaType& type)
{
	const messageId mid(messageId::generateId());
	const string id = mid.getId();

	m_objects.push_back(std::make_shared<embeddedObject>(data, enc, id, type));

	return "CID:" + id;
}


const string htmlTextPart::addObject(std::shared_ptr<contentHandler> data, const mediaType& type)
{
	return addObject(data, encoding::decide(data), type);
}


const string htmlTextPart::addObject(const string& data, const mediaType& type)
{
	std::shared_ptr<stringContentHandler> cts = std::make_shared<stringContentHandler>(data);
	return addObject(cts, encoding::decide(cts), type);
}


// static
const string htmlTextPart::cleanId(const string& id)
{
	if (id.length() >= 4 &&
	    (id[0] == 'c' || id[0] == 'C') &&
	    (id[1] == 'i' || id[1] == 'I') &&
	    (id[2] == 'd' || id[2] == 'D') &&
	    id[3] == ':')
	{
		return id.substr(4);
	}
	else
	{
		return id;
	}
}



//
// htmlTextPart::embeddedObject
//

htmlTextPart::embeddedObject::embeddedObject
	(std::shared_ptr<contentHandler> data, const encoding& enc,
	 const string& id, const mediaType& type)
	//: m_data(data->clone().dynamicCast <contentHandler>()), TODO shared
	//: (m_data(data->clone())),
	  : m_encoding(enc), m_id(id), m_type(type)
{
	m_data = data->clone();
}


const std::shared_ptr<const contentHandler> htmlTextPart::embeddedObject::getData() const
{
	return m_data;
}


const vmime::encoding& htmlTextPart::embeddedObject::getEncoding() const
{
	return m_encoding;
}


const string& htmlTextPart::embeddedObject::getId() const
{
	return m_id;
}


const mediaType& htmlTextPart::embeddedObject::getType() const
{
	return m_type;
}


} // vmime
