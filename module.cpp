#include "log4cImp.hpp"

extern "C"
{

    EXPORT_API_ATTR_DEFAULT void* WINAPI Attach(void* attach, void* reserved)
    {
		logger::CImplement* obj = new logger::CImplement((const char*)attach, (const char*)reserved);
        return obj;
    }

    EXPORT_API_ATTR_DEFAULT void WINAPI Dettach(void* p)
    {
        if (p)
        {
			logger::CImplement* obj = (logger::CImplement*)p;
            delete obj;
            obj = NULL;
        }
    }

}