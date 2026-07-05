#include "std17pch.h"

namespace HarisLinux {

enum class FileType { Generic, Pipe, NetworkSocket, UnixSocket, Epoll, EventFd };

inline void clean_up_fd_resource(int fd, FileType type, const std::string& file_path) noexcept {
    if (fd < 0) return;

    switch (type) {
        case FileType::NetworkSocket:
            ::shutdown(fd, SHUT_RDWR);  ///< Disconnect socket interface
            ::close(fd);
            break;

        case FileType::UnixSocket:
            ::shutdown(fd, SHUT_RDWR);
            [[fallthrough]];  ///< C++17 attribute: Safely bypass compiler implicit-fallthrough warnings

        case FileType::Pipe:
            ::close(fd);
            if (!file_path.empty()) {
                ::unlink(file_path.c_str());
            }
            break;

        case FileType::Generic:
        case FileType::Epoll:
        case FileType::EventFd:
        default:
            ::close(fd);
            break;
    }

    // std::cout << "-------------------------- UNLINK fd: " << fd << "--------------------------\n";
}

class UniqueFileDescriptor {
 private:
    int         _fd        = -1;  // Managed raw file descriptor
    FileType    _type      = FileType::Generic;
    std::string _file_path = "";

 public:
    /// @brief Default constructor creating an empty, invalid management envelope.
    UniqueFileDescriptor() noexcept : _fd(-1), _type(FileType::Generic), _file_path("") {}

    /// @brief Explicit value constructor binding a live descriptor and tracking properties.
    explicit UniqueFileDescriptor(int fd, FileType type = FileType::Generic, std::string path = "") noexcept
        : _fd(fd), _type(type), _file_path(std::move(path)) {}

    /// @brief Destructor triggering type-safe automated cleanup when exiting stack context.
    ~UniqueFileDescriptor() {
        if (_fd >= 0) {
            clean_up_fd_resource(_fd, _type, _file_path);
        }
    }

    /// @brief Delete copy semantics to eliminate process-level double-close security vulnerabilities.
    UniqueFileDescriptor(const UniqueFileDescriptor&) = delete;
    UniqueFileDescriptor& operator=(const UniqueFileDescriptor&) = delete;

    /// @brief Move constructor extracting assets from temporary source targets.
    UniqueFileDescriptor(UniqueFileDescriptor&& other) noexcept
        : _fd(other._fd), _type(other._type), _file_path(std::move(other._file_path)) {
        other._fd = -1;  // Reset source to an invalid state
    }

    /// @brief Move assignment operator recycling internal assets before acquiring new controls.
    UniqueFileDescriptor& operator=(UniqueFileDescriptor&& other) noexcept {
        if (this != &other) {
            if (_fd >= 0) ::close(_fd);  // Release existing resource
            _fd        = other._fd;      // Acquire new resource
            _type      = other._type;
            _file_path = std::move(other._file_path);
            other._fd  = -1;  // Reset source
        }
        return *this;
    }

    /// @brief Inline accessors optimized for compile-time execution.
    [[nodiscard]] constexpr int  get() const noexcept { return _fd; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return _fd >= 0; }
    [[nodiscard]] FileType       get_type() const noexcept { return _type; }
    const std::string&           get_path() const noexcept { return _file_path; }

    /// @brief Releases ownership without closing the underlying file descriptor.
    constexpr int release() noexcept {
        int tmp = _fd;
        _fd     = -1;
        return tmp;
    }

    static bool create_fifo_direction(const std::string& path) noexcept {
        ::unlink(path.c_str());  ///< Evict preexisting legacy node safely from path index

        if (::mkfifo(path.c_str(), 0666) < 0) {
            return false;  ///< Return invalid envelope on filesystem system initialization failures
        }

        ::access(path.c_str(), F_OK);

        return true;
    }

    /**
     * @brief Advanced RAII Resource Factory handling full initialization sequence for Linux FIFOs.
     * @return Fully configured UniqueFileDescriptor envelope controlling a live stream.
     */

    static UniqueFileDescriptor create_fifo_fd(const std::string& path, int open_flags) noexcept {
        int raw_fd = ::open(path.c_str(), open_flags);

        return UniqueFileDescriptor(raw_fd, FileType::Pipe, path);
    }
};

class ShareFileDescriptor {
 private:
    int                    _fd        = -1;
    std::atomic<uint32_t>* _ref_count = nullptr;
    FileType               _type      = FileType::Generic;
    std::string            _file_path = "";

    void release_ref() noexcept {
        if (_ref_count) {
            if (_ref_count->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                clean_up_fd_resource(_fd, _type, _file_path);
                delete _ref_count;
            }
        }
        _fd        = -1;
        _ref_count = nullptr;
    }

 public:
    constexpr ShareFileDescriptor() noexcept = default;

