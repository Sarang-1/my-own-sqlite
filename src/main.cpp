#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include "../utils.h"
#include "../query_parser.h"

using namespace std;

const int DB_HEADER_SIZE = 100;

class DB {
public:
    ifstream stream;
    unsigned short page_size,cell_count;

    DB(string db_file_path) {
        this->stream = ifstream(db_file_path, ios::binary);
        if (!this->stream.is_open()) {
            cerr << "Failed to open database file." << endl;
            exit(1);
        }
        
        // Extract Page Size
        this->stream.seekg(16);
        this->page_size = big_endian(this->stream, 2);

        this->stream.seekg(103);
        this->cell_count = big_endian(stream, 2);
    }

    void print_table_names() {
        // Page 1 contains the sqlite_schema.
        // The cell count is located at byte offset 3 of the B-Tree page header.
        // Since Page 1 has a 100-byte DB header, the cell count is at 100 + 3 = 103.
        

        vector<string> table_names;

        // The cell pointer array starts immediately after the 8-byte B-Tree header.
        // For Page 1, this is 100 (DB Header) + 8 (B-Tree Header) = 108.
        for (int i = 0; i < cell_count; i++) {
            stream.seekg(108 + (i * 2));
            unsigned short cell_offset = big_endian(stream, 2);

            // On Page 1, the page offset is 0, so the absolute offset is exactly the cell_offset.
            stream.seekg(cell_offset);

            // 1. Skip Payload Size (Varint)
            parse_varint(stream);
            
            // 2. Skip RowID (Varint)
            parse_varint(stream);

            // 3. Parse the Record
            vector<string> row = parse_record(stream);

            // The sqlite_schema columns are: 
            // 0: type, 1: name, 2: tbl_name, 3: rootpage, 4: sql
            if (row.size() >= 3 && row[0] == "table" && row[1] != "sqlite_sequence") {
                table_names.push_back(row[2]);
            }
        }

        // Print tables separated by spaces
        for (size_t i = 0; i < table_names.size(); i++) {
            cout << table_names[i] << (i == table_names.size() - 1 ? "" : " ");
        }
        cout << endl;
    }
};

int main(int argc, char *argv[]) {
    // Disable output buffering
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc != 3) {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string database_file_path = argv[1];
    std::string command = argv[2];

    DB db(database_file_path);

    if (command == ".dbinfo") {
        cout << "database page size: " << db.page_size << endl;
        cout << "number of tables: " << db.cell_count << endl;
        return 0;
    } 
    else if (command == ".tables") {
        db.print_table_names();
        return 0;
    } 
    else {
        QueryParser qp(command);
        // Put your count/query routing logic here
    }
    
    return 0;
}