#CrashExporter

#summary Frequently Asked Questions

<wiki:toc max_depth="2" />

----


== `CrashExporter`��ʲô��Ϊʲô����`CrashExporter`�� ==

`CrashExporter`�ǻ��ڿ�Դ��Ŀcrashrpt�����ϸ���ġ�����Windowsƽ̨��һ�����������쳣��Ϣ���������
ԭcrashrpt����ͨ���ʼ�����http���ʹ��󱨸棬API�ӿ�̫�ࡢ̫���ӣ��������˴����������ߡ�������Ŀʵ�ʣ�ҲΪ�˼��ٴ����Ķ�����ȥ���˷��͹��ܲ�����˵������ܣ������˸���Ȥ�Ķ�ջ��Ϣ��¼���ܡ�Ϊ���ڸ������������ԭ��Ŀ���̺��Լ��Ĺ��̣�����Ϊ`CrashExporter`��


== `CrashExporter`������ѹ�������涼��ʲô�� ==
һ���������ļ���
  * .dmp��׺��minidump�ļ�������ͨ��windbg���߲鿴����Ҫpdb�ļ�һ��ʹ�ã���������.ecxr��!analyze -v��������ֱ����vs2010�򿪡�
  * .txt��׺��Ϊ��ջ��ӡ����¼���쳣ʱ�����ģ����úͶ�ջ��Ϣ��ӡ�����Զ�λ�������ļ���ĳһ�С�
  * .xml��׺��Ϊϵͳ��Ϣ�ļ�����¼�˲���ϵͳ��ص���Ϣ�ʹ���ļ���һЩ��Ϣ�ȣ����ļ�����ͨ��crAddProperty�ӿڽ��ж��ơ�
���������crAddScreenshot�ӿڣ�ѹ�����л�����
  * .png��׺����Ļ�����ļ���ץȡ����ʱ��Ļ�Ľ�����
���⣬�����ʹ�ýӿ�CrAddFile���������ļ�һ�𱣴浽ѹ������


== ʹ��`CrashExporter`ʱ��������ô���ã� ==

��Ҫ�Ƕ�release�������������á�����Properties->C/C++->General�е�Debug info��ѡ��Program Database����Σ�����Properties->C/C++->Code Generation�е�Use run-time libraryΪMultithreaded DLL;���Properties->C/C++->link�й�ѡ��Generate debug info��



== MFC ��������ΰ�װ`CrashExporter`�� ==

�������дCWinApp::Run()����Run�����ﰲװ`CrashExporter`�������������£�
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

== ����ڶ��̳߳�����ʹ�ã� ==

һ��ֱ�ӵ���CrInstall���ɻ�ȡ�����߳��쳣��Ϣ����Ҳ������ÿ���̵߳���ʼ����crInstallToCurrentThread2���ֱ��ȡ�̵߳��쳣��Ϣ�����Ǳ��������߳��˳��ǵ���crUninstallFromCurrentThread��




== �������е�ĳ���߳��쳣���������߳���Ȼalive��Ҳû�б�����Ϣ�����������������ô���� ==

��ʱ�����ͨ��crGenerateErrorReport�ӿ��ֶ���ȡ�쳣��Ϣ��

== CrashExporter֧�ֳ����Զ�����ô�� ==

֧�֡�
����������Ҫ����������1������������Ҫ����1���ӡ�2����Ҫ������CR_INSTALL_INFO��dwFlags|CR_INST_APP_RESTART��



== �����򷢲�ʱ������Ҫ�������Щ���� ==

��������Ҫ���������������
  * CrashRptXXXX.dll
  * CrashExporterXXXX.exe
  * dbghelp.dll
����XXXΪCrashExporter�İ汾�š�
���⣬��Ҳ����ȷ���ͻ�������װ��vs2010���п⡣û�еĻ��������ÿͻ���װvs2010���п⣬���ߴ��msvcp100.dll��msvcr100.dll��

== �ҵĳ�����һ��DLL�������������ôʹ��CrashExporter�� ==

�������DLL�����ʼ��֮ǰ����CrashExporter������DllMain()�� 
���ǣ�����DLL��������Ҳ���ܼ�����CrashExporter����������»����ε���������쳣��⡣��������£�����Ҫ��Ϊ�����ˣ����������������м���CrashExporter��


== ʹ��`CrashExporter`�᲻��Ӱ���ҳ�������ܣ� ==

һ���ǲ���ġ�
��ΪCrashExporter�����ں�ִ̨���κ�����Ҳ����ռ�ö�����ڴ档CrashExporter�������ڳ��������ʱ�򱻵��á����ǣ�CrashExporter��Ӱ������쳣�˳�����ΪCrashExporter��Ҫ���м���symbols �� ������ջ����Ϊ��

== `CrashExporter`���Ի�ȡ���е��쳣�� ==