    explicit ShareFileDescriptor(int fd, FileType type = FileType::Generic, std::string path = "")
        : _type(type), _file_path(std::move(path)) {
        if (fd >= 0) {
            _fd        = fd;
            _ref_count = new std::atomic<uint32_t>(1);  // for counter
        }
    }

    explicit ShareFileDescriptor(UniqueFileDescriptor&& unique_fd, FileType type = FileType::Generic,
                                 std::string path = "") noexcept
        : _type(type), _file_path(std::move(path)) {
        if (unique_fd.is_valid()) {
            _fd        = unique_fd.release();
            _ref_count = new std::atomic<uint32_t>(1);
        }
    }

    // Destructor: ref_count
    ~ShareFileDescriptor() { release_ref(); }

    // Copy Constructor: Increase ref_count safety (Relaxed memory order)
    ShareFileDescriptor(const ShareFileDescriptor& other) noexcept
        : _fd(other._fd), _ref_count(other._ref_count), _type(other._type), _file_path(other._file_path) {
        if (_ref_count) {
            _ref_count->fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Copy Assignment Operator
    ShareFileDescriptor& operator=(const ShareFileDescriptor& other) noexcept {
        if (this != &other) {
            release_ref();
            _fd        = other._fd;
            _ref_count = other._ref_count;
            _type      = other._type;
            _file_path = other._file_path;
            if (_ref_count) {
                _ref_count->fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    // Move Semantics to container (not changed ref_count)
    ShareFileDescriptor(ShareFileDescriptor&& other) noexcept
        : _fd(other._fd),
          _ref_count(other._ref_count),
          _type(other._type),  //
          _file_path(std::move(other._file_path)) {
        other._fd        = -1;
        other._ref_count = nullptr;
    }

    ShareFileDescriptor& operator=(ShareFileDescriptor&& other) noexcept {
        if (this != &other) {
            release_ref();
            _fd              = other._fd;
            _ref_count       = other._ref_count;
            _type            = other._type;
            _file_path       = std::move(other._file_path);
            other._fd        = -1;
            other._ref_count = nullptr;
        }
        return *this;
    }

    [[nodiscard]] constexpr int  get() const noexcept { return _fd; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return _fd >= 0; }
    [[nodiscard]] uint32_t       use_count() const noexcept {
        return _ref_count ? _ref_count->load(std::memory_order_relaxed) : 0;
    }
    [[nodiscard]] FileType get_type() const noexcept { return _type; }
};

#include <stdexcept>

/// @brief Manage smart fd list.
template <typename SmartFdType, std::size_t NumberOfElements>
class SmartFdManager {
 private:
    std::array<SmartFdType, NumberOfElements> _list{};
    std::size_t                               _active_idx = 0;
    mutable std::mutex                        _mutex;

 public:
    // Constructor support move-only type like UniqueFileDescriptor)
    template <typename... Args>
    explicit SmartFdManager(Args&&... args) : _list{std::forward<Args>(args)...} {
        static_assert(std::is_same_v<SmartFdType, UniqueFileDescriptor> ||  //
                          std::is_same_v<SmartFdType, ShareFileDescriptor>,
                      "SmartFdType must be either UniqueFileDescriptor or ShareFileDescriptor.");

        static_assert((std::is_same_v<std::decay_t<Args>, SmartFdType> && ...),
                      "Args must be either UniqueFileDescriptor or ShareFileDescriptor.");
    }

    // Thread-safe access with bound checking exception handling
    [[nodiscard]] const SmartFdType& get_active_fd() const {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            return _list.at(_active_idx);
        } catch (const std::out_of_range& e) {
            // Handle or rethrow custom exception here
            throw std::runtime_error("Active index is out of bounds.");
        }
    }
    SmartFdType& get_active_fd_ref() {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            return _list.at(_active_idx);
        } catch (const std::out_of_range& e) {
            // Handle or rethrow custom exception here
            throw std::runtime_error("Active index is out of bounds.");
        }
    }

    SmartFdType& get_main_fd() {
        std::lock_guard<std::mutex> lock(_mutex);
        try {
            return _list.at(0);
        } catch (const std::out_of_range& e) {
            // Handle or rethrow custom exception here
            throw std::runtime_error("Active index is out of bounds.");
        }
    }

    void reset_active_fd(SmartFdType&& fd) {
        std::lock_guard<std::mutex> lock(_mutex);
        _list[_active_idx] = std::move(fd);
    }

    // Thread-safe index updates
    void set_active_fd(std::size_t active) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (active >= NumberOfElements) {
            throw std::out_of_range("Requested index exceeds array size.");
        }
        _active_idx = active;
    }

    // Thread-safe index updates
    const std::size_t get_active_fd_idx() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _active_idx;
    }

    // Reset helper
    void reset_main_fd() {
        std::lock_guard<std::mutex> lock(_mutex);
        _active_idx = 0;  // 0 means main fd
    }
};

}  // namespace HarisLinux