#ifndef _STL_LOG4C_H__
#define _STL_LOG4C_H__

class ILog4C
{
public:
	typedef enum
	{
		Debug	= 0x1000,
		Warning = 0x0100,
		Infor	= 0x0010,
		Error	= 0x0001
	}elog_level;
public:
	ILog4C(){}
	virtual ~ILog4C(){}
public:
	virtual void setRoot(const char* directory) = 0;
	virtual void setFileSize(int size) = 0;
	virtual void setTraceLevel(int level) = 0;
	virtual void setExpiryDate(int month) = 0;
	virtual void log(elog_level level, const char* layer, const char* file, int lineno, const char* func, int len, const char* varName, const unsigned char* hexStream) = 0;
	virtual void log(elog_level level, const char* layer, const char* file, int lineno, const char* func, const char* format, ...) = 0;
};

#endif	