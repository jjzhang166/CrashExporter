
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "CppUnit/TestResult.h"


#include "CrashExporterTests.h"
#include "CrashRpt.h"

#include <iostream>
using namespace std;


CrashExporterTests::CrashExporterTests(const std::string& name): CppUnit::TestCase(name){}

CrashExporterTests::~CrashExporterTests(){}

void CrashExporterTests::setUp()
{

}

void CrashExporterTests::tearDown()
{

}


CppUnit::Test* CrashExporterTests::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("CrashExporterTests");

	CppUnit_addTest(pSuite, CrashExporterTests, Test_crUninstall);
	CppUnit_addTest(pSuite, CrashExporterTests, Test_crInstall_null);
	CppUnit_addTest(pSuite, CrashExporterTests, Test_crInstall_wrong_cb);
	CppUnit_addTest(pSuite, CrashExporterTests, Test_crInstallW_zero_info);
	CppUnit_addTest(pSuite, CrashExporterTests, Test_crInstallA_zero_info);
	CppUnit_addTest(pSuite, CrashExporterTests, Test_crInstallToCurrentThread);
	CppUnit_addTest(pSuite, CrashExporterTests, Test_crAddScreenshot);
	

	return pSuite;
}


void CrashExporterTests::Test_crUninstall()
{
	// Call crUninstall - should fail, because crInstall should be called first

	int nUninstallResult = crUninstall();
	assert(nUninstallResult!=0); 

	// And another time... 
	int nUninstallResult2 = crUninstall();
	assert(nUninstallResult2!=0); 
}

void CrashExporterTests::Test_crInstall_null()
{   
	// Test crInstall with NULL info - should fail

	int nInstallResult = crInstallW(NULL);
	assert(nInstallResult!=0);

	int nInstallResult2 = crInstallA(NULL);
	assert(nInstallResult2!=0); 
}

void CrashExporterTests::Test_crInstall_wrong_cb()
{   
	// Test crInstall with wrong cb parameter - should fail

	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = 1000;

	int nInstallResult = crInstall(&info);
	assert(nInstallResult!=0);
}

void CrashExporterTests::Test_crInstallW_zero_info()
{   
	// Test crInstallW with zero info

	CR_INSTALL_INFOW infoW;
	memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
	infoW.cb = sizeof(CR_INSTALL_INFOW);

	int nInstallResult = crInstallW(&infoW);
	assert(nInstallResult==0);

	crUninstall();  
}

void CrashExporterTests::Test_crInstallA_zero_info()
{   
	// Test crInstallA with zero info

	CR_INSTALL_INFOA infoA;
	memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
	infoA.cb = sizeof(CR_INSTALL_INFOA);

	int nInstallResult = crInstallA(&infoA);
	assert(nInstallResult==0);

	crUninstall();  
}

void CrashExporterTests::Test_crInstallA_twice()
{   
	// Call crInstallA two times - the second one should fail

	CR_INSTALL_INFOA infoA;
	memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
	infoA.cb = sizeof(CR_INSTALL_INFOA);

	int nInstallResult = crInstallA(&infoA);
	assert(nInstallResult==0);

	int nInstallResult2 = crInstallA(&infoA);
	assert(nInstallResult2!=0);

	crUninstall();
}

void CrashExporterTests::Test_crInstallToCurrentThread()
{ 
	// Call before install - must fail
	int nResult = crInstallToCurrentThread(0);
	assert(nResult != 0);


	// Install crash handler for the main thread

	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);

	int nInstResult = crInstall(&info);
	assert(nInstResult == 0);

	// Call in the main thread - must fail
	int nResult3 = crInstallToCurrentThread(0);
	assert(nResult3 != 0);

	crUninstall();  
}


void CrashExporterTests::Test_crAddScreenshot()
{   
	// Should fail, because crInstall() should be called first
	int nResult = crAddScreenshot(CR_AS_VIRTUAL_SCREEN);
	assert(nResult != 0);

	// Install crash handler
	CR_INSTALL_INFOW infoW;
	memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
	infoW.cb = sizeof(CR_INSTALL_INFOW);

	int nInstallResult = crInstallW(&infoW);
	assert(nInstallResult == 0);

	// Should succeed
	int nResult2 = crAddScreenshot(CR_AS_VIRTUAL_SCREEN);
	assert(nResult2==0);

	// Call twice - should succeed
	int nResult3 = crAddScreenshot(CR_AS_MAIN_WINDOW);
	assert(nResult3 == 0);

	// Uninstall
	crUninstall();  
}