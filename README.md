#CrashExporter
CrashExporter是基于开源项目crashrpt基础上改造的、基于Windows平台的一个轻量级的异常信息导出组件。 
原crashrpt可以通过邮件或者http发送错误报告，API接口太多、太复杂，并搭配了大量分析工具。
由于项目实际，也为了减少代码阅读量，去除了发送功能并添加了导出功能，增加了感兴趣的堆栈信息记录功能。
因为只保留了导出功能，因而命名为CrashExporter。