#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include "sqlite3.h"

using database = std::unique_ptr<sqlite3, decltype(&sqlite3_close)> ;
database open_database(const char* dbname) {
    sqlite3* db = nullptr;
    auto rc = sqlite3_open(dbname, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Unable to open database '" << dbname << ": " << sqlite3_errmsg(db);
        sqlite3_close(db);
        std::exit(EXIT_FAILURE);
    }
    return database(db, sqlite3_close);
}

void execute(sqlite3* db, const char* sqlstmt) {
    char* errmsg = 0;
    auto rc = sqlite3_exec(db, sqlstmt, 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Unable to execute '" << sqlstmt << ": " << errmsg;
        sqlite3_free(errmsg);
        std::exit(EXIT_FAILURE);
    }
}

void create_table(sqlite3* db) {
    const char* stmt = R"~(create table IF NOT EXISTS user
    (
        id INTEGER not null primary key,
        area CHAR(6),
        age INTEGER not null,
        active INTEGER not null
    );)~"; 
    execute(db, stmt);
}

std::string get_random_area_code() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(100'000, 999'999);
    return std::to_string(distr(gen));
}

int get_random_age() {
    std::vector<int> ages{5, 10, 15};
    return ages[std::rand() % ages.size()];
}

void faker(sqlite3* db, unsigned int count=100'000) {
    for (auto i = 1; i < count+1; ++i) {
        int age = get_random_age();
        int active = int(std::rand() & 1);
        std::string stmt = "INSERT INTO user VALUES (NULL , ";
        if(bool(std::rand() & 1)) {
            std::string area = get_random_area_code();
            stmt +=  area + " , " + std::to_string(age) + " , " + std::to_string(active) + ")";
        }
        else
            stmt += "NULL ," + std::to_string(age) + " , " + std::to_string(active) + ")";
        execute(db, stmt.c_str());
        if (i % 100'000 == 0)
            std::cout << i << " rows inserted\n";
    }
}

int main() {
    auto naivedb = open_database("cppnaive.db");
    create_table(naivedb.get());
    execute(naivedb.get(), "PRAGMA journal_mode = WAL;");
    faker(naivedb.get(), 1'000'000);
    return 0;
}
