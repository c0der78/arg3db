/*!
 * @file db.h
 * a mysql specific database
 */
#ifndef RJ_DB_MYSQL_SESSION_H
#define RJ_DB_MYSQL_SESSION_H

#include <mysql/mysql.h>
#include "../session.h"
#include "../session_factory.h"

namespace rj
{
    namespace db
    {
        struct sqldb;

        namespace mysql
        {
            class factory : public session_factory
            {
               public:
                std::shared_ptr<rj::db::session_impl> create(const uri &uri);
            };

            /*!
             * a mysql specific implementation of a database
             */
            class session : public rj::db::session_impl, public std::enable_shared_from_this<session>
            {
                friend sqldb;
                friend class resultset;
                friend class statement;
                friend class factory;

               protected:
                std::shared_ptr<MYSQL> db_;

               public:
                /*!
                 * default constructor takes a uri to connect to
                 * @param connInfo the uri connection info
                 */
                session(const uri &connInfo);

                /* boilerplate */
                session(const session &other) = delete;
                session(session &&other);
                session &operator=(const session &other) = delete;
                session &operator=(session &&other);
                virtual ~session();

                /* sqldb overrides */
                bool is_open() const;
                void open();
                void close();
                long long last_insert_id() const;
                int last_number_of_changes() const;
                std::string last_error() const;
                std::shared_ptr<resultset_impl> query(const std::string &sql);
                bool execute(const std::string &sql);
                std::shared_ptr<statement_type> create_statement();
                std::shared_ptr<transaction_impl> create_transaction() const;
                std::vector<column_definition> get_columns_for_schema(const std::string &dbName, const std::string &tablename);
                std::string bind_param(size_t index) const;
            };
        }
    }
}

#endif
