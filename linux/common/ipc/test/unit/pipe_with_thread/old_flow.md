# POSIX Pipe System Flow Architecture

This document describes the Inter-Process Communication (IPC) flow between the **Server** and **Client** using POSIX **Named Pipes (FIFOs)**. The system implements a **Packet-based (Header + Payload)** framing structure and a **Feedback Loop** to detect data loss.

---

## 1. Sequence Diagram
```tree
[ PipeClient ]                                              [ PipeServer ]

      |                                                           |
      | ---- (1) Initialization & Connection -------------------> | (Creates FIFO /tmp/my_test_pipe)
      |                                                           | (Opens in ReadOnly/Feedback mode)
      |                                                           |
      | ---- (2) SendPacket (Header + Payload) -----------------> | (Reads Header -> Allocates memory)
      |     [Type: Text/Number/Media | Seq: N]                    | (Reads Payload based on Size)
      |                                                           | (Decodes & Prints data to console)
      |                                                           |
      | <--- (3) Send Feedback Response (Feedback Mode) --------- | (Sends empty packet with Seq: N)
      |     [Type: Command | Seq: N]                              |
      |                                                           |
      | ---- (4) CheckFeedback() -------------------------------> |
      |     (Compares Sent Seq vs Received Seq)                   |
      V                                                           V
```

---

## 2. Step-by-Step Flow Execution

### Step 1: Initialization & Handshake
* **Server**: 
  * Invokes `mkfifo()` to create the primary named pipe. If `Feedback` mode is enabled, it creates an additional response pipe (`_fb`).
  * Executes `Start()`, opening the read descriptor (`O_RDONLY`). The execution thread blocks here until a client connects.
* **Client**: 
  * Executes `Connect()`, opening the write descriptor (`O_WRONLY`) on the primary pipe. Once opened, the Server thread unblocks.

### Step 2: Packet Serialization & Transmission (Client)
1. Raw source data (String text, integer number, or raw media byte array) is serialized into a standard byte stream (`std::vector<uint8_t>`).
2. The internal tracking variable `current_seq` is incremented by 1.
3. The structural metadata is bound to a fixed-size `PacketHeader` utilizing the `__attribute__((packed))` attribute.
4. The `SendPacket()` API performs a two-stage sequential write:
   * **Stage 1**: Writes the exact `PacketHeader` structure (fixed at 17 bytes).
   * **Stage 2**: Writes the raw `Payload` memory buffer (variable length based on `payload_size`).

### Step 3: Packet Deserialization & Processing (Server)
1. The `ReadPacket()` API performs a two-stage sequential read:
   * **Stage 1**: Reads exactly 17 bytes from the stream to populate the `PacketHeader`.
   * **Stage 2**: Resizes the payload `data` vector dynamically, then reads the remaining stream up to the exact `payload_size`.
2. The `ProcessIncoming()` method evaluates the `header.type` field:
   * **DataType::Text**: Reconstructs the stream into a `std::string` for plain text display.
   * **DataType::Number**: Utilizes `std::memcpy` to safely cast the memory block back into a native `int`.
   * **DataType::Media / Command**: Sanitizes the binary buffer and outputs a readable Hexadecimal representation.

### Step 4: Verification & Feedback Loop
1. Upon successful packet processing, if the Server is configured in `Feedback` mode, it echoes back the identical `sequence_id` via an empty packet through the `_fb` pipe.
2. The Client triggers `CheckFeedback()` to non-blockingly or synchronously read from the feedback pipe.
3. The Client evaluates the response: `If (fb_header.sequence_id != current_seq)` $\rightarrow$ Triggers a `Data Lost Detected` alert to indicate network/buffer drops.
