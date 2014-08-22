//
// WinDriver.cpp
//


#include "CppUnit/WinTestRunner.h"
#include "CppUnit/TestRunner.h"
#include "CrashExporterTestsSuite.h"

class TestDriver: public CppUnit::WinTestRunnerApp
{
	void TestMain()
	{
		_CrtSetBreakAlloc(130);
		CppUnit::WinTestRunner runner;
		runner.addTest(CrashExporterTestsSuite::suite());
		runner.run();
	}
};


TestDriver theDriver;
