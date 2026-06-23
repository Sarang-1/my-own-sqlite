#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include "../utils.h"
#include "../query_parser.h"
#include <unordered_map>

using namespace std;

const int DB_HEADER_SIZE = 100;

unordered_map<string, int> parse_schema_to_map(const string& create_table_sql) {
    unordered_map<string, int> schema;
    size_t start = create_table_sql.find('(');
    if (start == string::npos) return schema;

    string cols_str = create_table_sql.substr(start + 1);
    size_t end = cols_str.rfind(')');
    if (end != string::npos) cols_str = cols_str.substr(0, end);

    stringstream ss(cols_str);
    string part;
    int idx = 0;

    while (getline(ss, part, ',')) {
        // Trim leading whitespace
        size_t first = part.find_first_not_of(" \t\r\n");
        if (first == string::npos) continue;
        string trimmed = part.substr(first);

        // Grab the column name (the first word)
        size_t space = trimmed.find_first_of(" \t\r\n");
        string col_name = (space == string::npos) ? trimmed : trimmed.substr(0, space);

        // Strip any SQL schema wrappers (e.g. "name" or [name])
        if (!col_name.empty() && (col_name.front() == '"' || col_name.front() == '\'' || col_name.front() == '`' || col_name.front() == '[')) col_name.erase(0, 1);
        if (!col_name.empty() && (col_name.back() == '"' || col_name.back() == '\'' || col_name.back() == '`' || col_name.back() == ']')) col_name.pop_back();

        schema[col_name] = idx++;
    }
    return schema;
}

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
   // 1. Search the schema table for a specific table name and return its root page
    int get_table_root_page(string target_table_name) {
        for (int i = 0; i < cell_count; i++) {
            stream.seekg(108 + (i * 2));
            unsigned short cell_offset = big_endian(stream, 2);
            stream.seekg(cell_offset);

            parse_varint(stream); // skip payload size
            parse_varint(stream); // skip rowid

            vector<string> row = parse_record(stream);

            // columns: 0:type, 1:name, 2:tbl_name, 3:rootpage, 4:sql
            if (row.size() >= 4 && row[0] == "table" && row[2] == target_table_name) {
                return stoi(row[3]); // Convert the rootpage string to an integer
            }
        }
        cerr << "Table not found in schema: " << target_table_name << endl;
        exit(1);
    }

    // 2. Jump to any page and read the 2-byte cell count sitting at offset +3
    unsigned short get_page_cell_count(int page_number) {
        // Apply the Golden Rules: Page 1 pays 100 tax, Page N pays (N-1) * page_size
        size_t page_offset = (page_number == 1) ? 100 : ((page_number - 1) * this->page_size);
        
        stream.seekg(page_offset + 3);
        return big_endian(stream, 2);
    }

    // 1. Upgraded metadata fetcher: returns { root_page_as_int, create_table_sql_string }
    pair<int, string> get_table_metadata(string target_table_name) {
        for (int i = 0; i < cell_count; i++) {
            stream.seekg(108 + (i * 2));
            unsigned short cell_offset = big_endian(stream, 2);
            stream.seekg(cell_offset);

            parse_varint(stream); // skip payload size
            parse_varint(stream); // skip rowid

            vector<string> row = parse_record(stream);

            // Columns: 0:type, 1:name, 2:tbl_name, 3:rootpage, 4:sql
            if (row.size() >= 5 && row[0] == "table" && row[2] == target_table_name) {
                return { stoi(row[3]), row[4] };
            }
        }
        cerr << "Table not found: " << target_table_name << endl;
        exit(1);
    }

    // 2. Scan a target page, parse every row, and print the requested column index
    // Added default parameters: filter_col_idx = -1 (meaning "no filter")
    void print_projected_rows(int page_number, const vector<int>& col_indices, int filter_col_idx = -1, string filter_val = "") {
        size_t page_offset = (page_number == 1) ? 100 : ((page_number - 1) * this->page_size);

        stream.seekg(page_offset + 3);
        unsigned short page_cells = big_endian(stream, 2);
        size_t cell_ptr_offset = page_offset + ((page_number == 1) ? 108 : 8);

        for (int i = 0; i < page_cells; i++) {
            stream.seekg(cell_ptr_offset + (i * 2));
            unsigned short cell_offset = big_endian(stream, 2);
            stream.seekg(page_offset + cell_offset);

            parse_varint(stream); // skip payload size
            parse_varint(stream); // skip rowid

            vector<string> row = parse_record(stream);

            // ---> THE WHERE CLAUSE GATEKEEPER <---
            if (filter_col_idx != -1) {
                // If the row is missing this column, or the value doesn't match: drop it.
                if (filter_col_idx >= row.size() || row[filter_col_idx] != filter_val) {
                    continue; 
                }
            }

            // Print the mapped columns separated by '|'
            for (size_t k = 0; k < col_indices.size(); k++) {
                int c_idx = col_indices[k];
                cout << (c_idx < row.size() ? row[c_idx] : ""); 
                if (k < col_indices.size() - 1) cout << "|";
            }
            cout << endl;
        }
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
    else if (command.find("SELECT COUNT") != string::npos) {
        // Quick & dirty shortcut: grab the very last word after the final space
        size_t last_space = command.rfind(' ');
        string table_name = command.substr(last_space + 1);

        // Strip off a trailing semicolon or quote just in case the tester gets funny
        while (!table_name.empty() && (table_name.back() == ';' || table_name.back() == '"' || table_name.back() == '\'')) {
            table_name.pop_back();
        }

        int root_page = db.get_table_root_page(table_name);
        unsigned short row_count = db.get_page_cell_count(root_page);

        cout << row_count << endl;
        return 0;
    }
    else if (command.find("SELECT ") == 0 && command.find(" COUNT") == string::npos) {
        string query = command.substr(7); // e.g. "name, color FROM apples WHERE color = 'Yellow'"
        
        size_t from_pos = query.find(" FROM ");
        string raw_cols = query.substr(0, from_pos); 
        string remainder = query.substr(from_pos + 6); // "apples WHERE color = 'Yellow'"

        string table_name;
        string where_col_name = "";
        string where_val = "";
        int filter_idx = -1;

        // 1. Check if a WHERE clause lives inside this query
        size_t where_pos = remainder.find(" WHERE ");
        if (where_pos != string::npos) {
            table_name = remainder.substr(0, where_pos);
            string raw_where = remainder.substr(where_pos + 7); // "color = 'Yellow'"

            size_t eq_pos = raw_where.find('=');
            if (eq_pos != string::npos) {
                where_col_name = raw_where.substr(0, eq_pos);
                where_val = raw_where.substr(eq_pos + 1);

                // Trim spaces off the column name
                size_t c_start = where_col_name.find_first_not_of(" \t");
                size_t c_end = where_col_name.find_last_not_of(" \t");
                if (c_start != string::npos) where_col_name = where_col_name.substr(c_start, c_end - c_start + 1);

                // Trim spaces AND strip the single quotes off the target value ('Yellow' -> Yellow)
                size_t v_start = where_val.find_first_not_of(" \t");
                size_t v_end = where_val.find_last_not_of(" \t");
                if (v_start != string::npos) {
                    where_val = where_val.substr(v_start, v_end - v_start + 1);
                    if (!where_val.empty() && (where_val.front() == '\'' || where_val.front() == '"')) where_val.erase(0, 1);
                    if (!where_val.empty() && (where_val.back() == '\'' || where_val.back() == '"')) where_val.pop_back();
                }
            }
        } else {
            table_name = remainder; // No WHERE clause, proceed as normal
        }

        while (!table_name.empty() && (table_name.back() == ';' || table_name.back() == '"' || table_name.back() == '\'')) {
            table_name.pop_back();
        }

        pair<int, string> meta = db.get_table_metadata(table_name);
        int root_page = meta.first;
        unordered_map<string, int> schema_map = parse_schema_to_map(meta.second);

        // 2. Resolve the WHERE column to an integer index (if requested)
        if (!where_col_name.empty()) {
            if (schema_map.find(where_col_name) == schema_map.end()) {
                cerr << "WHERE column '" << where_col_name << "' not found in schema." << endl;
                return 1;
            }
            filter_idx = schema_map[where_col_name];
        }

        // 3. Resolve the SELECT projected columns
        vector<int> projected_indices;
        stringstream ss_cols(raw_cols);
        string col_token;
        while (getline(ss_cols, col_token, ',')) {
            size_t first = col_token.find_first_not_of(" \t");
            size_t last = col_token.find_last_not_of(" \t");
            if (first == string::npos) continue;

            string clean_col = col_token.substr(first, last - first + 1);
            projected_indices.push_back(schema_map[clean_col]);
        }

        // 4. Run the scan!
        db.print_projected_rows(root_page, projected_indices, filter_idx, where_val);
        return 0;
    }
    else {
        QueryParser qp(command);
        // Put your count/query routing logic here
    }
    
    return 0;
}