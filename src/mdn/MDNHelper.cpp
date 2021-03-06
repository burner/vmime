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

#include "vmime/mdn/MDNHelper.hpp"

#include "vmime/exception.hpp"
#include "vmime/stringContentHandler.hpp"

#include "vmime/contentTypeField.hpp"

#include "vmime/path.hpp"
#include "vmime/dateTime.hpp"

#include "vmime/utility/outputStreamAdapter.hpp"


namespace vmime {
namespace mdn {


void MDNHelper::attachMDNRequest(std::shared_ptr<message> msg, const mailboxList& mailboxes)
{
	std::shared_ptr<header> hdr = msg->getHeader();

	hdr->DispositionNotificationTo()->setValue(mailboxes);
}


void MDNHelper::attachMDNRequest(std::shared_ptr<message> msg, const mailbox& mbox)
{
	mailboxList mboxList;
	//mboxList.appendMailbox(mbox.clone().dynamicCast <mailbox>()); TODO shared
	mboxList.appendMailbox(std::dynamic_pointer_cast<mailbox>(mbox.clone()));

	attachMDNRequest(msg, mboxList);
}


const std::vector <sendableMDNInfos> MDNHelper::getPossibleMDNs(const std::shared_ptr<const message> msg)
{
	std::vector <sendableMDNInfos> result;

	const std::shared_ptr<const header> hdr = msg->getHeader();

	if (hdr->hasField(fields::DISPOSITION_NOTIFICATION_TO))
	{
		//const mailboxList& dnto = *hdr->DispositionNotificationTo()->getValue() TODO shared
			//.dynamicCast <const mailboxList>(); TODO shared
		const mailboxList& dnto = *std::dynamic_pointer_cast<const mailboxList>(
			hdr->DispositionNotificationTo()->getValue());

		for (int i = 0 ; i < dnto.getMailboxCount() ; ++i)
			result.push_back(sendableMDNInfos(msg, *dnto.getMailboxAt(i)));
	}

	return (result);
}


bool MDNHelper::isMDN(const std::shared_ptr<const message> msg)
{
	const std::shared_ptr<const header> hdr = msg->getHeader();

	// A MDN message implies the following:
	//   - a Content-Type field is present and its value is "multipart/report"
	//   - a "report-type" parameter is present in the Content-Type field,
	//     and its value is "disposition-notification"
	if (hdr->hasField(fields::CONTENT_TYPE))
	{
		//const contentTypeField& ctf = *(hdr->ContentType() TODO shared
		//	.dynamicCast <const contentTypeField>());
		const contentTypeField& ctf = *std::dynamic_pointer_cast<const contentTypeField>(hdr->ContentType());

		//const mediaType type = *ctf.getValue().dynamicCast <const mediaType>(); TODO shared
		const mediaType type = *std::dynamic_pointer_cast<const mediaType>(ctf.getValue());

		if (type.getType() == vmime::mediaTypes::MULTIPART &&
		    type.getSubType() == vmime::mediaTypes::MULTIPART_REPORT)
		{
			if (ctf.hasParameter("report-type") &&
			    ctf.getReportType() == "disposition-notification")
			{
				return (true);
			}
		}
	}

	return (false);
}


receivedMDNInfos MDNHelper::getReceivedMDN(const std::shared_ptr<const message> msg)
{
	if (!isMDN(msg))
		throw exceptions::invalid_argument();

	return receivedMDNInfos(msg);
}


bool MDNHelper::needConfirmation(const std::shared_ptr<const message> msg)
{
	std::shared_ptr<const header> hdr = msg->getHeader();

	// No "Return-Path" field
	if (!hdr->hasField(fields::RETURN_PATH))
		return true;

	// More than one address in Disposition-Notification-To
	if (hdr->hasField(fields::DISPOSITION_NOTIFICATION_TO))
	{
		//const mailboxList& dnto = *hdr->DispositionNotificationTo()->getValue()
		//	.dynamicCast <const mailboxList>(); TODO shared
		const mailboxList& dnto = *std::dynamic_pointer_cast<const mailboxList>(
			hdr->DispositionNotificationTo()->getValue());

		if (dnto.getMailboxCount() > 1)
			return true;
		else if (dnto.getMailboxCount() == 0)
			return false;

		// Return-Path != Disposition-Notification-To
		const mailbox& mbox = *dnto.getMailboxAt(0);
		//const path& rp = *hdr->ReturnPath()->getValue().dynamicCast <const path>(); TODO shared
		const path& rp = *std::dynamic_pointer_cast<const path>(hdr->ReturnPath()->getValue());

		if (mbox.getEmail() != rp.getLocalPart() + "@" + rp.getDomain())
			return true;
	}

	// User confirmation not needed
	return false;
}


std::shared_ptr<message> MDNHelper::buildMDN(const sendableMDNInfos& mdnInfos,
                                  const string& text,
                                  const charset& ch,
                                  const mailbox& expeditor,
                                  const disposition& dispo,
                                  const string& reportingUA,
                                  const std::vector <string>& reportingUAProducts)
{
	// Create a new message
	std::shared_ptr<message> msg = vmime::factory<message>::create();

	// Fill-in header fields
	std::shared_ptr<header> hdr = msg->getHeader();

	hdr->ContentType()->setValue(mediaType(vmime::mediaTypes::MULTIPART,
	                                       vmime::mediaTypes::MULTIPART_REPORT));
	//hdr->ContentType().dynamicCast <contentTypeField>()->setReportType("disposition-notification"); TODO shared
	std::dynamic_pointer_cast<contentTypeField>(hdr->ContentType())->setReportType("disposition-notification");

	hdr->Disposition()->setValue(dispo);

	addressList to;
	to.appendAddress(vmime::factory<mailbox>::create(mdnInfos.getRecipient()));
	hdr->To()->setValue(to);

	hdr->From()->setValue(expeditor);

	hdr->Subject()->setValue(vmime::text(word("Disposition notification")));

	hdr->Date()->setValue(datetime::now());
	hdr->MimeVersion()->setValue(string(SUPPORTED_MIME_VERSION));

	msg->getBody()->appendPart(createFirstMDNPart(mdnInfos, text, ch));
	msg->getBody()->appendPart(createSecondMDNPart(mdnInfos,
		dispo, reportingUA, reportingUAProducts));
	msg->getBody()->appendPart(createThirdMDNPart(mdnInfos));

	return (msg);
}


std::shared_ptr<bodyPart> MDNHelper::createFirstMDNPart(const sendableMDNInfos& /* mdnInfos */,
                                             const string& text, const charset& ch)
{
	std::shared_ptr<bodyPart> part = vmime::factory<bodyPart>::create();

	// Header
	std::shared_ptr<header> hdr = part->getHeader();

	hdr->ContentType()->setValue(mediaType(vmime::mediaTypes::TEXT,
	                                       vmime::mediaTypes::TEXT_PLAIN));

	//hdr->ContentType().dynamicCast <contentTypeField>()->setCharset(ch); TODO shared
	std::dynamic_pointer_cast<contentTypeField>(hdr->ContentType())->setCharset(ch);

	// Body
	part->getBody()->setContents(vmime::factory<stringContentHandler>::create(text));

	return (part);
}


std::shared_ptr<bodyPart> MDNHelper::createSecondMDNPart(const sendableMDNInfos& mdnInfos,
                                              const disposition& dispo,
                                              const string& reportingUA,
                                              const std::vector <string>& reportingUAProducts)
{
	std::shared_ptr<bodyPart> part = vmime::factory<bodyPart>::create();

	// Header
	std::shared_ptr<header> hdr = part->getHeader();

	hdr->ContentDisposition()->setValue(vmime::contentDispositionTypes::INLINE);
	hdr->ContentType()->setValue(mediaType(vmime::mediaTypes::MESSAGE,
	                                       vmime::mediaTypes::MESSAGE_DISPOSITION_NOTIFICATION));

	// Body
	//
	//   The body of a message/disposition-notification consists of one or
	//   more "fields" formatted according to the ABNF of [RFC-MSGFMT] header
	//   "fields".  The syntax of the message/disposition-notification content
	//   is as follows:
	//
	//      disposition-notification-content = [ reporting-ua-field CRLF ]
	//      [ mdn-gateway-field CRLF ]
	//      [ original-recipient-field CRLF ]
	//      final-recipient-field CRLF
	//      [ original-message-id-field CRLF ]
	//      disposition-field CRLF
	//      *( failure-field CRLF )
	//      *( error-field CRLF )
	//      *( warning-field CRLF )
	//      *( extension-field CRLF )
	//
	header fields;

	// -- Reporting-UA (optional)
	if (!reportingUA.empty())
	{
		string ruaText;
		ruaText = reportingUA;

		for (unsigned int i = 0 ; i < reportingUAProducts.size() ; ++i)
		{
			if (i == 0)
				ruaText += "; ";
			else
				ruaText += ", ";

			ruaText += reportingUAProducts[i];
		}

		std::shared_ptr<headerField> rua = headerFieldFactory::getInstance()->
			create(vmime::fields::REPORTING_UA);

		rua->setValue(ruaText);

		fields.appendField(rua);
	}

	// -- Final-Recipient
	std::shared_ptr<headerField> fr = headerFieldFactory::getInstance()->
		create(vmime::fields::FINAL_RECIPIENT);

	fr->setValue("rfc822; " + mdnInfos.getRecipient().getEmail());

	fields.appendField(fr);

	// -- Original-Message-ID
	if (mdnInfos.getMessage()->getHeader()->hasField(vmime::fields::MESSAGE_ID))
	{
		fields.OriginalMessageId()->setValueConst
			(mdnInfos.getMessage()->getHeader()->MessageId()->getValue());
	}

	// -- Disposition
	fields.Disposition()->setValue(dispo);


	std::ostringstream oss;
	utility::outputStreamAdapter vos(oss);

	fields.generate(vos);

	part->getBody()->setContents(vmime::factory<stringContentHandler>::create(oss.str()));

	return (part);
}


std::shared_ptr<bodyPart> MDNHelper::createThirdMDNPart(const sendableMDNInfos& mdnInfos)
{
	std::shared_ptr<bodyPart> part = vmime::factory<bodyPart>::create();

	// Header
	std::shared_ptr<header> hdr = part->getHeader();

	hdr->ContentDisposition()->setValue(vmime::contentDispositionTypes::INLINE);
	hdr->ContentType()->setValue(mediaType(vmime::mediaTypes::TEXT,
	                                       vmime::mediaTypes::TEXT_RFC822_HEADERS));

	// Body: original message headers
	std::ostringstream oss;
	utility::outputStreamAdapter vos(oss);

	mdnInfos.getMessage()->getHeader()->generate(vos);

	part->getBody()->setContents(vmime::factory<stringContentHandler>::create(oss.str()));

	return (part);
}


} // mdn
} // vmime
