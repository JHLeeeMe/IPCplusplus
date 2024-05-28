/// ============================================================================
/// sysv_mq.h
/// ----------------------------------------------------------------------------
/// Message Queue (System V IPC) Header-Only library
///
/// ----------------------------------------------------------------------------
/// Code Structure
/// ----------------------------------------------------------------------------
/// namespace ipcplusplus
/// {
/// namespace sysv
/// {
///     // Declarations
///     namespace utils { ... }
///     namespace mq { ... }
///
///     // Implementations
///     namespace utils { ... }
///     namespace mq
///     {
///         inline MQueue::xxx { ... };
///         ...
///     }
/// }
/// }
/// using sysv = ::ipcplusplus::sysv;
///
/// ----------------------------------------------------------------------------
/// License: The Unlicense <https://unlicense.org/>
/// ============================================================================

#ifndef SYSV_MQ_H
#define SYSV_MQ_H


#include <sys/ipc.h>  // ftok()
#include <sys/msg.h>  // msgctl(), msgget(), ...

#include <cstring>    // memcpy()
#include <stdexcept>  // runtime_error()
#include <vector>     // vector

#define __IPCPLUSPLUS_BEGIN    namespace ipcplusplus {
#define __IPCPLUSPLUS_END      }

__IPCPLUSPLUS_BEGIN
namespace sysv
{

/// ============================================================================
/// Decalarations
/// ============================================================================

namespace utils
{
    key_t create_key(const std::string& path, const uint8_t proj_id);
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

    ePermission operator|(ePermission lhs, ePermission rhs);

    class MQueue
    {
    public:  // rule of 5
        explicit MQueue(const key_t key);
        explicit MQueue(const key_t key, ePermission perm);
        explicit MQueue(const key_t key, const size_t payload_max_size);
        explicit MQueue(const key_t key,
                        ePermission perm,
                        const size_t payload_max_size);
        ~MQueue();

        MQueue(const MQueue&) = delete;
        MQueue& operator=(const MQueue&) = delete;

        MQueue(MQueue&&) = delete;
        MQueue& operator=(MQueue&&) = delete;
    public:
        auto change_permission(ePermission perm) -> ssize_t;

        auto send(const std::string& msg, const long msg_type = 0) -> ssize_t;
        auto send_nowait(const std::string& msg,
                         const long msg_type = 0) -> ssize_t;
        auto receive(const long msg_type) -> ssize_t;
        auto receive_nowait(const long msg_type) -> ssize_t;

        auto permission() const -> ePermission;
        auto queue_info() const -> struct msqid_ds;
        auto msg()        const -> std::string;
        auto err()        const -> ssize_t;

    private:
        auto create_queue(ePermission perm) -> ssize_t;
        auto remove_queue() -> ssize_t;
        auto set_msg_buf(const std::string& msg,
                         const long msg_type) -> void;
    private:
        key_t key_;

        bool            queue_owner_;
        int32_t         queue_id_;
        struct msqid_ds queue_info_;
        ePermission     permission_;

        std::vector<uint8_t> msg_buf_;
        size_t               payload_max_size_;

        ssize_t err_;
    };
}  // ::ipcplusplus::sysv::mq


/// ============================================================================
/// Implementations
/// ============================================================================

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
    inline ePermission operator|(ePermission lhs, ePermission rhs)
    {
        return static_cast<ePermission>(
            static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
    }


    /// ========================================================================
    /// MQueue rule of 5
    /// ========================================================================

    inline MQueue::MQueue(const key_t key)
        : MQueue(key, ePermission::RWR_R_, 128)
    {
    }

    inline MQueue::MQueue(const key_t key, ePermission perm)
        : MQueue(key, perm, 128)
    {
    }

    inline MQueue::MQueue(const key_t key, const size_t payload_max_size)
        : MQueue(key, ePermission::RWR_R_, payload_max_size)
    {
    }

    inline MQueue::MQueue(const key_t key,
                   ePermission perm,
                   const size_t payload_max_size)
        : key_{ key }
        , queue_owner_{ true }
        , queue_id_{}
        , queue_info_{}
        , permission_{}
        , msg_buf_{}
        , payload_max_size_{ payload_max_size }
        , err_{}
    {
        if (create_queue(perm) < 0)
        {
            if (err_ != EEXIST)
            {
                throw std::runtime_error("Error: " + std::to_string(err_));
            }

            err_ = 0;
            if ((queue_id_ = ::msgget(key_, 0)) < 0)
            {
                err_ = errno;
                throw std::runtime_error(
                    "Error accessing existing queue: " + std::to_string(err_));
            }
            queue_owner_ = false;
        }

        try
        {
            if (::msgctl(queue_id_, IPC_STAT, &queue_info_) < 0)
            {
                throw std::runtime_error("Error: " + std::to_string(err_));
            }

            permission_ = static_cast<ePermission>(queue_info_.msg_perm.mode);
            msg_buf_.reserve(sizeof(long) + sizeof(size_t) + payload_max_size_);
        }
        catch (...)
        {
            if (queue_owner_)
            {
                remove_queue();
            }
            throw;
        }
    }

    inline MQueue::~MQueue()
    {
        if (queue_owner_)
        {
            remove_queue();
        }
    }


    /// ========================================================================
    /// MQueue public methods
    /// ========================================================================

    inline auto MQueue::change_permission(ePermission perm) -> ssize_t
    {
        queue_info_.msg_perm.mode = static_cast<uint32_t>(perm);
        if (::msgctl(queue_id_, IPC_SET, &queue_info_) < 0)
        {
            queue_info_.msg_perm.mode = static_cast<uint32_t>(permission_);
            err_ = errno;
            return -1;
        }

        permission_ = perm;

        return 0;
    }

    inline auto MQueue::send(const std::string& msg,
                             const long msg_type) -> ssize_t
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

    inline auto MQueue::send_nowait(const std::string& msg,
                                    const long msg_type) -> ssize_t
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

    inline auto MQueue::receive(const long msg_type) -> ssize_t
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

    inline auto MQueue::receive_nowait(const long msg_type) -> ssize_t
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

    inline auto MQueue::permission() const -> ePermission
    {
        return permission_;
    }

    inline auto MQueue::queue_info() const -> struct msqid_ds
    {
        return queue_info_;
    }

    inline auto MQueue::msg() const -> std::string
    {
        const size_t* msg_len =
            reinterpret_cast<const size_t*>(msg_buf_.data() + sizeof(long));
        return std::string(
            msg_buf_.begin() + sizeof(long) + sizeof(size_t),
            msg_buf_.begin() + sizeof(long) + sizeof(size_t) + *msg_len);
    }

    inline auto MQueue::err() const -> ssize_t
    {
        return err_;
    }


    /// ========================================================================
    /// MQueue private methods
    /// ========================================================================

    inline auto MQueue::create_queue(ePermission perm) -> ssize_t
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

    inline auto MQueue::remove_queue() -> ssize_t
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

    inline auto MQueue::set_msg_buf(const std::string& msg,
                                    const long msg_type) -> void
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
}  // ::ipcplusplus::sysv::mq

}  // ::ipcplusplus::sysv
__IPCPLUSPLUS_END


/// ============================================================================
/// namespace alias
/// ============================================================================

namespace sysv = ::ipcplusplus::sysv;


#endif  // SYSV_MQ_H
