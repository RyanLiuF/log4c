# log4c 基于C++11的轻量级日志库
 ## 支持多线程、多进程并发调用;
 ## 支持跨平台(Android/Linux/Windows);
 ## 支持日志加密;
 ## 严格按日志等级来区分;
 ## 提供配套日志查看工具，git路径 [https://github.com/RyanLiuF/log4cView]
 ## 使用示例
    #include <ILog4C.h>
    #include <stl/os/path.hpp>
    #include <stl/utility/module.hpp>
    #include <iostream>
    int main()
    {
        std::string path = stl::os::path::to_absolute("tmpLog");

        stl::CModule<ILog4C> ptr = stl::make_module<ILog4C>("log4c", 
                                            stl::CModuleLifeCycle("Attach", "Dettach"), 
                                            "CMODULEXXX", path.c_str());

        if(!ptr)
        {
            printf("load dll failed\n");
            return 1;
        }
        ptr->setFileSize(1);

        auto start = stl::time::tick();
        auto f = [&](int i, int n = 0)->void {
            std::cout << this_thread::get_id() << std::endl;
            std::string module;
            switch (n)
            {
            case 0:
                module = "Moudle00000"; break;
            case 1:
                module = "Moudle11111"; break;
            case 2:
                module = "Moudle22222"; break;
            case 3:
                module = "Moudle33333"; break;
            default:
                module = "MoudleDefault"; break;
            }

            std::unique_ptr<char[]> buffer(new char[1024]());
            for (int k = 0; k < i; k++)
            {
                memset(&buffer[0], 0x00, 1024);
                sprintf(&buffer[0], "%04d", k);
                ptr->log(ILog4C::Debug, module.c_str(), __FILE__, __LINE__, __FUNCTION__, &buffer[0]);
                ptr->log(ILog4C::Warning, module.c_str(), __FILE__, __LINE__, __FUNCTION__, &buffer[0]);
                ptr->log(ILog4C::Infor, module.c_str(), __FILE__, __LINE__, __FUNCTION__, &buffer[0]);
                ptr->log(ILog4C::Error, module.c_str(), __FILE__, __LINE__, __FUNCTION__, &buffer[0]);
            }
        };
        auto thread1 = async(launch::async, f, 2000);
        auto thread2 = async(launch::async, f, 2000, 1);
        thread1.get();
 	    thread2.get();
        std::cout << "using:" <<stl::time::tick() - start << "ms" << std::endl;
        return 0;
    }