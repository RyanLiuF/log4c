#ifndef __LOG4C_WRITER_HPP__
#define __LOG4C_WRITER_HPP__

#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include <list>
#include <condition_variable>

#include "log4cManager.hpp"

#define MAX_BUFFER_SIZE		1024*100
#define MAX_FUNC_NAME_LEN	64
#define MAX_LAYER_NAME_LEN	32
#define MAX_FILE_NAME_LEN	32
#define MAX_RECORD_NO_LEN	4
#define MAX_THREAD_ID_LEN	4
#define MAX_TYPE_LEN		8

namespace logger
{
    struct Details
    {
        std::string layer;
        std::string type;
        std::string file;
        std::string func;
        std::string content;
        tmExtend    time;
        bool        isCrypt;
        unsigned long threadId;
        unsigned long lineNo;
    };
    
    class CWriter
    {
    public:
        explicit CWriter(const std::string& name_of_module, const std::string& path_to_save)
        : name_of_moudle_(name_of_module)
        , path_to_save_(path_to_save)
        , manager_(new logger::CManager())
        , exit_task_flag_(false)
        , last_detail_no_(1)
        {

        }
        virtual ~CWriter()
        {
            exit_task_flag_.store(true);
            if(thread_task_.valid())
            {
                thread_task_.get();
            }
			Details detail;
            while(pop(detail))
            {
                write(detail);
            }
        }

        int run()
        {
            Details detail;
            while(!exit_task_flag_.load())
            {
                while(pop(detail))
                {
                    write(detail);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
			return 0;
        }

        bool push(const Details& detail)
        {
            if(!thread_task_.valid())
            {
                thread_task_ = std::async(std::launch::async, std::bind(&CWriter::run, this));
            }
			std::lock_guard<std::mutex> locker(deque_lock_);
            details_.push_back(detail);
            return true;
        }
        bool pop(Details& detail)
        {
			std::lock_guard<std::mutex> locker(deque_lock_);
            if(details_.empty()){
                return false;
            }
            detail = details_.front();
            details_.pop_front();
            return true;
        }
        void setFileSize(int size)
        {
            manager_->setFileSize(size);
        }
        void setExpiryDate(int month)
        {
            manager_->setExpiryDate(month);
        }
        void setRoot(const std::string& directory)
        {
            path_to_save_ += directory;
        }
        void write(const Details& detail)
        {
            std::string file = manager_->formatFileName(path_to_save_, name_of_moudle_, detail.time);

            bool file_exists = (access(file.c_str(), F_OK) == 0);
            if(file_exists && !writer_stream_.is_open())
            {
                manager_->isRename(file);
            }

            if( writer_stream_.is_open() && \
                manager_->detectDateChanged(last_write_detail_time_, detail.time) )
            {
                writer_stream_.close();
            }

            if(!writer_stream_.is_open())
            {
                writer_stream_.open(file.c_str(), std::ios_base::in|std::ios_base::out|\
                                                  std::ios_base::app|std::ios_base::binary);
                if(!writer_stream_.is_open())
                {
                    return;
                }
            }
            std::unique_ptr<char[]> buffer(new char[MAX_BUFFER_SIZE]());
            int bufferLen = 0;
            last_detail_no_ = 1;
            if(!file_exists)
            {
                memcpy(&buffer[0], &last_detail_no_, MAX_RECORD_NO_LEN);
                bufferLen += MAX_RECORD_NO_LEN;
            }
            else
            {
                writer_stream_.seekg(-MAX_RECORD_NO_LEN, std::ios_base::end);
                writer_stream_.read((char*)&last_detail_no_, MAX_RECORD_NO_LEN);
                writer_stream_.seekg(0, std::ios_base::end);
            }

			auto _append_buffer_func = [&](void* param, int len)->void{
				memcpy(&buffer[bufferLen], param, len);
				bufferLen += len;
			};

            last_write_detail_time_ = detail.time;

			_append_buffer_func((void*)detail.layer.c_str(), MAX_LAYER_NAME_LEN);

			_append_buffer_func((void*)&detail.time, sizeof(tmExtend));

			_append_buffer_func((void*)detail.type.c_str(), MAX_TYPE_LEN);

            std::string name = stl::os::file::name(detail.file);

			_append_buffer_func((void*)name.c_str(), MAX_FILE_NAME_LEN);

			_append_buffer_func((void*)&detail.lineNo, MAX_RECORD_NO_LEN);

			_append_buffer_func((void*)detail.func.c_str(), MAX_FUNC_NAME_LEN);

			_append_buffer_func((void*)detail.isCrypt ? "E" : "D", 1);

			_append_buffer_func((void*)&detail.threadId, MAX_THREAD_ID_LEN);

            int leftLen = MAX_BUFFER_SIZE - bufferLen - MAX_RECORD_NO_LEN;
            int contentLen = detail.content.size()<leftLen?detail.content.size():leftLen;
			_append_buffer_func((void*)&contentLen, MAX_RECORD_NO_LEN);

			_append_buffer_func((void*)detail.content.c_str(), contentLen);

            last_detail_no_++;
			_append_buffer_func((void*)&last_detail_no_, MAX_RECORD_NO_LEN);

            writer_stream_.write(&buffer[0], bufferLen);
			writer_stream_.flush();
			writer_stream_.close();
            return;
        }
    private:
        std::shared_ptr<logger::CManager> manager_;
        std::atomic<bool> exit_task_flag_;
        std::future<int> thread_task_;

		std::mutex deque_lock_;

        tmExtend last_write_detail_time_;
        int last_detail_no_;

        std::fstream writer_stream_;
        std::list<Details> details_;
        std::string name_of_moudle_;
        std::string path_to_save_;
    };
}

#endif//__LOG4C_WRITER_HPP__