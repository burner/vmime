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

#include "tests/testUtils.hpp"


#define VMIME_TEST_SUITE         bodyPartTest
#define VMIME_TEST_SUITE_MODULE  "Parser"


VMIME_TEST_SUITE_BEGIN

	VMIME_TEST_LIST_BEGIN
		VMIME_TEST(testParse)
		VMIME_TEST(testGenerate)
		VMIME_TEST(testParseGuessBoundary)
		VMIME_TEST(testParseMissingLastBoundary)
		VMIME_TEST(testPrologEpilog)
		VMIME_TEST(testPrologEncoding)
		VMIME_TEST(testSuccessiveBoundaries)
		VMIME_TEST(testGenerate7bit)
		VMIME_TEST(testTextUsageForQPEncoding)
		VMIME_TEST(testParseVeryBigMessage)
	VMIME_TEST_LIST_END


	static const vmime::string extractComponentString
		(const vmime::string& buffer, const vmime::component& c)
	{
		return vmime::string(buffer.begin() + c.getParsedOffset(),
		                     buffer.begin() + c.getParsedOffset() +
							 c.getParsedLength()); 
	}

	static const vmime::string extractContents(const std::shared_ptr<const vmime::contentHandler> cts)
	{
		std::ostringstream oss;
		vmime::utility::outputStreamAdapter os(oss);

		cts->extract(os);

		return oss.str();
	}


	void testParse()
	{
		vmime::string str1 = "HEADER\r\n\r\nBODY";
		auto p1(vmime::factory<vmime::bodyPart>::create());
		p1->parse(str1);

		VASSERT_EQ("1", "HEADER\r\n\r\n", extractComponentString(str1, *p1->getHeader()));
		VASSERT_EQ("2", "BODY", extractComponentString(str1, *p1->getBody()));

		vmime::string str2 = "HEADER\n\nBODY";
		auto p2(vmime::factory<vmime::bodyPart>::create());
		p2->parse(str2);

		VASSERT_EQ("3", "HEADER\n\n", extractComponentString(str2, *p2->getHeader()));
		VASSERT_EQ("4", "BODY", extractComponentString(str2, *p2->getBody()));

		vmime::string str3 = "HEADER\r\n\nBODY";
		auto p3(vmime::factory<vmime::bodyPart>::create());
		//vmime::bodyPart p3;
		p3->parse(str3);

		VASSERT_EQ("5", "HEADER\r\n\n", extractComponentString(str3, *p3->getHeader()));
		VASSERT_EQ("6", "BODY", extractComponentString(str3, *p3->getBody()));
	}

	void testParseMissingLastBoundary()
	{
		vmime::string str =
			"Content-Type: multipart/mixed; boundary=\"MY-BOUNDARY\""
			"\r\n\r\n"
			"--MY-BOUNDARY\r\nHEADER1\r\n\r\nBODY1\r\n"
			"--MY-BOUNDARY\r\nHEADER2\r\n\r\nBODY2";

		auto p(vmime::factory<vmime::bodyPart>::create());
		p->parse(str);

		VASSERT_EQ("count", 2, p->getBody()->getPartCount());

		VASSERT_EQ("part1-body", "BODY1", extractContents(p->getBody()->getPartAt(0)->getBody()->getContents()));
		VASSERT_EQ("part2-body", "BODY2", extractContents(p->getBody()->getPartAt(1)->getBody()->getContents()));
	}

	void testGenerate()
	{
		auto p1(vmime::factory<vmime::bodyPart>::create());
		p1->getHeader()->getField("Foo")->setValue(vmime::string("bar"));
		p1->getBody()->setContents(vmime::factory<vmime::stringContentHandler>::create("Baz"));

		VASSERT_EQ("1", "Foo: bar\r\n\r\nBaz", p1->generate());
	}

	void testPrologEpilog()
	{
		const char testMail[] =
			"To: test@vmime.org\r\n"
			"From: test@vmime.org\r\n"
			"Subject: Prolog and epilog test\r\n"
			"Content-Type: multipart/mixed; \r\n"
			" boundary=\"=_boundary\"\r\n"
			"\r\n"
			"Prolog text\r\n"
			"--=_boundary\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"Part1\r\n"
			"--=_boundary--\r\n"
			"Epilog text";

		auto part(vmime::factory<vmime::bodyPart>::create());
		part->parse(testMail);

		VASSERT_EQ("prolog", "Prolog text", part->getBody()->getPrologText());
		VASSERT_EQ("epilog", "Epilog text", part->getBody()->getEpilogText());
	}

	// Test for bug fix: prolog should not be encoded
	// http://sourceforge.net/tracker/?func=detail&atid=525568&aid=3174903&group_id=69724
	void testPrologEncoding()
	{
		const char testmail[] =
			"To: test@vmime.org\r\n"
			"From: test@vmime.org\r\n"
			"Subject: Prolog encoding test\r\n"
			"Content-Type: multipart/mixed; \r\n"
			" boundary=\"=_+ZWjySayKqSf2CyrfnNpaAcO6-G1HpoXdHZ4YyswAWqEY39Q\"\r\n"
			"\r\n"
			"This is a multi-part message in MIME format. Your mail reader does not\r\n"
			"understand MIME message format.\r\n"
			"--=_+ZWjySayKqSf2CyrfnNpaAcO6-G1HpoXdHZ4YyswAWqEY39Q\r\n"
			"Content-Type: text/html; charset=windows-1251\r\n"
			"Content-Transfer-Encoding: quoted-printable\r\n"
			"\r\n"
			"=DD=F2=EE =F2=E5=EA=F1=F2=EE=E2=E0=FF =F7=E0=F1=F2=FC =F1=EB=EE=E6=ED=EE=E3=\r\n"
			"=EE =F1=EE=EE=E1=F9=E5=ED=E8=FF\r\n"
			"--=_+ZWjySayKqSf2CyrfnNpaAcO6-G1HpoXdHZ4YyswAWqEY39Q\r\n"
			"Content-Type: application/octet-stream; charset=windows-1251\r\n"
			"Content-Disposition: attachment; filename=FNS.zip\r\n"
			"Content-Transfer-Encoding: base64\r\n"
			"\r\n"
			"UEsDBB...snap...EEAAAAAA==\r\n"
			"--=_+ZWjySayKqSf2CyrfnNpaAcO6-G1HpoXdHZ4YyswAWqEY39Q--\r\n"
			"Epilog text";

		std::shared_ptr<vmime::message> msg =
			vmime::factory<vmime::message>::create();

		std::string istr(testmail);

		std::string ostr;
		//vmime::utility::outputStreamStringAdapter out(ostr); TODO shared
		auto out(vmime::factory<vmime::utility::outputStreamStringAdapter>::
			create(ostr));

		for (int i = 0 ; i < 10 ; ++i) {
			ostr.clear();

			msg->parse(istr);
			msg->generate(*out);

			istr = ostr;
		}

		VASSERT_EQ("prolog", 
			"This is a multi-part message in MIME format. Your mail reader"
		   " does not understand MIME message format.", 
		   msg->getBody()->getPrologText());
		VASSERT_EQ("epilog", "Epilog text", msg->getBody()->getEpilogText());
	}

	void testSuccessiveBoundaries()
	{
		vmime::string str =
			"Content-Type: multipart/mixed; boundary=\"MY-BOUNDARY\""
			"\r\n\r\n"
			"--MY-BOUNDARY\r\nHEADER1\r\n\r\nBODY1\r\n"
			"--MY-BOUNDARY\r\n"
			"--MY-BOUNDARY--\r\n";

		auto p(vmime::factory<vmime::bodyPart>::create());
		p->parse(str);

		VASSERT_EQ("count", 2, p->getBody()->getPartCount());

		VASSERT_EQ("part1-body", "BODY1", extractContents(p->getBody()->getPartAt(0)->getBody()->getContents()));
		VASSERT_EQ("part2-body", "", extractContents(p->getBody()->getPartAt(1)->getBody()->getContents()));
	}

	/** Ensure '7bit' encoding is used when body is 7-bit only. */
	void testGenerate7bit()
	{
		std::shared_ptr<vmime::plainTextPart> p1 =
			vmime::factory<vmime::plainTextPart>::create();
		p1->setText(vmime::factory<vmime::stringContentHandler>::create("Part1 is US-ASCII only."));

		std::shared_ptr<vmime::message> msg =
			vmime::factory<vmime::message>::create();
		p1->generateIn(msg, msg);

		std::shared_ptr<vmime::header> header1 = msg->getBody()->getPartAt(0)->getHeader();
		VASSERT_EQ("1", "7bit", header1->ContentTransferEncoding()->getValue()->generate());
	}

	void testTextUsageForQPEncoding()
	{
		std::shared_ptr<vmime::plainTextPart> part =
			vmime::factory<vmime::plainTextPart>::create();
		part->setText(vmime::factory<vmime::stringContentHandler>::create("Part1-line1\r\nPart1-line2\r\n\x89"));

		std::shared_ptr<vmime::message> msg =
			vmime::factory<vmime::message>::create();
		part->generateIn(msg, msg);

		std::shared_ptr<vmime::body> body = msg->getBody()->getPartAt(0)->getBody();
		std::shared_ptr<vmime::header> header = msg->getBody()->getPartAt(0)->getHeader();

		std::ostringstream oss;
		vmime::utility::outputStreamAdapter os(oss);
		body->generate(os, 80);

		VASSERT_EQ("1", "quoted-printable", header->ContentTransferEncoding()->getValue()->generate());

		// This should *NOT* be:
		//    Part1-line1=0D=0APart1-line2=0D=0A=89
		VASSERT_EQ("2", "Part1-line1\r\nPart1-line2\r\n=89", oss.str());
	}

	void testParseGuessBoundary()
	{
		// Boundary is not specified in "Content-Type" field
		// Parser will try to guess it from message contents.

		vmime::string str =
			"Content-Type: multipart/mixed"
			"\r\n\r\n"
			"--UNKNOWN-BOUNDARY\r\nHEADER1\r\n\r\nBODY1\r\n"
			"--UNKNOWN-BOUNDARY\r\nHEADER2\r\n\r\nBODY2\r\n"
			"--UNKNOWN-BOUNDARY--";

		auto p(vmime::factory<vmime::bodyPart>::create());
		p->parse(str);

		VASSERT_EQ("count", 2, p->getBody()->getPartCount());

		VASSERT_EQ("part1-body", "BODY1", extractContents(p->getBody()->getPartAt(0)->getBody()->getContents()));
		VASSERT_EQ("part2-body", "BODY2", extractContents(p->getBody()->getPartAt(1)->getBody()->getContents()));
	}

	void testParseVeryBigMessage()
	{
		// When parsing from a seekable input stream, body contents should not
		// be kept in memory in a "stringContentHandler" object. Instead, content
		// should be accessible via a "streamContentHandler" object.

		static const std::string BODY1_BEGIN = "BEGIN1BEGIN1BEGIN1";
		static const std::string BODY1_LINE = "BODY1BODY1BODY1BODY1BODY1BODY1BODY1BODY1BODY1BODY1BODY1BODY1BODY1";
		static const std::string BODY1_END = "END1END1";
		static const unsigned int BODY1_REPEAT = 35000;
		static const unsigned int BODY1_LENGTH =
			BODY1_BEGIN.length() + BODY1_LINE.length() * BODY1_REPEAT + BODY1_END.length();

		static const std::string BODY2_LINE = "BODY2BODY2BODY2BODY2BODY2BODY2BODY2BODY2BODY2BODY2BODY2BODY2BODY2";
		static const unsigned int BODY2_REPEAT = 20000;

		std::ostringstream oss;
		oss << "Content-Type: multipart/mixed; boundary=\"MY-BOUNDARY\""
		    << "\r\n\r\n"
		    << "--MY-BOUNDARY\r\n"
		    << "HEADER1\r\n"
		    << "\r\n";

		oss << BODY1_BEGIN;

		for (unsigned int i = 0 ; i < BODY1_REPEAT ; ++i)
			oss << BODY1_LINE;

		oss << BODY1_END;

		oss << "\r\n"
		    << "--MY-BOUNDARY\r\n"
		    << "HEADER2\r\n"
		    << "\r\n";

		for (unsigned int i = 0 ; i < BODY2_REPEAT ; ++i)
			oss << BODY2_LINE;

		oss << "\r\n"
		    << "--MY-BOUNDARY--\r\n";

		std::shared_ptr<vmime::utility::inputStreamStringAdapter> is =
			vmime::factory<vmime::utility::inputStreamStringAdapter>::create(oss.str());

		std::shared_ptr<vmime::message> msg =
			vmime::factory<vmime::message>::create();
		msg->parse(is, oss.str().length());

		std::shared_ptr<vmime::body> body1 = msg->getBody()->getPartAt(0)->getBody();
		std::shared_ptr<const vmime::contentHandler> body1Cts = body1->getContents();

		std::shared_ptr<vmime::body> body2 = msg->getBody()->getPartAt(1)->getBody();
		std::shared_ptr<const vmime::contentHandler> body2Cts = body2->getContents();

		vmime::string body1CtsExtracted;
		vmime::utility::outputStreamStringAdapter body1CtsExtractStream(body1CtsExtracted);
		body1Cts->extract(body1CtsExtractStream);

		VASSERT_EQ("1.1", BODY1_LENGTH, body1Cts->getLength());
		// VASSERT("1.2", body1Cts.dynamicCast <const
		// vmime::streamContentHandler>() != NULL); TODO shared
		VASSERT("1.2", std::dynamic_pointer_cast<const vmime::streamContentHandler>(body1Cts) != NULL);
		VASSERT_EQ("1.3", BODY1_LENGTH, body1CtsExtracted.length());
		VASSERT_EQ("1.4", BODY1_BEGIN, body1CtsExtracted.substr(0, BODY1_BEGIN.length()));
		VASSERT_EQ("1.5", BODY1_END, body1CtsExtracted.substr(BODY1_LENGTH - BODY1_END.length(), BODY1_END.length()));

		VASSERT_EQ("2.1", BODY2_LINE.length() * BODY2_REPEAT, body2Cts->getLength());
		// VASSERT("2.2", body2Cts.dynamicCast <const
		// vmime::streamContentHandler>() != NULL); TODO shared
		VASSERT("2.2", std::dynamic_pointer_cast<const vmime::streamContentHandler>(body2Cts) != NULL);
	}

VMIME_TEST_SUITE_END

