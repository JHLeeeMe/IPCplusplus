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


#include <sys/ipc.h>  // ftok()
#include <sys/msg.h>  // msgctl(), msgget(), ...
#include <string.h>   // memcpy()

#include <iostream>
#include <stdexcept>  // runtime_error()

#define __IPCPLUSPLUS_BEGIN    namespace ipcplusplus {
#define __IPCPLUSPLUS_END      }

__IPCPLUSPLUS_BEGIN
namespace sysv
{

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
}  // ::ipcplusplus::sysv::utils

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
            , queue_id_{}
            , queue_info_{}
            , permission_{}
            , msg_buf_{}
            , payload_max_size_{ 128 }
            , err_{}
        {
            if (create_queue(perm) < 0)
            {
                if (err_ == EEXIST)
                {
                    throw std::runtime_error("Queue already exists");
                }

                throw std::runtime_error("Error: " + std::to_string(err_));
            }

            if (::msgctl(queue_id_, IPC_STAT, &queue_info_) < 0)
            {
                throw std::runtime_error("Error: " + std::to_string(err_));
            }

            permission_ = static_cast<ePermission>(queue_info_.msg_perm.mode);
            set_msg_buf_size(sizeof(long) + sizeof(size_t) + payload_max_size_);
        }

        ~MQueue()
        {
            if (remove_queue() < 0)
            {
                throw std::runtime_error("Error: " + std::to_string(err_));
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

        inline ssize_t send(const std::string& msg, const long msg_type = 0)
        {
            set_msg_buf(msg, msg_type);

            if (::msgsnd(queue_id_,
                         msg_buf_.data(),
                         sizeof(size_t) + msg.length(), 0) < 0)
            {
                err_ = errno;
                return -1;
            }

            return 0;
        }

        inline ssize_t send_nowait(const std::string& msg, const long msg_type = 0)
        {
            set_msg_buf(msg, msg_type);

            if (::msgsnd(queue_id_,
                         msg_buf_.data(),
                         sizeof(size_t) + msg.length(), IPC_NOWAIT) < 0)
            {
                err_ = errno;
                return -1;
            }

            return 0;
        }

        inline ssize_t receive(const long msg_type)
        {
            msg_buf_.clear();
            msg_buf_.resize(sizeof(msg_type) + sizeof(size_t) + payload_max_size_);

            if (::msgrcv(queue_id_,
                         msg_buf_.data(),
                         sizeof(size_t) + payload_max_size_,
                         msg_type, 0) < 0)
            {
                err_ = errno;
                return -1;
            }

            return 0;
        }

        inline ssize_t receive_nowait(const long msg_type)
        {
            msg_buf_.clear();
            msg_buf_.resize(sizeof(msg_type) + sizeof(size_t) + payload_max_size_);

            if (::msgrcv(queue_id_,
                         msg_buf_.data(),
                         sizeof(size_t) + payload_max_size_,
                         msg_type, IPC_NOWAIT) < 0)
            {
                err_ = errno;
                return -1;
            }

            return 0;
        }

        inline void set_msg_buf_size(const size_t size)
        {
            msg_buf_.reserve(size);
            payload_max_size_ = size - sizeof(long);
        }

        inline ePermission get_permission() const
        {
            return permission_;
        }

        inline struct msqid_ds get_queue_info() const
        {
            return queue_info_;
        }

        inline std::string msg() const
        {
            const size_t* msg_len = reinterpret_cast<const size_t*>(msg_buf_.data() + sizeof(long));
            return std::string(msg_buf_.begin() + sizeof(long) + sizeof(size_t),
                               msg_buf_.begin() + sizeof(long) + sizeof(size_t) + *msg_len);
        }

        inline ssize_t err() const
        {
            return err_;
        }

    private:
        inline ssize_t create_queue(ePermission perm)
        {
            queue_id_ = ::msgget(key_, IPC_CREAT | IPC_EXCL |
                                       static_cast<uint32_t>(perm));
            if (queue_id_ < 0)
            {
                err_ = errno;
                return -1;
            }

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

        inline void set_msg_buf(const std::string& msg, const long msg_type)
        {
            const size_t msg_len = msg.length();

            msg_buf_.clear();
            msg_buf_.resize(sizeof(msg_type) + sizeof(msg_len) + msg_len);

            memcpy(msg_buf_.data(),
                   &msg_type, sizeof(long));
            memcpy(msg_buf_.data() + sizeof(msg_type),
                   &msg_len, sizeof(msg_len));
            memcpy(msg_buf_.data() + sizeof(msg_type) + sizeof(msg_len),
                   msg.c_str(), msg.length());
        }

    private:
        key_t key_;

        int32_t         queue_id_;
        struct msqid_ds queue_info_;
        ePermission     permission_;

        std::vector<uint8_t> msg_buf_;
        size_t               payload_max_size_;

        ssize_t err_;
    };
}  // ::ipcplusplus::sysv::mq

}
__IPCPLUSPLUS_END

namespace sysv = ::ipcplusplus::sysv;


#endif  // MQUEUE_H
