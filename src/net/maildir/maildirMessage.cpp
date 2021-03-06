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

#include "vmime/net/maildir/maildirMessage.hpp"
#include "vmime/net/maildir/maildirFolder.hpp"
#include "vmime/net/maildir/maildirUtils.hpp"
#include "vmime/net/maildir/maildirStore.hpp"

#include "vmime/message.hpp"

#include "vmime/exception.hpp"
#include "vmime/platform.hpp"

#include "vmime/utility/outputStreamAdapter.hpp"


namespace vmime {
namespace net {
namespace maildir {


//
// maildirPart
//

class maildirStructure;

class maildirPart : public part
{
public:

	maildirPart(std::shared_ptr<maildirPart> parent, const int number, const bodyPart& part);
	~maildirPart();


	std::shared_ptr<const structure> getStructure() const;
	std::shared_ptr<structure> getStructure();

	std::weak_ptr<const maildirPart> getParent() const { return (m_parent); }

	const mediaType& getType() const { return (m_mediaType); }
	int getSize() const { return (m_size); }
	int getNumber() const { return (m_number); }

	std::shared_ptr<const header> getHeader() const
	{
		if (m_header == NULL)
			throw exceptions::unfetched_object();
		else
			return m_header;
	}

	header& getOrCreateHeader()
	{
		if (m_header != NULL)
			return (*m_header);
		else
			return (*(m_header = vmime::factory<header>::create()));
	}

	int getHeaderParsedOffset() const { return (m_headerParsedOffset); }
	int getHeaderParsedLength() const { return (m_headerParsedLength); }

	int getBodyParsedOffset() const { return (m_bodyParsedOffset); }
	int getBodyParsedLength() const { return (m_bodyParsedLength); }

	void initStructure(const bodyPart& part);

private:

	std::shared_ptr<maildirStructure> m_structure;
	std::weak_ptr<maildirPart> m_parent;
	std::shared_ptr<header> m_header;

	int m_number;
	int m_size;
	mediaType m_mediaType;

	int m_headerParsedOffset;
	int m_headerParsedLength;

	int m_bodyParsedOffset;
	int m_bodyParsedLength;
};



//
// maildirStructure
//

class maildirStructure : public structure
{
public:

	maildirStructure()
	{
	}

	maildirStructure(std::shared_ptr<maildirPart> parent, const bodyPart& part)
	{
		std::shared_ptr<maildirPart> mpart =
			vmime::factory<maildirPart>::create(parent, 0, part);
		mpart->initStructure(part);

		m_parts.push_back(mpart);
	}

	maildirStructure(std::shared_ptr<maildirPart> parent, const std::vector <std::shared_ptr<const vmime::bodyPart> >& list)
	{
		for (unsigned int i = 0 ; i < list.size() ; ++i)
		{
			std::shared_ptr<maildirPart> mpart =
				vmime::factory<maildirPart>::create(parent, i, *list[i]);
			mpart->initStructure(*list[i]);

			m_parts.push_back(mpart);
		}
	}


	std::shared_ptr<const part> getPartAt(const int x) const
	{
		return m_parts[x];
	}

	std::shared_ptr<part> getPartAt(const int x)
	{
		return m_parts[x];
	}

	int getPartCount() const
	{
		return m_parts.size();
	}


	static std::shared_ptr<maildirStructure> emptyStructure()
	{
		return m_emptyStructure;
	}

private:

	static std::shared_ptr<maildirStructure> m_emptyStructure;

