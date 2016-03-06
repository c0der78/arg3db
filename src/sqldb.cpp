/*!
 * @copyright ryan jennings (arg3.com), 2013
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include "sqldb.h"
#include "query.h"
#include "exception.h"
#include "resultset.h"
#include "sqlite/db.h"
#include "mysql/db.h"
#include "postgres/db.h"
#include "select_query.h"

using namespace std;

namespace arg3
{
    namespace db
    {
        ARG3_IMPLEMENT_EXCEPTION(database_exception, std::exception);

        ARG3_IMPLEMENT_EXCEPTION(no_such_column_exception, database_exception);

        ARG3_IMPLEMENT_EXCEPTION(record_not_found_exception, database_exception);

        ARG3_IMPLEMENT_EXCEPTION(binding_error, database_exception);

        void uri::parse(const string &url_s)
        {
            value = url_s;
            // do the manual implementation from stack overflow
            // with some mods for the port
            const string prot_end("://");
            string::const_iterator pos_i = search(url_s.begin(), url_s.end(), prot_end.begin(), prot_end.end());
            protocol.reserve(distance(url_s.begin(), pos_i));
            transform(url_s.begin(), pos_i, back_inserter(protocol), ptr_fun<int, int>(tolower));  // protocol is icase
            if (pos_i == url_s.end()) {
                return;
            }

            advance(pos_i, prot_end.length());

            string::const_iterator user_i = find(pos_i, url_s.end(), '@');
            string::const_iterator path_i;

            if (user_i != url_s.end()) {
                string::const_iterator pwd_i = find(pos_i, user_i, ':');

                if (pwd_i != user_i) {
                    password.assign(pwd_i, user_i);
                    user.assign(pos_i, pwd_i);
                } else {
                    user.assign(pos_i, user_i);
                }

                pos_i = user_i + 1;
            }

            path_i = find(pos_i, url_s.end(), '/');
            if (path_i == url_s.end()) {
                path_i = pos_i;
            }

            string::const_iterator port_i = find(pos_i, path_i, ':');
            string::const_iterator host_end;
            if (port_i != url_s.end()) {
                port.assign(*port_i == ':' ? (port_i + 1) : port_i, path_i);
                host_end = port_i;
            } else {
                host_end = path_i;
            }
            host.reserve(distance(pos_i, host_end));
            transform(pos_i, host_end, back_inserter(host), ptr_fun<int, int>(tolower));  // host is icase
            string::const_iterator query_i = find(path_i, url_s.end(), '?');
            path.assign(*path_i == '/' ? (path_i + 1) : path_i, query_i);
            if (query_i != url_s.end()) ++query_i;
            query.assign(query_i, url_s.end());
        }

        shared_ptr<sqldb> sqldb::from_uri(const string &uristr)
        {
            db::uri uri(uristr);
#ifdef HAVE_LIBSQLITE3
            if ("file" == uri.protocol) return make_shared<sqlite::db>(uri);
#endif
#ifdef HAVE_LIBMYSQLCLIENT
            if ("mysql" == uri.protocol) return make_shared<mysql::db>(uri);
#endif
#ifdef HAVE_LIBPQ
            if ("postgres" == uri.protocol || "postgresql" == uri.protocol) return make_shared<postgres::db>(uri);
#endif
            throw database_exception("unknown database " + uri.value);
        }

        sqldb::sqldb() : schema_factory_(this)
        {
        }

        sqldb::sqldb(const uri &connectionInfo) : connectionInfo_(connectionInfo), schema_factory_(this)
        {
        }

        void sqldb::query_schema(const string &tableName, std::vector<column_definition> &columns)
        {
            if (!is_open()) return;

            select_query pkq(this, {"tc.table_schema, tc.table_name, kc.column_name"});

            pkq.from("information_schema.table_constraints tc");

            pkq.join("information_schema.key_column_usage kc").on("kc.table_name = tc.table_name") and ("kc.table_schema = tc.table_schema");

            pkq.where("tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = $1", tableName);

            pkq.order_by("tc.table_schema, tc.table_name, kc.position_in_unique_constraint");

            auto primary_keys = pkq.execute();

            select_query info_schema(this, {"column_name", "data_type"});

            info_schema.from("information_schema.columns");

            info_schema.where("table_name = $1", tableName);

            auto rs = info_schema.execute();

            for (auto &row : rs) {
                column_definition def;

                // column name
                def.name = row["column_name"].to_value().to_string();

                if (def.name.empty()) {
                    continue;
                }

                def.pk = false;

                for (auto &pk : primary_keys) {
                    if (pk["column_name"].to_value() == def.name) {
                        def.pk = true;
                    }
                }

                // find type
                def.type = row["data_type"].to_value().to_string();

                columns.push_back(def);
            }
        }

        uri sqldb::connection_info() const
        {
            return connectionInfo_;
        }

        void sqldb::set_connection_info(const uri &value)
        {
            connectionInfo_ = value;
        }


        schema_factory *sqldb::schemas()
        {
            return &schema_factory_;
        }

        const schema_factory *sqldb::schemas() const
        {
            return &schema_factory_;
        }

        string sqldb::insert_sql(const std::shared_ptr<schema> &schema, const vector<string> &columns) const
        {
            ostringstream buf;

            buf << "INSERT INTO " << schema->table_name();

            buf << "(";

            buf << join_csv(columns);

            buf << ") VALUES(";

            buf << join_params(columns, false);

            buf << ");";

            return buf.str();
        }
    }
}
