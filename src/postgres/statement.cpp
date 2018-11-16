
#include "statement.h"
#include <algorithm>
#include "../exception.h"
#include "resultset.h"
#include "session.h"

using namespace std;

namespace coda::db::postgres {
      namespace helper {
        void res_delete::operator()(PGresult *p) const {
          if (p != nullptr) {
            PQclear(p);
          }
        }
      }  // namespace helper
      statement::statement(const std::shared_ptr<postgres::session> &sess) : sess_(sess), stmt_(nullptr) {
        if (sess_ == nullptr) {
          throw database_exception("no database provided to postgres statement");
        }
      }

      void statement::prepare(const string &sql) {
        if (!sess_ || !sess_->is_open()) {
          throw database_exception("postgres database not open");
        }

        sql_ = bindings_.prepare(sql);
      }

      bool statement::is_valid() const noexcept { return !sql_.empty(); }

      unsigned long long statement::last_number_of_changes() {
        if (stmt_ == nullptr) {
          return 0;
        }
        char *changes = PQcmdTuples(stmt_.get());

        unsigned long long value = 0;

        if (changes != nullptr && *changes != 0) {
          try {
            value = static_cast<unsigned long long int>(stoi(changes));
          } catch (const std::exception &e) {
            value = 0;
          }
        }
        if (sess_ != nullptr) {
          sess_->set_last_number_of_changes(value);
        }

        return value;
      }

      string statement::last_error() {
        if (sess_ == nullptr) {
          return "no database";
        }
        return sess_->last_error();
      }

      statement &statement::bind(size_t index, const sql_value &value) {
        bindings_.bind(index, value);
        return *this;
      }

      statement &statement::bind(const string &name, const sql_value &value) {
        bindings_.bind(name, value);
        return *this;
      }

      statement::resultset_type statement::results() {
        if (sess_ == nullptr) {
          throw database_exception("statement::results invalid database");
        }

        PGresult *res = PQexecParams(sess_->db_.get(), sql_.c_str(),
                                     static_cast<int>(bindings_.num_of_bindings()), bindings_.types_,
                                     bindings_.values_, bindings_.lengths_, bindings_.formats_, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
          throw database_exception(last_error());
        }

        stmt_ = shared_ptr<PGresult>(res, helper::res_delete());

        return resultset_type(make_shared<resultset>(sess_, stmt_));
      }

      bool statement::result() {
        if (sess_ == nullptr) {
          throw database_exception("statement::results invalid database");
        }

        PGresult *res = PQexecParams(sess_->db_.get(), sql_.c_str(),
                                     static_cast<int>(bindings_.num_of_bindings()), bindings_.types_,
                                     bindings_.values_, bindings_.lengths_, bindings_.formats_, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK) {
          PQclear(res);
          return false;
        }

        stmt_ = shared_ptr<PGresult>(res, helper::res_delete());

        return true;
      }

      void statement::finish() {
        stmt_ = nullptr;
        sql_.clear();
      }

      void statement::reset() {
        if (stmt_ != nullptr) {
          stmt_ = nullptr;
        }
      }

      long long statement::last_insert_id() {
        if (stmt_ == nullptr) {
          return 0;
        }

        Oid oid = PQoidValue(stmt_.get());

        if (oid != InvalidOid) {
          return oid;
        }

        long long value = 0;

        if (PQntuples(stmt_.get()) <= 0) {
          return value;
        }
        auto val = PQgetvalue(stmt_.get(), 0, 0);
        if (val != nullptr) {
          try {
            value = stoll(val);
          } catch (const std::exception &e) {
            value = 0;
          }
        }

        sess_->set_last_insert_id(value);

        return value;
      }

      size_t statement::num_of_bindings() const noexcept { return bindings_.num_of_bindings(); }
}  // namespace coda::db::postgres
