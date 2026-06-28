#include "std17pch.h"

namespace HarisLinux {

enum class FileType { Generic, Pipe, NetworkSocket, UnixSocket, Epoll, EventFd };

inline void clean_up_fd_resource(int fd, FileType type, const std::string& file_path) noexcept {
    if (fd < 0) return;

    switch (type) {
        case FileType::NetworkSocket:
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
            break;

        case FileType::UnixSocket:
            ::shutdown(fd, SHUT_RDWR);
            [[fallthrough]];  // C++17 Attribute

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

    std::cout << "-------------------------- UNLINK fd: " << fd << "--------------------------\n";
}

class UniqueFileDescriptor {
 private:
    int         _fd        = -1;  // Managed raw file descriptor
    FileType    _type      = FileType::Generic;
    std::string _file_path = "";

 public:
    UniqueFileDescriptor() noexcept : _fd(-1), _type(FileType::Generic), _file_path("") {}

    explicit UniqueFileDescriptor(int fd, FileType type = FileType::Generic, std::string path = "") noexcept
        : _fd(fd), _type(type), _file_path(std::move(path)) {}

    // Destructor: Automatically closes the file descriptor when leaving stack scope
    ~UniqueFileDescriptor() {
        if (_fd >= 0) {
            clean_up_fd_resource(_fd, _type, _file_path);
        }
    }

    // Force move-only semantics at compile-time to prevent duplicate close calls
    UniqueFileDescriptor(const UniqueFileDescriptor&) = delete;
    UniqueFileDescriptor& operator=(const UniqueFileDescriptor&) = delete;

    // Move constructor:
    UniqueFileDescriptor(UniqueFileDescriptor&& other) noexcept
        : _fd(other._fd), _type(other._type), _file_path(std::move(other._file_path)) {
        other._fd = -1;  // Reset source to an invalid state
    }

    // Move assignment operator: Delete constexpr because ::close()
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

    // Inline accessors optimized for compile-time execution
    [[nodiscard]] constexpr int  get() const noexcept { return _fd; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return _fd >= 0; }
    [[nodiscard]] FileType       get_type() const noexcept { return _type; }
    const std::string&           get_path() const noexcept { return _file_path; }

    // Releases ownership without closing the underlying file descriptor
    constexpr int release() noexcept {
        int tmp = _fd;
        _fd     = -1;
        return tmp;
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

}  // namespace HarisLinux