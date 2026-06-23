# Custom SQLite Database & Query Engine

A low-level relational database storage and query engine built from scratch in **C++23**. This project natively parses, navigates, and queries binary SQLite `.db` database files directly from disk without using the official SQLite library or any external relational third-party dependencies.

Developed as part of a deep dive into database internals, low-level binary I/O, and file-format reverse engineering.

---

## 🚀 Core Features

* **Zero-Dependency Binary Parsing:** Reads and decodes native SQLite database page configurations, extracting the page size, table cell counts, and file headers directly from the raw bitstream.
* **Variable-Length Quantity (Varint) Decoder:** Implements a custom Huffman-style bit-shifting stream parser to unpack SQLite’s 7-bit encoded variable integers used for payload offsets and record lengths.
* **Recursive B-Tree Traversal:** Dynamically navigates both **Interior Table Nodes (`0x05`)** for hierarchical page routing and **Leaf Table Nodes (`0x0D`)** for binary row payload extraction.
* **High-Performance $O(\log N)$ Index Scanner:** Intercepts incoming queries to check for indexed columns (`0x02` interior / `0x0A` leaf index pages), executing highly-optimized binary searches on 1GB+ databases in **under 5 milliseconds** instead of relying on slow, memory-heavy flat-file table scans.
* **Schema-to-Symbol Mapping:** Dynamically reconstructs an in-memory symbol table map from physical `CREATE TABLE` schema definitions to manage robust column projections and instant `WHERE` clause token filtering.

---

## 🛠️ Architecture & Core Mechanics

The query processor executes relational logic using a structured layout divided between physical file layouts and logical execution structures:

### Physical File Layout
* **Page 1 Tax:** Automatically accounts for the initial 100-byte database header restriction.
* **Interior vs. Leaf Logic:** Interprets the Rightmost Child Pointer array embedded inside static 12-byte interior headers as a fallback catch-all for unbounded index upper keys.
* **RowID Optimization:** Detects when an `INTEGER PRIMARY KEY` column payload is optimized out of disk storage (represented as a `0x00` physical NULL type) and injects the raw cell B-Tree RowID back into the runtime projection array.

---

## 📦 Prerequisites & Local Setup

To compile and run this engine locally, you will need a compiler that supports the modern **C++23 standard** along with **CMake**.

### 1. Build the Binary
Clone the repository, initialize your build environment, and compile the executable using CMake:

```bash
# Generate the build files
cmake -B build -S .

# Build the project targets
cmake --build ./build