#include "xmd_thread.h"

void* XMDThread::Func(void* obj)
{
    XMDThread* th = (XMDThread*)obj;
    return th->process();
}

int XMDThread::run()
{
	t = new std::thread(XMDThread::Func, this);
	return 0;
}

int XMDThread::join()
{
	t->join();
	return 0;
}


