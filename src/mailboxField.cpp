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

#include "vmime/mailboxField.hpp"
#include "vmime/mailboxGroup.hpp"


#ifndef VMIME_BUILDING_DOC


namespace vmime
{


mailboxField::mailboxField()
{
}


mailboxField::mailboxField(const mailboxField&)
	: headerField()
{
}


void mailboxField::parse(const string& buffer, const string::size_type position,
	const string::size_type end, string::size_type* newPosition)
{
	std::shared_ptr<mailbox> mbox = vmime::factory<mailbox>::create();

	// Here, we cannot simply call "m_mailbox.parse()" because it
	// may have more than one address specified (even if this field
	// should contain only one). We are never too much careful...
	std::shared_ptr<address> parsedAddress = address::parseNext(buffer, position, end, newPosition);

	if (parsedAddress)
	{
		if (parsedAddress->isGroup())
		{
			// If it is a group of mailboxes, take the first
			// mailbox of the group
			// std::shared_ptr<mailboxGroup> group = parsedAddress.staticCast
			// <mailboxGroup>(); TODO shared
			std::shared_ptr<mailboxGroup> group =
				std::static_pointer_cast<mailboxGroup>(parsedAddress);

			if (!group->isEmpty())
				mbox = group->getMailboxAt(0);
		}
		else
		{
			// Parse only if it is a mailbox
			// mbox = parsedAddress.staticCast <mailbox>(); TODO shared
			mbox = std::static_pointer_cast<mailbox>(parsedAddress);
		}
	}

	mbox->setParsedBounds(position, end);

	setValue(mbox);

	setParsedBounds(position, end);

	if (newPosition)
		*newPosition = end;
}


} // vmime


#endif // VMIME_BUILDING_DOC

