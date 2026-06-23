# SQLite Storage & Query Engine from Scratch (C++23)

I built a lightweight relational database storage engine from scratch in C++23. It parses, navigates, and queries binary SQLite .db files directly from disk. It does not use the official SQLite library or any external database dependencies - just raw binary file I/O and stream parsing.

The primary goal was to understand low-level database internals: how records are packed onto disk, how variable-length integers (Varints) save space, and how B-Trees scale queries out from O(N) full table scans to O(log N) index lookups on massive files.

---

## How It Works Under the Hood

* **Lazy Disk Seeking:** The engine uses std::ifstream::seekg to jump to exact byte offsets instead of loading the entire database file into RAM. This keeps the memory footprint tiny, even when querying gigabyte-scale databases.
* **The B-Tree Engine:** It implements recursive page traversal. It identifies the page type at runtime, navigating **Interior Pages (0x05)** via child pointers to route down the tree, and reading data records off **Leaf Pages (0x0D)**.
* **Self-Optimizing Index Scans:** When a query includes a WHERE filter, the engine checks sqlite_schema to see if an index exists for that column. If it finds one, it switches to the index B-Tree (0x02/0x0A pages), extracts the matching RowIDs, and performs direct point-lookups on the main table. This drops execution time on a 1GB database from seconds to under 5 milliseconds.
* **Primary Key Overriding:** SQLite optimizes storage by stripping out the INTEGER PRIMARY KEY column from the row payload if it matches the B-Tree RowID. My engine catches the RowID during cell decoding and dynamically restores it into the row array to prevent NULL field errors during projection.

---

## Complete Command Reference

The program acts as a CLI tool that accepts a database file path and an instruction/SQL string. Here is every command you can run:

### 1. Database Metadata (.dbinfo)
Extracts global settings from the 100-byte file header on Page 1.

Command:
```c++
./your_program.sh sample.db .dbinfo
```

Expected Output:
```c++
database page size: 4096
number of tables: 3
```

### 2. List Tables (.tables)
Scans Page 1's schema cells to find user-created table structures, skipping internal sequence metadata.

Command:
```c++
./your_program.sh sample.db .tables
```

Expected Output:
```c++
apples superheroes companies
```

### 3. Aggregate Rows (SELECT COUNT(*))
Jumps directly to the target table's root page and extracts the 2-byte cell count integer located at the page header offset.

Command:
```c++
./your_program.sh sample.db "SELECT COUNT(*) FROM apples"
```

Expected Output:
```c++
4
```

### 4. Single-Column Projection (SELECT col)
Parses the table's CREATE TABLE schema to find the target column's structural index, then prints that specific payload column for all leaf rows.

Command:
```c++
./your_program.sh sample.db "SELECT name FROM apples"
```

Expected Output:
```c++
Granny Smith
Fuji
Honeycrisp
Golden Delicious
```

### 5. Multi-Column Projection (SELECT col1, col2)
Generates an internal symbol map of requested column positions and pieces the text records together separated by a pipe (|).

Command:
```c++
./your_program.sh sample.db "SELECT name, color FROM apples"
```

Expected Output:
```c++
Granny Smith|Light Green
Fuji|Red
Honeycrisp|Blush Red
Golden Delicious|Yellow
```

### 6. Table Scan with Filtering (SELECT ... WHERE)
Applies an in-memory string-matching filter onto leaf data attributes during table parsing.

Command:
```c++
./your_program.sh sample.db "SELECT name, color FROM apples WHERE color = 'Yellow'"
```

Expected Output:
```c++
Golden Delicious|Yellow
```

### 7. Fast Index Scan (Large Datasets)
Automatically triggers when querying an indexed column (like country in the companies database). It bypasses full table scans entirely, walking the index tree first.

Command:
```c++
./your_program.sh companies.db "SELECT id, name FROM companies WHERE country = 'eritrea'"
```

Expected Output:
```c++
121311|unilink s.c.
2102438|orange asmara it solutions
5729848|zara mining share company
6634629|asmara rental
```

---

## Setup & Compilation

The project uses CMake and requires a compiler supporting C++23.

### Build Instructions

Command:
```c++
cmake -B build -S
``` 

Command:
```c++
cmake --build ./build
```

### Cross-Platform Note
The execution script (your_program.sh) detects the operating system environment at runtime. On a local Windows machine, it explicitly hooks into a w64devkit MinGW toolchain, while natively executing via standard Unix paths when deployed inside remote Linux testing pipelines.