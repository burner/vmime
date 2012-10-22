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

#ifndef VMIME_UTILITY_ENCODER_ENCODERFACTORY_HPP_INCLUDED
#define VMIME_UTILITY_ENCODER_ENCODERFACTORY_HPP_INCLUDED


#include "vmime/utility/encoder/encoder.hpp"
#include "vmime/utility/stringUtils.hpp"


namespace vmime {
namespace utility {
namespace encoder {


/** A factory to create 'encoder' objects for the specified encoding.
  */

class encoderFactory
{
	friend class vmime::factory<encoderFactory>;
private:

	encoderFactory();

public:
	~encoderFactory();

	// static encoderFactory* getInstance(); TODO shared
	static std::shared_ptr<encoderFactory> getInstance();

	/** Information about a registered encoder. */
	class registeredEncoder : public object
	{
	protected:

		virtual ~registeredEncoder() { }

	public:

		virtual std::shared_ptr<encoder> create() const = 0;

		virtual const string& getName() const = 0;
	};

private:

	template <class E>
	class registeredEncoderImpl : public registeredEncoder
	{
		//friend class vmime::creator; TODO shared_ptr
		//friend class vmime::factory<registeredEncoderImpl<E>>;

	protected:

	public:
		// TODO shared was protected
		registeredEncoderImpl(const string& name) : m_name(name) { }

		std::shared_ptr<encoder> create() const
		{
			return vmime::factory<E>::create();
		}

		const string& getName() const
		{
			return (m_name);
		}

	private:

		const string m_name;
	};


	std::vector <std::shared_ptr<registeredEncoder> > m_encoders;

public:

	/** Register a new encoder by its encoding name.
	  *
	  * @param name encoding name
	  */
	template <class E>
	void registerName(const string& name)
	{
		//m_encoders.push_back(std::make_shared<registeredEncoderImpl <E>
		//>(utility::stringUtils::toLower(name))); TODO shared
		//std::shared_ptr<registeredEncoderImpl<E>> tmpStd =
		//	vmime::factory<registeredEncoderImpl<E>>(
		//	);
		// registeredEncoderImpl<E>* tmp = new registeredEncoderImpl<E>( TODO
		// shared
		std::shared_ptr<registeredEncoderImpl<E>> tmp =
			vmime::factory<registeredEncoderImpl<E>>::create(
				utility::stringUtils::toLower(name)
				);
		std::shared_ptr<object> tmpStd = tmp->thisRef();
		m_encoders.push_back(
			std::dynamic_pointer_cast<registeredEncoderImpl<E>>(tmpStd)
			//tmpStd
			//vmime::factory<registeredEncoderImpl<E>>::create(
			//	utility::stringUtils::toLower(name)
			//)
		);
	}

	/** Create a new encoder instance from an encoding name.
	  *
	  * @param name encoding name (eg. "base64")
	  * @return a new encoder instance for the specified encoding
	  * @throw exceptions::no_encoder_available if no encoder is registered
	  * for this encoding
	  */
	std::shared_ptr<encoder> create(const string& name);

	/** Return information about a registered encoder.
	  *
	  * @param name encoding name
	  * @return information about this encoder
	  * @throw exceptions::no_encoder_available if no encoder is registered
	  * for this encoding
	  */
	const std::shared_ptr<const registeredEncoder> getEncoderByName(const string& name) const;

	/** Return the number of registered encoders.
	  *
	  * @return number of registered encoders
	  */
	int getEncoderCount() const;

	/** Return the registered encoder at the specified position.
	  *
	  * @param pos position of the registered encoder to return
	  * @return registered encoder at the specified position
	  */
	const std::shared_ptr<const registeredEncoder> getEncoderAt(const int pos) const;

	/** Return a list of all registered encoders.
	  *
	  * @return list of registered encoders
	  */
	const std::vector <std::shared_ptr<const registeredEncoder> > getEncoderList() const;
};


} // encoder
} // utility
} // vmime


#endif // VMIME_UTILITY_ENCODER_ENCODERFACTORY_HPP_INCLUDED
