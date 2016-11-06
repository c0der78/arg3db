
#include "session.h"
#include <sstream>
#include "../schema.h"
#include "../sqldb.h"
#include "resultset.h"
#include "statement.h"
#include "transaction.h"

using namespace std;

namespace rj
{
    namespace db
    {
        namespace sqlite
        {
            namespace helper
            {
                struct close_db {
                    void operator()(sqlite3 *p) const
                    {
                        if (p != nullptr) {
                            sqlite3_close(p);
                        }
                    }
                };
            }

            __attribute__((constructor)) void initialize(void)
            {
                auto factory = std::make_shared<sqlite::factory>();
                sqldb::register_session("sqlite", factory);
                sqldb::register_session("file", factory);
            }

            std::shared_ptr<rj::db::session_impl> factory::create(const uri &uri)
            {
                return std::make_shared<session>(uri);
            }

            session::session(const uri &info) : session_impl(info), db_(nullptr)
            {
            }

            session::session(session &&other) : session_impl(std::move(other)), db_(std::move(other.db_))
            {
                other.db_ = nullptr;
            }

            session &session::operator=(session &&other)
            {
                session_impl::operator=(std::move(other));

                db_ = std::move(other.db_);
                other.db_ = nullptr;

                return *this;
            }

            session::~session()
            {
                if (is_open()) {
                    close();
                }
            }

            std::vector<column_definition> session::get_columns_for_schema(const string &dbName, const string &tableName)
            {
                std::vector<column_definition> columns;

                if (tableName.empty()) {
                    throw database_exception("no table name to query schema");
                }

                auto rs = query("pragma table_info(" + tableName + ")");

                while (rs->next()) {
                    auto row = rs->current_row();
                    column_definition def;

                    // column name
                    def.name = row["name"].value().to_string();

                    // primary key check
                    def.pk = row["pk"].value().as<sql_number>();

                    // find type
                    def.type = row["type"].value().to_string();

                    def.autoincrement = def.pk;

                    def.default_value = row["dflt_value"].value().to_string();

                    columns.push_back(def);
                }

                return columns;
            }

            void session::open()
            {
                open(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI);
            }

            void session::open(int flags)
            {
                if (db_ != nullptr) {
                    return;
                }

                sqlite3 *conn = nullptr;

                if (sqlite3_open_v2(connection_info().path.c_str(), &conn, flags, nullptr) != SQLITE_OK) {
                    throw database_exception(last_error());
                }

                db_ = shared_ptr<sqlite3>(conn, helper::close_db());
            }

            bool session::is_open() const
            {
                return db_ != nullptr && db_;
            }

            void session::close()
            {
                // the shared_ptr destructor should close
                db_ = nullptr;
            }

            string session::last_error() const
            {
                string buf;

                if (db_ == nullptr) {
                    return string();
                }

                buf += sqlite3_errcode(db_.get());
                buf += ": ";
                buf += sqlite3_errmsg(db_.get());

                return buf;
            }

            long long session::last_insert_id() const
            {
                if (db_ == nullptr) {
                    return 0;
                }
                return sqlite3_last_insert_rowid(db_.get());
            }

            int session::last_number_of_changes() const
            {
                if (db_ == nullptr) {
                    return 0;
                }
                return sqlite3_changes(db_.get());
            }

            std::shared_ptr<resultset_impl> session::query(const string &sql)
            {
                sqlite3_stmt *stmt;

                if (db_ == nullptr) {
                    throw database_exception("session::execute database not open");
                }

                if (sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                    throw database_exception(last_error());
                }

                return make_shared<resultset>(shared_from_this(), shared_ptr<sqlite3_stmt>(stmt, helper::stmt_delete()));
            }

            bool session::execute(const string &sql)
            {
                sqlite3_stmt *stmt;

                if (db_ == nullptr) {
                    throw database_exception("session::execute database not open");
                }

                if (sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                    return false;
                }

                int res = sqlite3_step(stmt);

                sqlite3_finalize(stmt);

                return res == SQLITE_OK || res == SQLITE_ROW || res == SQLITE_DONE;
            }

            shared_ptr<session::statement_type> session::create_statement()
            {
                return make_shared<statement>(static_pointer_cast<sqlite::session>(shared_from_this()));
            }

            std::shared_ptr<transaction_impl> session::create_transaction() const
            {
                return make_shared<sqlite::transaction>(db_);
            }

            std::shared_ptr<transaction_impl> session::create_transaction(transaction::type type) const
            {
                return make_shared<sqlite::transaction>(db_, type);
            }

            std::string session::bind_param(size_t index) const
            {
                return "?" + std::to_string(index);
            }

            bool session::supports_named_parameters() const
            {
                return true;
            }
        }
    }
}
