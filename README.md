#CrashExporter

### 1.	CrashExporter��ʲô�� 
CrashExporter�ǻ��ڿ�Դ��Ŀcrashrpt(https://code.google.com/p/crashrpt/) �����ϸ���ġ�����Windowsƽ̨���������쳣���������
ԭcrashrpt ��Ҫ��Զ�˵ı�����Ϣѹ����ͨ�����緢�͵����أ�����������ϣ��������Ϣֻ�ڱ������ɡ�ͬʱcrashrpt û���������Ǹ���Ȥ�Ķ�ջ��ӡ�ļ���
`CrashExporter`��`CrashExporter`�������ǣ�`CrashExporter`ֻ������crashrpt�ĵ���dmp��ץ�����ܣ��������˶�ջ��ӡ���ܡ�

### 2.	��������������ʲô��
����������󣬻��ڳ���ִ�е�crashrptĿ¼�£�����һ���Ա���ʱ���������ļ��С�
�ļ������������ļ���
  * crashdump.dmpΪminidump�ļ�������ʹ��windbg���߲鿴����Ҫpdb�ļ�һ��ʹ�ã���������.ecxr��!analyze -v��������ֱ����vs2010�򿪡�
  * crashinfo.txtΪ��ջ��ӡ����¼���쳣ʱϵͳ��Ϣ�Ͷ�ջ��Ϣ��ӡ�����Զ�λ�������ļ���ĳһ�С�
  * screenshot0.pngΪ��Ļ�����ļ���ץȡ����ʱ����Ļͼ��

### 3.	ʹ��CrashExporterʱ��������ô���ã�
��Ҫ��release���̵����á�
����VC6���̣�
  * Properties->C/C++->General�е�Debug info��ѡ��Program Database;
  * ����Properties->C/C++->Code Generation�е�Use run-time libraryΪMultithreaded DLL;
  * Properties->C/C++->link�й�ѡ��Generate debug info��

����vs2010���̣�
  * ʹ�ö��߳�DLL(/MD)
      ���÷������һ����̣�ѡ������->C/C++->�������ɣ��ڵ�ǰҳ�ġ����п⡱��ѡ�񡰶��߳� DLL(/MD)����
  * ����debugging symbols (PDB�ļ�)
      ���÷������һ����̣�ѡ������->C/C++->���棬�ڵ�ǰҳ��������Ϣ��ʽ����ѡ�񡰳������ݿ�(/Zi)����
  * �����������ԡ����ɵ�����Ϣ������
      ���÷������һ����̣�ѡ������->������->���ԣ��ڵ�ǰҳ�����ɵ�����Ϣ����ѡ����(/DEBUG)����

### 4.	MFC ���������ʹ��CashExporter��
�������дCWinApp::Run()����Run�����ﰲװCrashExporter�������������£�

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

### 5.	����ڶ��̳߳�����ʹ�ã�
һ��ֱ�ӵ���CrInstall���ɻ�ȡ�����߳��쳣��Ϣ����Ҳ������ÿ���̵߳���ʼ����crInstallToCurrentThread���ֱ��ȡ�̵߳��쳣��Ϣ�����Ǳ��������߳��˳��ǵ���crUninstallFromCurrentThread��

### 6.	�������ˣ����������ô����
��ʱ�����ͨ��crGenerateErrorReport�ӿ��ֶ���ȡ�쳣��Ϣ��

### 7.	CrashExporter֧�ֳ����Զ�����ô��
֧�֡� ����������Ҫ����������
  * ����������Ҫ����1���ӡ�
  * ��Ҫ������CR_INSTALL_INFO��dwFlags|CR_INST_APP_RESTART��

### 8.	�����򷢲�ʱ������Ҫ�������Щ������
��������Ҫ���������������
  * CrashRpt.dll
  * CrashExporter.exe
  * dbghelp.dll
���⣬��Ҳ����ȷ���ͻ�������װ��vs2010���п⡣û�еĻ��������ÿͻ���װvs2010���п⣬���ߴ��msvcp100.dll��msvcr100.dll��

### 9.	�ҵĳ�����һ��DLL�������������ôʹ��CrashExporter��
�������DLL�����ʼ��֮ǰ����CrashExporter������DllMain()�� ���ǣ�����DLL��������Ҳ���ܼ�����CrashExporter����������»����ε����̵߳ı�����Ϣ��ȡ�����ֳ�ͻ������Ҫ��Ϊ�����ˣ����������������м���CrashExporter��

### 10.	ʹ��CrashExporter�᲻��Ӱ���ҳ�������ܣ�
���ᣡ��ΪCrashExporter�����ں�ִ̨���κ�����Ҳ����ռ�ö�����ڴ档CrashExporter�������ڳ��������ʱ�򱻵��á����ǣ�CrashExporter���ӳ��쳣�����˳�ʱ�䣬��ΪCrashExporter��Ҫ����symbols��������ջ��дdmp�͵��ö�ջ�ļ���

### 11.	CrashExporter���Ի�ȡ���е��쳣��
���Ի�ȡ������������쳣�� ��Ϊ��ȫԭ��΢���ǲ�������������ĳЩ�쳣�ġ� ����һ�������
	CPoint* pt = new CPoint;
	delete pt;
	delete pt;
��������Ϊ��������ģ����������ʱ��ᵼ�³���������������ʱ��ȴ�ǿ��Լ�����������������������쳣��CrashExporter�ǲ��ܻ�ȡ�ġ� ��ˣ����ǽ���ɾ��ָ���ڴ�ʱ�������������ƺꡣ
	#define SAFE_DELETE(ptr) { delete(ptr); (ptr)=NULL; }

### 12.	����ϸ�����һ�����ö�����ȷ�ģ�������Ȼ���ܻ�ȡ���쳣��Ϣ������Ϊʲô��
һ�����ܵ�ԭ�������쳣�ĳ�������CrashExporter������Դ�Ļ��ᶼû�����������������һ��ѭ���ݹ顣

### 13.	��û���쳣��Ϣָ��CrashExporter���������������������쳣��ջ���ٵ�Crashrpt.dll��
�еġ� ���쳣����ʱ��Crashrpt.dll��Ҫ����쳣ָ��Ľṹ�����ָ��ṹһ���ɲ���ϵͳ���䡣���ǣ���ʱ�����ָ��ṹ�᲻���ڣ�������Ч�Ĳ���������ʱ��Crashrpt.dll����ݵ�ǰCPU�Ĵ���״̬����ָ��ṹ����������£�������쳣��ջ�и��ٵ�Crashrpt.dll�ˡ�

### 14.	����CrashExporterԭ��
CrashExporter������core��ɡ��ֱ�ΪCrashRpt.dll �� CrashExporter.exe��

CrashRpt.dll�����ȡ������쳣��Ϣ�������������ڴ棻CrashExporter.exe����ӹ����ڴ��л�ȡ�쳣��Ϣ��дdmp�Ͷ�ջ�����ļ���������Ļ������ ��������Ŀ���ǰ����׳����д�ļ��Ȳ������뵽CrashExporter.exe�У�������ֻ�����CrashRpt.dll�����ַ�ռ䣬������������������ΪCrashExporter�����쳣�Ŀ����ԡ� 

CrashRpt.dll �� CrashExporter.exeʹ�ù����ڴ洫�����ݡ�

### 15.	Ӧ����ʲô���͵�minidump��
������MiniDumpNormal����ΪMiniDumpNormal���������Ǹ���Ȥ��ÿ���̶߳�ջ��Ϣ��

### 16.	��Щ������ᵼ���쳣��
	���������һ��Ƿ����ڴ��ַ������NULLָ�룩.
	�����޵ݹ��У�ջ���.
	������ݱ�д��һƬС������
	C++���еĴ��麯��������
	�ڴ��޷����䣨�ڴ治�㣩
	��C++��ϵͳ�����д���Ƿ��Ĳ���
	C���п�����������Ҫֹͣ��������
	��Ҫ���������͵��쳣��SEH�쳣���ṹ���쳣�����ͱ�׼C++�쳣


