/// ============================================================================
/// sysv_mq.h
/// ----------------------------------------------------------------------------
/// Message Queue (System V IPC)
///
/// ----------------------------------------------------------------------------
/// Code Structure
/// ----------------------------------------------------------------------------
///
/// ----------------------------------------------------------------------------
/// License: The Unlicense <http://unlicense.org/>
/// ============================================================================

#ifndef MQUEUE_H
#define MQUEUE_H

/*
extern int msgctl(int __msqid, int __cmd, struct msqid_ds *__buf) __THROW;
extern int msgget(key_t __key, int __msgflg) __THROW;
extern ssize_t msgrcv(int __msqid, void *__msgp, size_t __msgsz, long int __msgtyp, int __msgflg);
extern int msgsnd(int __msqid, const void *__msgp, size_t __msgsz, int __msgflg);
*/

#include <sys/ipc.h>  // ftok()
#include <sys/msg.h>  // msgctl(), msgget(), ...

#include <iostream>

#define __IPCPLUSPLUS_BEGIN    namespace IPCplusplus {
#define __IPCPLUSPLUS_END      }

#define __SYSV_BEGIN    namespace sysv {
#define __SYSV_END      }

__IPCPLUSPLUS_BEGIN
__SYSV_BEGIN
namespace mq
{
    class MQueue
    {
    public:  // rule of five
        ~MQueue() = default;

        MQueue(const MQueue&) = default;
        MQueue& operator=(const MQueue&) = default;

        MQueue(MQueue&&) = default;
        MQueue& operator=(MQueue&&) = default;
    public:
        MQueue() { std::cout << "Hello, world!" << std::endl; }
    private:
        /// To do
    };
}  // IPCplusplus::sysv::mq
__SYSV_END
__IPCPLUSPLUS_END

using namespace ::IPCplusplus;


#endif  // MQUEUE_H
