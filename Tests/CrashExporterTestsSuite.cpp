
#include "CrashExporterTestsSuite.h"
#include "CrashExporterTests.h"

CppUnit::Test* CrashExporterTestsSuite::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("CrashExporterTestsSuite");

	pSuite->addTest(CrashExporterTests::suite());

	return pSuite;
}
