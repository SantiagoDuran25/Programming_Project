TinyKV – TCP Key/Value Storage Server

TinyKV is a lightweight in-memory key/value database with persistent saving, designed in C using socket programming, threading, and hash-based storage. Multiple clients can connect simultaneously through a simple plaintext protocol (SET, GET, DEL, KEYS, SAVE, EXIT).

======================================================
🚀 FEATURES

• Multi-client TCP server using POSIX threads
• Thread-safe hash table with mutex locking
• In-memory key/value storage with constant-time lookup
• Persistent file storage using a dump file (dump.kv)
• Robust error handling for malformed commands
• Simple readable text protocol
• Clean modular C code (Data store + Networking + Client)

======================================================
🔐 CONCURRENCY & DATA SAFETY

TinyKV supports multiple clients at the same time.
To prevent race conditions when accessing shared data:

All data modifications occur inside critical sections

Storage operations (SET, GET, DEL, SAVE) are protected using a mutex

Each client runs in its own thread via pthread_create

This ensures data integrity even under concurrent use.

======================================================
⚠️ ERROR HANDLING

The server detects malformed or incomplete commands and responds without crashing. Examples:

ERROR: Usage SET <key> <value>
ERROR: Unknown command
NOT FOUND


Invalid deletions also return:

NOT FOUND

======================================================
📦 PROJECT STRUCTURE

├── CMakeLists.txt
├── Common.h
├── Client.c
├── Server.c
├── Data_Store.c
├── Data_Store.h
├── run_tests.sh (simple functional test)
└── dump.kv (created automatically after SAVE)

======================================================
🛠️ COMPILATION INSTRUCTIONS

⚠️ IMPORTANT: In every new terminal, activate the C environment:

conda activate myenv


Create and enter the build directory:

mkdir build
cd build


Run CMake configuration:

cmake ..


Compile the program:

cmake --build .

======================================================
▶️ RUNNING THE PROGRAM

⚠️ IMPORTANT: Each terminal must run:

conda activate myenv


🖥️ Start the server (Terminal 1):

./server 5001


💬 Start the client (Terminal 2):

./client 127.0.0.1 5001

======================================================
💻 COMMAND PROTOCOL (CLIENT)
Command	Description
SET <key> <value>	Stores a key/value pair
GET <key>	Prints value or NOT FOUND
DEL <key>	Deletes key
KEYS	Lists keys
SAVE	Saves to dump.kv
EXIT	Disconnects

📌 Limits (Defined in Common.h)

Key length ≤ 64

Value length ≤ 256

======================================================
📌 EXAMPLE SESSION
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
Bye

======================================================
🧠 DESIGN OVERVIEW

📍 DATA STORAGE

Hash table using linked lists (HASH_SIZE = 128)

Average constant-time access

Persistent dump file (dump.kv)

Core functions:

kv_set, kv_get, kv_delete, kv_keys, kv_save, kv_init

📍 NETWORKING

TCP server using AF_INET, SOCK_STREAM

One thread per client (pthread_create)

Plain text communication protocol

📍 THREAD SAFETY

Mutex ensures exclusive access to the shared store

All data operations occur inside locked critical sections

======================================================
🧪 SIMPLE TEST SCRIPT

A minimal test script verifies basic functionality:

./run_tests.sh


Expected output:

OK
10
Bye

======================================================
💾 PERSISTENCE (SAVE COMMAND)

When user enters: SAVE
→ All key/value pairs are written into dump.kv.

When the server starts:
→ If dump.kv exists, data is automatically loaded.

======================================================
🎉 ENJOY TINYKV!

A tiny, thread-safe database that fits in your terminal 😊