	std::vector <std::shared_ptr<maildirPart> > m_parts;
};


std::shared_ptr<maildirStructure> maildirStructure::m_emptyStructure =
vmime::factory<maildirStructure>::create();



maildirPart::maildirPart(std::shared_ptr<maildirPart> parent, const int number, const bodyPart& part)
	: m_parent(parent), m_header(NULL), m_number(number)
{
	m_headerParsedOffset = part.getHeader()->getParsedOffset();
	m_headerParsedLength = part.getHeader()->getParsedLength();

	m_bodyParsedOffset = part.getBody()->getParsedOffset();
	m_bodyParsedLength = part.getBody()->getParsedLength();

	m_size = part.getBody()->getContents()->getLength();

	m_mediaType = part.getBody()->getContentType();
}


maildirPart::~maildirPart()
{
}


void maildirPart::initStructure(const bodyPart& part)
{
	if (part.getBody()->getPartList().size() == 0)
		m_structure = NULL;
	else
	{
		m_structure = vmime::factory<maildirStructure>::create
			// (thisRef().dynamicCast <maildirPart>(), TODO shared
			(std::dynamic_pointer_cast<maildirPart>(thisRef()),
			 part.getBody()->getPartList());
	}
}


std::shared_ptr<const structure> maildirPart::getStructure() const
{
	if (m_structure != NULL)
		return m_structure;
	else
		return maildirStructure::emptyStructure();
}


std::shared_ptr<structure> maildirPart::getStructure()
{
	if (m_structure != NULL)
		return m_structure;
	else
		return maildirStructure::emptyStructure();
}



//
// maildirMessage
//

maildirMessage::maildirMessage(std::shared_ptr<maildirFolder> folder, const int num)
	: m_folder(folder), m_num(num), m_size(-1), m_flags(FLAG_UNDEFINED),
	  m_expunged(false), m_structure(NULL)
{
	folder->registerMessage(this);
}


maildirMessage::~maildirMessage()
{
	// std::shared_ptr<maildirFolder> folder = m_folder.acquire(); TODO shared
	std::shared_ptr<maildirFolder> folder = m_folder.lock();

	if (folder)
		folder->unregisterMessage(this);
}


void maildirMessage::onFolderClosed()
{
	// m_folder = NULL; TODO shared
	m_folder.reset();
}


int maildirMessage::getNumber() const
{
	return (m_num);
}


const message::uid maildirMessage::getUniqueId() const
{
	return (m_uid);
}


int maildirMessage::getSize() const
{
	if (m_size == -1)
		throw exceptions::unfetched_object();

	return (m_size);
}


bool maildirMessage::isExpunged() const
{
	return (m_expunged);
}


std::shared_ptr<const structure> maildirMessage::getStructure() const
{
	if (m_structure == NULL)
		throw exceptions::unfetched_object();

	return m_structure;
}


std::shared_ptr<structure> maildirMessage::getStructure()
{
	if (m_structure == NULL)
		throw exceptions::unfetched_object();

	return m_structure;
}


std::shared_ptr<const header> maildirMessage::getHeader() const
{
	if (m_header == NULL)
		throw exceptions::unfetched_object();

	return (m_header);
}


int maildirMessage::getFlags() const
{
	if (m_flags == FLAG_UNDEFINED)
		throw exceptions::unfetched_object();

	return (m_flags);
}


void maildirMessage::setFlags(const int flags, const int mode)
{
	// std::shared_ptr<maildirFolder> folder = m_folder.acquire(); TODO shared
	std::shared_ptr<maildirFolder> folder = m_folder.lock();

	if (!folder)
		throw exceptions::folder_not_found();

	folder->setMessageFlags(m_num, m_num, flags, mode);
}


void maildirMessage::extract(utility::outputStream& os,
	utility::progressListener* progress, const int start,
	const int length, const bool peek) const
{
	extractImpl(os, progress, 0, m_size, start, length, peek);
}


void maildirMessage::extractPart(std::shared_ptr<const part> p, utility::outputStream& os,
	utility::progressListener* progress, const int start,
	const int length, const bool peek) const
{
	// std::shared_ptr<const maildirPart> mp = p.dynamicCast <const
	// maildirPart>(); TODO shared
	std::shared_ptr<const maildirPart> mp = std::dynamic_pointer_cast<const maildirPart>(p);

	extractImpl(os, progress, mp->getBodyParsedOffset(), mp->getBodyParsedLength(),
		start, length, peek);
}


void maildirMessage::extractImpl(utility::outputStream& os, utility::progressListener* progress,
	const int start, const int length, const int partialStart, const int partialLength,
	const bool /* peek */) const
{
	// std::shared_ptr<const maildirFolder> folder = m_folder.acquire(); TODO
	// shared
	std::shared_ptr<const maildirFolder> folder = m_folder.lock();

	std::shared_ptr<utility::fileSystemFactory> fsf = platform::getHandler()->getFileSystemFactory();

	const utility::file::path path = folder->getMessageFSPath(m_num);
	std::shared_ptr<utility::file> file = fsf->create(path);

	std::shared_ptr<utility::fileReader> reader = file->getFileReader();
	std::shared_ptr<utility::inputStream> is = reader->getInputStream();

	is->skip(start + partialStart);

	utility::stream::value_type buffer[8192];
	utility::stream::size_type remaining = (partialLength == -1 ? length
		: std::min(partialLength, length));

	const int total = remaining;
	int current = 0;

	if (progress)
		progress->start(total);

	while (!is->eof() && remaining > 0)
	{
		const utility::stream::size_type read =
			is->read(buffer, std::min(remaining, sizeof(buffer)));

		remaining -= read;
		current += read;

		os.write(buffer, read);

		if (progress)
			progress->progress(current, total);
	}

	if (progress)
		progress->stop(total);

	// TODO: mark as read unless 'peek' is set
}


void maildirMessage::fetchPartHeader(std::shared_ptr<part> p)
{
	// std::shared_ptr<maildirFolder> folder = m_folder.acquire(); TODO shared
	std::shared_ptr<maildirFolder> folder = m_folder.lock();

	// std::shared_ptr<maildirPart> mp = p.dynamicCast <maildirPart>(); TODO
	// shared
	std::shared_ptr<maildirPart> mp = std::dynamic_pointer_cast<maildirPart>(p);

	std::shared_ptr<utility::fileSystemFactory> fsf = platform::getHandler()->getFileSystemFactory();

	const utility::file::path path = folder->getMessageFSPath(m_num);
	std::shared_ptr<utility::file> file = fsf->create(path);

	std::shared_ptr<utility::fileReader> reader = file->getFileReader();
	std::shared_ptr<utility::inputStream> is = reader->getInputStream();

	is->skip(mp->getHeaderParsedOffset());

	utility::stream::value_type buffer[1024];
	utility::stream::size_type remaining = mp->getHeaderParsedLength();

	string contents;
	contents.reserve(remaining);

	while (!is->eof() && remaining > 0)
	{
		const utility::stream::size_type read =
			is->read(buffer, std::min(remaining, sizeof(buffer)));

		remaining -= read;

		contents.append(buffer, read);
	}

	mp->getOrCreateHeader().parse(contents);
}


void maildirMessage::fetch(std::shared_ptr<maildirFolder> msgFolder, const int options)
{
	// std::shared_ptr<maildirFolder> folder = m_folder.acquire(); TODO shared
	std::shared_ptr<maildirFolder> folder = m_folder.lock();

	if (folder != msgFolder)
		throw exceptions::folder_not_found();

	std::shared_ptr<utility::fileSystemFactory> fsf = platform::getHandler()->getFileSystemFactory();

	const utility::file::path path = folder->getMessageFSPath(m_num);
	std::shared_ptr<utility::file> file = fsf->create(path);

	if (options & folder::FETCH_FLAGS)
		m_flags = maildirUtils::extractFlags(path.getLastComponent());

	if (options & folder::FETCH_SIZE)
		m_size = file->getLength();

	if (options & folder::FETCH_UID)
		m_uid = maildirUtils::extractId(path.getLastComponent()).getBuffer();

	if (options & (folder::FETCH_ENVELOPE | folder::FETCH_CONTENT_INFO |
	               folder::FETCH_FULL_HEADER | folder::FETCH_STRUCTURE |
	               folder::FETCH_IMPORTANCE))
	{
		string contents;

		std::shared_ptr<utility::fileReader> reader = file->getFileReader();
		std::shared_ptr<utility::inputStream> is = reader->getInputStream();

		// Need whole message contents for structure
		if (options & folder::FETCH_STRUCTURE)
		{
			utility::stream::value_type buffer[16384];

			contents.reserve(file->getLength());

			while (!is->eof())
			{
				const utility::stream::size_type read = is->read(buffer, sizeof(buffer));
				contents.append(buffer, read);
			}
		}
		// Need only header
		else
		{
			utility::stream::value_type buffer[1024];

			contents.reserve(4096);

			while (!is->eof())
			{
				const utility::stream::size_type read = is->read(buffer, sizeof(buffer));
				contents.append(buffer, read);

				const string::size_type sep1 = contents.rfind("\r\n\r\n");
				const string::size_type sep2 = contents.rfind("\n\n");

				if (sep1 != string::npos)
				{
					contents.erase(contents.begin() + sep1 + 4, contents.end());
					break;
				}
				else if (sep2 != string::npos)
				{
					contents.erase(contents.begin() + sep2 + 2, contents.end());
					break;
				}
			}
		}

		vmime::message msg;
		msg.parse(contents);

		// Extract structure
		if (options & folder::FETCH_STRUCTURE)
		{
			// m_structure = std::make_shared<maildirStructure>(null, msg);
			// TODO shared
			m_structure = vmime::factory<maildirStructure>::create(nullptr, msg);
		}

		// Extract some header fields or whole header
		if (options & (folder::FETCH_ENVELOPE |
		               folder::FETCH_CONTENT_INFO |
		               folder::FETCH_FULL_HEADER |
		               folder::FETCH_IMPORTANCE))
		{
			getOrCreateHeader()->copyFrom(*(msg.getHeader()));
		}
	}
}


std::shared_ptr<header> maildirMessage::getOrCreateHeader()
{
	if (m_header != NULL)
		return (m_header);
	else
		return (m_header = vmime::factory<header>::create());
}


std::shared_ptr<vmime::message> maildirMessage::getParsedMessage()
{
	std::ostringstream oss;
	utility::outputStreamAdapter os(oss);

	extract(os);

	std::shared_ptr<vmime::message> msg =
		vmime::factory<vmime::message>::create();
	msg->parse(oss.str());

	return msg;
}


} // maildir
} // net
} // vmime
