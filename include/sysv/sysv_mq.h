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

/*
// errno
EACCES : path를 구성하는 directory중에서 search 권한(x)이 없어서 접근할 수 없습니다.
EFAULT : path 변수 자체가 잘못된 주소입니다. 
ELOOP : 너무 많은 symbolic link로 directory가 loop에 빠졌습니다.
ENAMETOOLONG : path가 너무 길거나 이름이 너무 깁니다.
ENOENT : path가 빈 문자열이거나 path를 구성하는 directory중에서 없는 directory가 있습니다.
ENOMEM : 메모리가 부족합니다.
ENOTDIR : path를 구성하는 directory중에서 directory가 아닌 것이 있습니다.
EOVERFLOW : 32bit OS에서 컴파일시에 -D_FILE_OFFSET_BITS=64 옵션없이 컴파일하여 파일크기나 
    inode번호가 64bit에 맞지 않은 경우
*/

#ifndef MQUEUE_H
#define MQUEUE_H


#include <sys/ipc.h>  // ftok()
#include <sys/msg.h>  // msgctl(), msgget(), ...

#include <iostream>
#include <stdexcept>  // runtime_error()

#define __IPCPLUSPLUS_BEGIN    namespace IPCplusplus {
#define __IPCPLUSPLUS_END      }

#define __SYSV_BEGIN    namespace sysv {
#define __SYSV_END      }

__IPCPLUSPLUS_BEGIN
__SYSV_BEGIN
namespace utils
{
    inline key_t create_key(const std::string& path, const uint8_t proj_id)
    {
        if (proj_id == 0)
        {
            return -1;
        }

        return ::ftok(path.c_str(), proj_id);
    }
}  // ::IPCplusplus::sysv::utils

namespace mq
{
    enum class ePermission
    {
        R_____ = 0400, RW____ = 0600,
        _W____ = 0200, __RW__ = 0060,
        __R___ = 0040, ____RW = 0006,
        ___W__ = 0020, RWRWRW = 0666,
        ____R_ = 0004, RWR_R_ = 0644,
        _____W = 0002, RW_W_W = 0622,
    };

    inline ePermission operator|(ePermission lhs, ePermission rhs)
    {
        return static_cast<ePermission>(
            static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs)
        );
    }

    class MQueue
    {
    public:  // rule of five
        MQueue(const key_t key, ePermission perm = ePermission::RWR_R_)
            : key_{ key }
        {
            if (create_queue(perm) < 0)
            {
                if (err_ == EEXIST)
                {
                    queue_id_ = ::msgget(key_, 0);
                }

                // throw or ...
            }

            if (::msgctl(queue_id_, IPC_STAT, &queue_info_) < 0)
            {
                // throw or ...
            }

            permission_ = static_cast<ePermission>(queue_info_.msg_perm.mode);
        }

        ~MQueue()
        {
            if (remove_queue() < 0)
            {
                // throw or ...
            }
        }

        MQueue(const MQueue&) = delete;
        MQueue& operator=(const MQueue&) = delete;

        MQueue(MQueue&&) = delete;
        MQueue& operator=(MQueue&&) = delete;

    public:
        inline ssize_t change_permission(ePermission perm)
        {
            queue_info_.msg_perm.mode = static_cast<uint32_t>(perm);
            if (::msgctl(queue_id_, IPC_SET, &queue_info_) < 0)
            {
                err_ = errno;
                return -1;
            }

            permission_ = perm;

            return 0;
        }

        inline ssize_t send(const std::string& msg, int msg_type = 0)
        {
            // To do
        }

        inline ssize_t receive(int msg_type)
        {
            // To do
        }

        inline ePermission get_permission() const
        {
            return permission_;
        }

        inline struct msqid_ds get_queue_info() const
        {
            return queue_info_;
        }

        inline ssize_t get_err() const
        {
            return err_;
        }

    private:
        inline ssize_t create_queue(ePermission perm)
        {
            queue_id_ = ::msgget(key_, IPC_CREAT |
                                       IPC_EXCL |
                                       static_cast<uint32_t>(perm));
            if (queue_id_ < 0)
            {
                err_ = errno;
                return -1;
            }

            queue_owner_ = true;
            return 0;
        }

        inline ssize_t remove_queue()
        {
            if (::msgget(key_, 0) < 0)
            {
                err_ = errno;
                return -1;
            }

            if (::msgctl(queue_id_, IPC_RMID, nullptr) < 0)
            {
                err_ = errno;
                return -1;
            }

            return 0;
        }

    private:
        bool            queue_owner_{};

        key_t           key_{};
        ePermission     permission_{};

        uint32_t        queue_id_{};
        struct msqid_ds queue_info_{};

        ssize_t         err_{};
    };
}  // ::IPCplusplus::sysv::mq
__SYSV_END
__IPCPLUSPLUS_END

namespace sysv = ::IPCplusplus::sysv;


#endif  // MQUEUE_H
