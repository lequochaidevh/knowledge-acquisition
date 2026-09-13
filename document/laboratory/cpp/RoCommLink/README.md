
```txt
.
├── CMakeLists.txt                      # Root CMake managing the entire build pipeline
├── cmake/                              # Optional: Custom CMake modules / compiler flags
├── include/                            # Public Header Directory (Exportable)
│   └── core/
│       ├── common/                     # Utility and general infrastructure
│       │   ├── byte_utilities.hpp
│       │   └── task_queue.hpp
│       ├── transport/                  # I/O Mediums (Network, Serial, IPC)
│       │   ├── io_interface.hpp
│       │   └── udp_transport.hpp
│       ├── protocol/                   # Custom Binary Protocol Serialization & Framework
│       │   ├── packet.hpp
│       │   ├── comlink_parser.hpp
│       │   └── packet_serializer.hpp
│       └── service/                    # Top-level conductor nodes orchestrating the pipeline
│           ├── comlink_dispatcher.hpp
│           └── com_link.hpp
├── src/                                # Private Implementation Directory (.cpp files)
│   └── core/
│       ├── common/
│       │   └── task_queue.cpp
│       ├── transport/
│       │   └── udp_transport.cpp
│       ├── protocol/
│       │   ├── comlink_parser.cpp
│       │   └── packet_serializer.cpp
│       └── service/
│           ├── comlink_dispatcher.cpp
│           └── com_link.cpp
└── tests/                              # Isolated Test Suite Architecture
    ├── CMakeLists.txt
    ├── functional/
    │   ├── *.h                         # Todo: Test execution entry point (GTest or Catch2 runner)
    └── main_test.cpp
```