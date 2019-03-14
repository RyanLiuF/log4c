#ifndef __LOG4C_IMP_HPP__
#define __LOG4C_IMP_HPP__

#include "ILog4C.h"
#include <stl/algorithm/base.hpp>
#include <stl/stringhelper.hpp>
#include "log4cWriter.hpp"


namespace logger
{
    class CImplement: public ILog4C
    {
    public:
        explicit CImplement(const char* module, const char* path)
        : log_of_module_(module)
        , log_of_path_(path)
        , log_trace_level_(0x1111)
        {
            if(log_writer_map_.find(log_of_module_) == log_writer_map_.end())
            {
                log_writer_map_[log_of_module_] = std::make_shared<CWriter>(log_of_module_, log_of_path_);
            }
        }
        virtual ~CImplement()
        {
        }
        virtual void setRoot(const char* directory) override
        {
            std::lock_guard<std::mutex> lock(writer_locker_);
            if (log_writer_map_.find(log_of_module_) != log_writer_map_.end())
            {
                log_writer_map_[log_of_module_]->setRoot(directory);
            }
            return ;
        }
        virtual void setFileSize(int size) override
        {
            std::lock_guard<std::mutex> lock(writer_locker_);
            if (log_writer_map_.find(log_of_module_) != log_writer_map_.end())
            {
                log_writer_map_[log_of_module_]->setFileSize(size);
            }
            return ;
        }
        virtual void setTraceLevel(int level) override
        {
            std::lock_guard<std::mutex> lock(writer_locker_);
            log_trace_level_ = level;
            return ;
        }
        virtual void setExpiryDate(int month) override
        {
            std::lock_guard<std::mutex> lock(writer_locker_);
            if (log_writer_map_.find(log_of_module_) != log_writer_map_.end())
            {
                log_writer_map_[log_of_module_]->setExpiryDate(month);
            }
            return ;
        }
        virtual void log( elog_level level, 
                          const char* layer, const char* file, 
                          int lineno, const char* func, 
                          int len, const char* varName, 
                          const unsigned char* hexStream ) override
        {
            Details detail;
            detail.layer = layer;
            detail.file = file;
            detail.lineNo = lineno;
            detail.func = func;
            detail.time = GetLocalTime();
            switch(level&log_trace_level_)
            {
            case Debug:
                detail.type = "DEBUG";
                break;
            case Warning:
                detail.type = "WARNING";
                break;
            case Infor:
                detail.type = "INFOR";
                break;
            case Error:
                detail.type = "ERROR";
                break;
            default:
                return;
            }
            detail.isCrypt = true;
            detail.threadId = stl::lexical::as<unsigned long, std::thread::id>(std::this_thread::get_id());
            std::vector<char> vContent(len, 0);
		    memcpy(&vContent[0], hexStream, len);
            std::string tmp(varName);
            tmp += "=";
            tmp += stl::stringhelper::stringify(vContent.begin(), vContent.end(), " ");
            detail.content = stl::algorithm::base64::encode(tmp);

            std::lock_guard<std::mutex> lock(writer_locker_);
            if (log_writer_map_.find(log_of_module_) != log_writer_map_.end())
            {
                log_writer_map_[log_of_module_]->push(detail);
            }
            return ;
        }
        virtual void log( elog_level level, 
                          const char* layer, const char* file, 
                          int lineno, const char* func, 
                          const char* format, ... ) override
        {
            Details detail;
            detail.layer = layer;
            detail.file = file;
            detail.lineNo = lineno;
            detail.func = func;
            detail.time = GetLocalTime();
            std::unique_ptr<char[]> buffer(new char[1024*10+1]());
            va_list ap;
            va_start(ap, format);
            vsnprintf(&buffer[0], 1024*10, format, ap);
            va_end(ap);

            detail.content = stl::algorithm::base64::encode(&buffer[0]);

            switch(level&log_trace_level_)
            {
            case Debug:
				detail.type = "DEBUG";
                break;
            case Warning:
				detail.type = "WARNING";
                break;
            case Infor:
				detail.type = "INFOR";
                break;
            case Error:
				detail.type = "ERROR";
                break;
            default:
                return;
            }
            detail.isCrypt = true;
            detail.threadId = stl::lexical::as<unsigned long, std::thread::id>(std::this_thread::get_id());

           std::lock_guard<std::mutex> lock(writer_locker_);
            if (log_writer_map_.find(log_of_module_) != log_writer_map_.end())
            {
                log_writer_map_[log_of_module_]->push(detail);
            }
            return ;
        }
    private:
        static std::map<std::string, std::shared_ptr<CWriter>> log_writer_map_;
        static std::mutex writer_locker_;
        std::string log_of_module_;
        std::string log_of_path_;
        int log_trace_level_;
    };
	std::map<std::string, std::shared_ptr<CWriter>> CImplement::log_writer_map_;
	std::mutex CImplement::writer_locker_;
}

#endif//__LOG4C_IMP_HPP__