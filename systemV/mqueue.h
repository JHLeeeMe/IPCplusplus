/// ============================================================================
/// mqueue.h
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


#include <sys/ipc.h>
#include <sys/msg.h>

#define __IPCPLUSPLUS_SYSV_BEGIN    namespace IPCplusplus { \
                                        namespace sysv {
#define __IPCPLUSPLUS_SYSV_END      }}

__IPCPLUSPLUS_SYSV_BEGIN
namespace mq
{
    class MQueue
    {
    public:
        /// To do
        /// 1. rule of zero || rule of five
    private:
        /// To do
    };
}  // IPCplusplus::sysv::mq
__IPCPLUSPLUS_SYSV_END

using namespace ::IPCplusplus;


#endif  // MQUEUE_H
