#CrashExporter

CrashExporter是基于开源项目crashrpt(https://code.google.com/p/crashrpt/) 基础上改造的、基于Windows平台的轻量级异常导出组件。 

CrashExporter与crashrpt的区别是：CrashExporter只保留了crashrpt的导出dmp和抓屏功能，并增加了堆栈打印。

支持VC6~VS2010

使用请参考demos目录代码


=================================================================

当程序崩溃后，会在程序执行的目录的crashrpt下，增加一个以崩溃时间命名的文件夹。

文件夹里有三个文件：

	.dmp后缀的minidump文件；可以通过windbg工具查看（需要pdb文件一起使用，常用命令.ecxr和!analyze -v），或者直接用vs2010打开。

	.txt后缀的为堆栈打印；记录了异常时系统信息和堆栈信息打印，可以定位到代码文件的某一行。

	.png后缀的屏幕截屏文件；抓取崩溃时的屏幕图像。






