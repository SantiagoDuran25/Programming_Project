# TinyKV – TCP Key/Value Storage Server

TinyKV is a lightweight in-memory key/value database with persistent saving, designed in C using socket programming, threading, and hash-based storage. Multiple clients can connect simultaneously through a simple plaintext protocol (SET, GET, DEL, KEYS, SAVE, EXIT).

======================================================
🚀 FEATURES
======================================================
• Multi-client TCP server using POSIX threads  
• In-memory hash table key/value store  
• Persistent file storage using a dump file (dump.kv)  
• Simple readable protocol  
• Clean modular C code (Data store + Networking + Client)

======================================================
📦 PROJECT STRUCTURE
======================================================
├── CMakeLists.txt  
├── Common.h  
├── Client.c  
├── Server.c  
├── Data_Store.c  
├── Data_Store.h  
└── dump.kv (created automatically after SAVE)

======================================================
🛠️ COMPILATION INSTRUCTIONS
======================================================

⚠️ IMPORTANT: In every new terminal, activate the C environment:
    conda activate myenv

1) Create and enter the build directory:
    mkdir build
    cd build

2) Run CMake configuration:
    cmake ..

3) Compile the program:
    cmake --build .

======================================================
▶️ RUNNING THE PROGRAM
======================================================

⚠️ IMPORTANT: Each terminal must run:
    conda activate myenv

🖥️ Start the server (Terminal 1):
    ./server 5001

💬 Start the client (Terminal 2):
    ./client 127.0.0.1 5001

======================================================
💻 COMMAND PROTOCOL (CLIENT)
======================================================

SET <key> <value>   → Stores a key/value pair  
GET <key>           → Retrieves value by key  
DEL <key>           → Deletes a key  
KEYS                → Lists stored keys  
SAVE                → Saves data into dump.kv  
EXIT                → Disconnects the client

======================================================
📌 EXAMPLE SESSION
======================================================
> SET name tomas  
OK  
> GET name  
tomas  
> SET city madrid  
OK  
> KEYS  
name  
city  
> DEL name  
DELETED  
> SAVE  
SAVED  
> EXIT  

======================================================
🧠 DESIGN OVERVIEW
======================================================

📍 DATA STORAGE
- Uses a linked-list hash table (HASH_SIZE = 128)
- Constant-time average lookup using a string hash
- Persistence via dump.kv
- Commands implemented in Data_Store.c:
  kv_set, kv_get, kv_delete, kv_keys, kv_save

📍 NETWORKING
- TCP server using AF_INET and SOCK_STREAM
- Each client is handled in a separate thread (pthread)
- Client/Server exchange plain text messages

======================================================
💾 PERSISTENCE (SAVE COMMAND)
======================================================
When user enters: SAVE  
→ All key/value pairs are written into dump.kv

When the server starts:  
→ If dump.kv exists, data is automatically loaded.

======================================================
🎉 ENJOY TINYKV!
======================================================
A tiny database that fits in your terminal 😊
