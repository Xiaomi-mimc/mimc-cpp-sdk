#ifndef XMD_THREAD_H
#define XMD_THREAD_H

#include <thread>
#include <string>

class XMDThread {
public:
    XMDThread() {

    }
    
    static void* Func(void* param);
    virtual void* process() = 0;
    virtual ~XMDThread() { }

    int run();
    int join();

	std::thread::id threadId() { return t->get_id(); }


private:

	std::thread* t;

};

#endif //THREAD_H
