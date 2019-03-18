#ifndef __LOG4C_MANAGER_HPP__
#define __LOG4C_MANAGER_HPP__

#include <stl/define.hpp>
#include <stl/time.hpp>
#include <stl/os/path.hpp>
#include <stl/os/file.hpp>

using namespace stl::time;
using namespace stl::os;

namespace logger
{
    class CManager
    {
        const int   MAX_LOG_PATH = 260;
        const char* LOG_APPEND_FIX = "log4c";
    #ifdef __os_windows__
		const char* SEPRATOR = "\\";
    #else
		const char* SEPRATOR = "/";
    #endif
    public:
        explicit CManager()
        : file_size_(100)
        , expiry_month_(1)
        {  
        }
        virtual ~CManager(){}
        
        void setFileSize(int size)
        {
            file_size_ = (size<0 || size>200)?100:size;
        }

        void setExpiryDate(int month)
        {
            expiry_month_ = (month<=0 || month>5)?5:month;
        }

        void clearExpiryDirectory(const std::string& dir, const tmExtend& time)
        {
            std::vector<std::string> traversor;
            path::traverse_only_current(dir, traversor);
            if (traversor.empty())
            {
                return;
            }
            std::unique_ptr<char[]> currenPath(new char[MAX_LOG_PATH]());
            int lastYear, lastMonth;
            if(time.tm_mon < expiry_month_)
            {
                lastMonth = time.tm_mon - expiry_month_ + 12 + 1;
                lastYear = time.tm_year - 1;
            }
            else
            {
                lastMonth = time.tm_mon - expiry_month_ + 1;
                lastYear = time.tm_year;
            }
            sprintf(&currenPath[0], "%04d-%02d", lastYear, lastMonth);
            for (auto path : traversor)
            {
                if (file::name(path) < &currenPath[0])
                {
                    path::rmdir(path);
                }
            }
        }

        bool isRename(const std::string& file)
        {
            auto fileSize = io::GetFileSize(file);
            if(fileSize > file_size_*1024*1024)
            {
                std::unique_ptr<char[]> tmp(new char[MAX_LOG_PATH]());
                for (int i=0; ; i++)
                {
                    memset(&tmp[0], 0x00, MAX_LOG_PATH);
                    sprintf(&tmp[0], "%s_%d.%s", file.substr(0, file.rfind('.')).c_str(), i, LOG_APPEND_FIX);
                    if (0 != access(&tmp[0], F_OK))
                    {
                        rename(file.c_str(), &tmp[0]);
                        return true;
                    }
                }
            }
            return false;
        }

        std::string createLogDirectory(const std::string& path, const tmExtend& time)
        {
            std::unique_ptr<char[]> buffer(new char[MAX_LOG_PATH]());
            sprintf(&buffer[0], "%s%s%04d-%02d%s%02d-%02d", path.c_str(), SEPRATOR, time.tm_year, time.tm_mon, SEPRATOR, time.tm_mon, time.tm_mday);
            if(0 != access(&buffer[0], F_OK))
            {
                //delete or check the expired directory while only ready to create new directory
                clearExpiryDirectory(path, time);
                path::mkdir(&buffer[0]);
            }
            return (std::string)&buffer[0];
        }

        bool detectDateChanged(const tmExtend& cur, const tmExtend& pre)
        {
            return (0!=memcmp(&cur.tm_mday, &pre.tm_mday, sizeof(pre.tm_year)*(&pre.tm_year-&pre.tm_hour)));
        }

        std::string formatFileName( const std::string& path, 
                                    const std::string& module, 
                                    const tmExtend& time )
        {
            std::string datesPath = createLogDirectory(path, time);
            std::unique_ptr<char[]> buffer(new char[MAX_LOG_PATH]());
            sprintf(&buffer[0], "%s%s%s_%04d-%02d-%02d.%s", 
                                    datesPath.c_str(), \
                                    SEPRATOR, \
                                    module.c_str(), \
                                    time.tm_year, time.tm_mon, time.tm_mday, \
                                    LOG_APPEND_FIX);
            return (std::string)&buffer[0];
        }
    private:
        int file_size_; // Mbi
        int expiry_month_; // month
    };
}

#endif//__LOG4C_MANAGER_HPP__
