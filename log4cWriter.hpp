#ifndef __LOG4C_WRITER_HPP__
#define __LOG4C_WRITER_HPP__

#include <thread>
#include <future>
#include <list>

#include "log4cManager.hpp"

#include <stl/concurrence/semaphore.hpp>

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
        , pre_detail_no_(1)
		, is_fd_open_(false)
		, fd_(-1)
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
			if (is_fd_open_)
			{
				io::flush(fd_);
				io::close(fd_);
				is_fd_open_ = false;
			}
        }

        int task()
        {
            Details detail;
            while(!exit_task_flag_.load())
            {
				if (deque_event_.wait_for(std::chrono::milliseconds(1)))
				{
					while (pop(detail))
					{
						write(detail);
					}
					if (is_fd_open_)
					{
						io::flush(fd_);
						io::close(fd_);
						is_fd_open_ = false;
					}
				}				
            }
			return 0;
        }

        bool push(const Details& detail)
        {
            if(!thread_task_.valid()){
                thread_task_ = std::async(std::launch::async, std::bind(&CWriter::task, this));
            }
			std::lock_guard<std::mutex> locker(deque_lock_);
            details_.push_back(detail);
			deque_event_.post();
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
            if(file_exists && !is_fd_open_)
            {
                manager_->isRename(file);
            }

            if( is_fd_open_ && \
                manager_->detectDateChanged(pre_time_, detail.time) )
            {
				io::flush(fd_);
				io::close(fd_);
				is_fd_open_ = false;
            }

			if (!is_fd_open_)
			{
#ifdef __os_windows__
				fd_ = io::open(file.c_str(), _O_APPEND | _O_CREAT | _O_BINARY | _O_RDWR, _S_IREAD | _S_IWRITE);
#else
				fd_ = io::open(file.c_str(), O_APPEND | O_CREAT | O_RDWR/*|O_SYNC*/, S_IREAD | S_IWRITE);
#endif
				if (fd_ == -1)
				{
					return;
				}
			}
            std::unique_ptr<char[]> buffer(new char[MAX_BUFFER_SIZE]());
            int bufferLen = 0;
			pre_detail_no_ = 1;
            if(!file_exists)
            {
                memcpy(&buffer[0], &pre_detail_no_, MAX_RECORD_NO_LEN);
                bufferLen += MAX_RECORD_NO_LEN;
            }
            else
            {
				io::lseek(fd_, -MAX_RECORD_NO_LEN, SEEK_END);
				io::read(fd_, &pre_detail_no_, MAX_RECORD_NO_LEN);
				io::lseek(fd_, 0, SEEK_END);
            }

			auto _append_buffer_func = [&](void* param, int len)->void{
				memcpy(&buffer[bufferLen], param, len);
				bufferLen += len;
			};

            pre_time_ = detail.time;

			_append_buffer_func((void*)detail.layer.c_str(), MAX_LAYER_NAME_LEN);

			_append_buffer_func((void*)&detail.time, sizeof(tmExtend));

			_append_buffer_func((void*)detail.type.c_str(), MAX_TYPE_LEN);

            std::string name = file::name(detail.file);

			_append_buffer_func((void*)name.c_str(), MAX_FILE_NAME_LEN);

			_append_buffer_func((void*)&detail.lineNo, MAX_RECORD_NO_LEN);

			_append_buffer_func((void*)detail.func.c_str(), MAX_FUNC_NAME_LEN);

			_append_buffer_func((void*)detail.isCrypt ? "E" : "D", 1);

			_append_buffer_func((void*)&detail.threadId, MAX_THREAD_ID_LEN);

            int leftLen = MAX_BUFFER_SIZE - bufferLen - MAX_RECORD_NO_LEN;
            int contentLen = detail.content.size()<leftLen?detail.content.size():leftLen;
			_append_buffer_func((void*)&contentLen, MAX_RECORD_NO_LEN);

			_append_buffer_func((void*)detail.content.c_str(), contentLen);

            pre_detail_no_++;
			_append_buffer_func((void*)&pre_detail_no_, MAX_RECORD_NO_LEN);

			io::write(fd_, &buffer[0], bufferLen);
            return;
        }
    private:
        std::shared_ptr<logger::CManager> manager_;
        std::atomic<bool> exit_task_flag_;
        std::future<int> thread_task_;

		stl::concurrence::CSemaphore deque_event_;
		std::mutex deque_lock_;

        tmExtend pre_time_;
        int pre_detail_no_;

		std::atomic<bool> is_fd_open_;
		std::atomic<int> fd_;

        std::list<Details> details_;
        std::string name_of_moudle_;
        std::string path_to_save_;
    };
}

#endif//__LOG4C_WRITER_HPP__