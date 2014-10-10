#CrashExporter

#summary Frequently Asked Questions

<wiki:toc max_depth="2" />

----


== `CrashExporter`是什么？为什么起名`CrashExporter`？ ==

`CrashExporter`是基于开源项目crashrpt基础上改造的、基于Windows平台的一个轻量级的异常信息导出组件。
原crashrpt可以通过邮件或者http发送错误报告，API接口太多、太复杂，并搭配了大量分析工具。由于项目实际，也为了减少代码阅读量，去除了发送功能并添加了导出功能，增加了感兴趣的堆栈信息记录功能。为了在改造过程中区分原项目工程和自己的工程，命名为`CrashExporter`。


== `CrashExporter`导出的压缩包里面都有什么？ ==
一般有三个文件：
  * .dmp后缀的minidump文件。可以通过windbg工具查看（需要pdb文件一起使用，常用命令.ecxr和!analyze -v），或者直接用vs2010打开。
  * .txt后缀的为堆栈打印。记录了异常时程序的模块调用和堆栈信息打印，可以定位到代码文件的某一行。
  * .xml后缀的为系统信息文件。记录了操作系统相关的信息和打包文件的一些信息等，该文件可以通过crAddProperty接口进行定制。
如果调用了crAddScreenshot接口，压缩包中还包含
  * .png后缀的屏幕截屏文件。抓取崩溃时屏幕的截屏。
另外，你可以使用接口CrAddFile，将其他文件一起保存到压缩包。


== 使用`CrashExporter`时，工程怎么配置？ ==

主要是对release工程三个的配置。首先Properties->C/C++->General中的Debug info，选择Program Database；其次，设置Properties->C/C++->Code Generation中的Use run-time library为Multithreaded DLL;最后，Properties->C/C++->link中勾选上Generate debug info。



== MFC 程序中如何安装`CrashExporter`？ ==

你可以重写CWinApp::Run()，在Run函数里安装`CrashExporter`。代码类似如下：
{{{
int CYourApp::Run() 
{
  // Call your crInstall code here ...

  BOOL bRun;
  BOOL bExit=FALSE;
  while(!bExit)
  {
    bRun= CWinApp::Run();
    bExit=TRUE;
  }
  return bRun;
}
}}}

== 如何在多线程程序中使用？ ==

一般直接调用CrInstall即可获取所有线程异常信息。你也可以在每个线程的起始调用crInstallToCurrentThread2来分别获取线程的异常信息，但是别忘记在线程退出是调用crUninstallFromCurrentThread。




== 当程序中的某个线程异常，但是主线程仍然alive，也没有崩溃信息产生。这种情况下这么处理？ ==

这时候可以通过crGenerateErrorReport接口手动获取异常信息。

== CrashExporter支持程序自动重启么？ ==

支持。
程序重启需要两个条件：1）程序运行需要超过1分钟。2）需要设置了CR_INSTALL_INFO中dwFlags|CR_INST_APP_RESTART。



== 当程序发布时，我需要打包进哪些东西 ==

你至少需要打包进三个东西：
  * CrashRptXXXX.dll
  * CrashExporterXXXX.exe
  * dbghelp.dll
其中XXX为CrashExporter的版本号。
另外，你也必须确保客户机器安装了vs2010运行库。没有的话，可以让客户安装vs2010运行库，或者打包msvcp100.dll和msvcr100.dll。

== 我的程序是一个DLL，这种情况下怎么使用CrashExporter。 ==

你可以在DLL程序初始化之前加载CrashExporter，比如DllMain()。 
但是，调用DLL的主程序也可能加载了CrashExporter，这样情况下会屏蔽掉主程序的异常检测。这种情况下，就需要人为控制了，建议是在主程序中加载CrashExporter。


== 使用`CrashExporter`会不会影响我程序的性能？ ==

一般是不会的。
因为CrashExporter不会在后台执行任何任务，也不会占用多余的内存。CrashExporter仅仅会在程序崩溃的时候被调用。但是，CrashExporter会影响程序异常退出，因为CrashExporter需要进行加载symbols 和 分析堆栈等行为。

== `CrashExporter`可以获取所有的异常吗？ ==

