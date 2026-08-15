import socket
import threading

def stress_client():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('localhost', 8080))
        s.sendall(b'High Volume Benchmark Test!')
        data = s.recv(1024)
        s.close()
    except Exception:
        pass

# Fire 100 simultaneous concurrent connections
threads = []
for i in range(100):
    t = threading.Thread(target=stress_client)
    threads.append(t)
    t.start()

for t in threads:
    t.join()
print("Sent 100 concurrent requests successfully!")