���Ի�ȡ������������쳣��
��Ϊ��ȫԭ��΢���ǲ�������������ĳЩ�쳣�ġ�
����һ�������
{{{
CPoint* pt = new CPoint;
delete pt;
delete pt;
}}}
��������Ϊ��������ģ����������ʱ��ᵼ�³���������������ʱ��ȴ�ǿ��Լ�����������������������쳣��CrashExporter�ǲ��ܻ�ȡ�ġ�
��ˣ����ǽ���ɾ��ָ���ڴ�ʱ�������������ƺꡣ
{{{
#define SAFE_DELETE(ptr) { delete(ptr); (ptr)=NULL; }
}}}

== ����ϸ�����һ�����ö�����ȷ�ģ�������Ȼ���ܻ�ȡ���쳣��Ϣ������Ϊʲô�� ==

һ�����ܵ�ԭ�������쳣�ĳ�������`CrashExporter`������Դ�Ļ��ᶼû�����������������һ��ѭ���ݹ顣




== ��û���쳣��Ϣָ��`CrashExporter`���������������������쳣��ջ���ٵ�`Crashrpt.dll`�� ==

�����еġ�
���쳣����ʱ��Crashrpt.dll��Ҫ����쳣ָ��Ľṹ�����ָ��ṹһ���ɲ���ϵͳ���䡣���ǣ���ʱ�����ָ��ṹ�᲻���ڣ�������Ч�Ĳ���������ʱ��Crashrpt.dll����ݵ�ǰCPU�Ĵ���״̬����ָ��ṹ����������£�������쳣��ջ�и��ٵ�Crashrpt.dll�ˡ�


== `����CrashExporter����ô�����ģ�` ==

CrashExporter������core��ɡ��ֱ�ΪCrashRpt.dll �� CrashExporter.exe
CrashRpt.dll������ȡ������쳣��Ϣ���ܣ�CrashExporter.exe����д�ļ�����Ļ������ѹ������Ĺ��ܵȡ�
���������׳����д�ļ��ȹ��ܷ��뵽������CrashExporter.exe�У����ͻ�����ֻ�����CrashRpt.dll�����ַ�ռ䣬���������˿ͻ�������ΪCrashExporter�����쳣�Ŀ����ԡ�
CrashRpt.dll �� CrashExporter.exeͨ�������ڴ�ķ�ʽ�������ݡ�

== `Ӧ����ʲô���͵�minidump��`==
������MiniDumpNormal����ΪMiniDumpNormal���������Ǹ���Ȥ��ÿ���̶߳�ջ��Ϣ��


== `ʲô���쳣��` ==

�쳣������˵�ؼ��Դ��󡢱�����һ����˵����ĳ������в��������Ӷ����ò�ֹͣ���е����������˵�������ĳ������һ����Ч���ڴ��ַ����NULLָ�룩���޷�����һ��Buffer���ڴ治�㣩��C���Կ������ʱ��C run-time libraries��CRT������һ�����󣬲�����Ҫ��������ֹͣ���еȵȣ���Щ����¶������һ���쳣��


== `��Щ������ᵼ���쳣��` ==

  * ���������һ��Ƿ����ڴ��ַ������NULLָ�룩.

  * �����޵ݹ��У�ջ���.

  * ������ݱ�д��һƬС������

  * C++���еĴ��麯��������

  * �ڴ��޷����䣨�ڴ治�㣩

  * ��C++��ϵͳ�����д���Ƿ��Ĳ���
  
  * C���п�����������Ҫֹͣ��������
 ��Ҫ���������͵��쳣��SEH�쳣���ṹ���쳣�����ͱ�׼C++�쳣

== ��������SEH�쳣�ͱ�׼C++�쳣�� ==


�ṹ���쳣����ϵͳ���ɲ���ϵͳ�ṩ�ģ�����ζ�����е�Windows�����ܲ����ʹ���SEH�쳣����SEH�쳣�����ΪC������Ƶģ�����C++��Ҳ����ʹ�á�

SEH�쳣��ͨ�� __try{} __except(){} �����Ľṹ������ġ������е� main() �����ͱ������Ľṹ��Χ�ţ���������û�б������SEH�쳣Ĭ�϶��ᱻ���񣬻���ҽ���ᵯ������
SEH�쳣��������Visual C++������ָ���ġ������Ҫд������ǿ�Ĵ��룬��Ӧ����SEH�ṹ���˼���#ifdef/#endif������˵���SEHû�����壬��ôSEH�Ĵ���Ͳ�Ҫ������룩��
ʾ���������£�
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

C++��ʽ���쳣����ϵͳ��������C����ʱ���ṩ�ģ�����ζ��ֻ��C++������Բ����ʹ��������쳣����C++��ʽ�쳣������ͨ��try{} catch{}�����Ľṹ������ġ�
ʾ���Ĵ������£�
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

