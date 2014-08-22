

#ifndef CrashExporterTests_INCLUDED
#define CrashExporterTests_INCLUDED
#include <Windows.h>

#include "CppUnit/TestCase.h"
class CrashExporterTests: public CppUnit::TestCase
{
public:
	CrashExporterTests(const std::string& name);
	~CrashExporterTests();

	void setUp();
	void tearDown();
	static CppUnit::Test* suite();

	void Test_crUninstall();
	void Test_crInstall_null();
	void Test_crInstall_wrong_cb();
	void Test_crInstallW_zero_info();
	void Test_crInstallA_zero_info();
	void Test_crInstallA_twice();
	void Test_crInstallToCurrentThread();
	void Test_crAddScreenshot();
	void Test_crGenerateErrorReport();

	
private:
};


#endif // CrashExporterTests_INCLUDED
