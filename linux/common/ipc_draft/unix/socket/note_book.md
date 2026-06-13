# Work flow
```txt
1. SocketServer server(AF_type, types, server_flags)
    .start_server(path, port = 0) {
            init -> socket_fd
            Base::configure_addr(path,port) + ... -> mode: af, type
        }
    -> accept_connection() { accept socket_fd api }

2. Init header and payload

3. Loop {
    ? server.receive_packet:
        .check data_type and log data
}
```

```txt
SocketClient client(AF_type, type, client_flags)
    .connect_to_server(path) {
        initialize_socket()
        bind // api -> _socket_fd
        Base::configure_addr(path,port) + ... -> mode: af, type
    }
    .get_socket_fd() fd
    setsockopt(...) // timeout option api, todo wrapper
    .send_packet(Num, "init")
    .send_packet(Text, ...)
    .send_packet(Media, vector<uint8_t>)
    .thread send Hearthbeat
```