可以获取大多数常见的异常。
因为安全原因，微软是不允许我们拦截某些异常的。
还有一种情况：
{{{
CPoint* pt = new CPoint;
delete pt;
delete pt;
}}}
这种是因为堆损坏引起的，这种情况有时候会导致程序立即崩溃，有时候却是可以继续工作。对于这种随机的异常，CrashExporter是不能获取的。
因此，我们建议删除指针内存时，采用这样类似宏。
{{{
#define SAFE_DELETE(ptr) { delete(ptr); (ptr)=NULL; }
}}}

== 我仔细检查了一切配置都是正确的，但是仍然不能获取到异常信息，这是为什么？ ==

一个可能的原因是你异常的程序连给`CrashExporter`分配资源的机会都没给，比如你程序中有一个循环递归。




== 有没有异常信息指向`CrashExporter`本身出错的情况？比如跟踪异常堆栈跟踪到`Crashrpt.dll`。 ==

这是有的。
当异常产生时，Crashrpt.dll需要检查异常指针的结构，这个指针结构一般由操作系统分配。但是，有时候这个指针结构会不存在（比如无效的参数），这时候Crashrpt.dll会根据当前CPU寄存器状态分配指针结构。这种情况下，你会在异常堆栈中跟踪到Crashrpt.dll了。


== `简述CrashExporter是怎么工作的？` ==

CrashExporter由两个core组成。分别为CrashRpt.dll 和 CrashExporter.exe
CrashRpt.dll包含获取程序的异常信息功能，CrashExporter.exe包含写文件、屏幕截屏、压缩打包的功能等。
这样把容易出错的写文件等功能分离到单独的CrashExporter.exe中，而客户程序只会加载CrashRpt.dll到其地址空间，这样减少了客户程序因为CrashExporter出现异常的可能性。
CrashRpt.dll 和 CrashExporter.exe通过共享内存的方式传递数据。

== `应该用什么类型的minidump？`==
建议用MiniDumpNormal，因为MiniDumpNormal包含了我们感兴趣的每个线程堆栈信息。


== `什么是异常？` ==

异常（或者说关键性错误、崩溃）一般来说是你的程序运行不正常，从而不得不停止运行的情况。比如说，如果你的程序访问一块无效的内存地址（如NULL指针）、无法分配一个Buffer（内存不足）、C语言库的运行时（C run-time libraries，CRT）发现一个错误，并且需要程序立即停止运行等等，这些情况下都会产生一个异常。


== `哪些情况都会导致异常？` ==

  * 程序访问了一块非法的内存地址（比如NULL指针）.

  * 在无限递归中，栈溢出.

  * 大块数据被写入一片小缓冲区

  * C++类中的纯虚函数被调用

  * 内存无法分配（内存不足）

  * 向C++的系统函数中传入非法的参数
  
  * C运行库遇到错误，需要停止程序运行
 主要有两种类型的异常：SEH异常（结构化异常处理）和标准C++异常

== 简述捕获SEH异常和标准C++异常。 ==


结构化异常处理系统是由操作系统提供的（这意味着所有的Windows程序都能产生和处理SEH异常）。SEH异常最初是为C语言设计的，但在C++中也可以使用。

SEH异常是通过 __try{} __except(){} 这样的结构来处理的。程序中的 main() 函数就被这样的结构包围着，所以所有没有被处理的SEH异常默认都会被捕获，华生医生会弹出来。
SEH异常处理是由Visual C++编译器指定的。如果你要写兼容性强的代码，你应该在SEH结构两端加上#ifdef/#endif（就是说如果SEH没被定义，那么SEH的代码就不要参与编译）。
示例代码如下：
{{{
int* p = NULL;   // pointer to NULL
__try
{
    // Guarded code
    *p = 13; // causes an access violation exception
}
__except(EXCEPTION_EXECUTE_HANDLER) // Here is exception filter expression
{
    // Here is exception handler
    // Terminate program
    ExitProcess(1);
}
}}}

C++形式的异常处理系统是由是由C运行时库提供的（这意味着只有C++程序可以产生和处理这种异常）。C++形式异常处理是通过try{} catch{}这样的结构来处理的。
示例的代码如下：
{{{
try
{
    throw 20;
}
catch (int e)
{
    cout << "An exception occurred. Exception Nr. " << e << endl;
}
}}}

