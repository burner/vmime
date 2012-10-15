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

#ifndef VMIME_NET_IMAP_IMAPMESSAGEPARTCONTENTHANDLER_HPP_INCLUDED
#define VMIME_NET_IMAP_IMAPMESSAGEPARTCONTENTHANDLER_HPP_INCLUDED


#include "vmime/contentHandler.hpp"
#include "vmime/net/imap/IMAPMessage.hpp"


namespace vmime {
namespace net {
namespace imap {


class IMAPMessagePartContentHandler : public contentHandler
{
public:

	IMAPMessagePartContentHandler(std::shared_ptr<IMAPMessage> msg, std::shared_ptr<class part> part, const vmime::encoding& encoding);

	std::shared_ptr<contentHandler> clone() const;

	void generate(utility::outputStream& os, const vmime::encoding& enc, const string::size_type maxLineLength = lineLengthLimits::infinite) const;

	void extract(utility::outputStream& os, utility::progressListener* progress = NULL) const;
	void extractRaw(utility::outputStream& os, utility::progressListener* progress = NULL) const;

	string::size_type getLength() const;

	bool isEncoded() const;

	const vmime::encoding& getEncoding() const;

	bool isEmpty() const;

	bool isBuffered() const;

private:

	std::weak_ptr<IMAPMessage> m_message;
	std::weak_ptr<part> m_part;

	vmime::encoding m_encoding;
};


} // imap
} // net
} // vmime


#endif // VMIME_NET_IMAP_IMAPMESSAGEPARTCONTENTHANDLER_HPP_INCLUDED

