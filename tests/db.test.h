#ifndef ARG3_TEST_DB_H
#define ARG3_TEST_DB_H

#include <unistd.h>
#include "sqldb.h"
#include "record.h"
#include "sqlite/session.h"
#include "mysql/session.h"
#include "postgres/session.h"
#include "uri.h"

#if !defined(HAVE_LIBMYSQLCLIENT) && !defined(HAVE_LIBSQLITE3) && !defined(HAVE_LIBPQ)
#error "Mysql, postgres or sqlite is not installed on the system"
#endif

class test_session
{
   public:
    virtual void setup() = 0;
    virtual void teardown() = 0;
};

std::string get_env_uri(const char *name, const std::string &def);

#if defined(HAVE_LIBSQLITE3)

class test_sqlite3_factory : public arg3::db::session_factory
{
   public:
    std::shared_ptr<arg3::db::session_impl> create(const arg3::db::uri &value);
};

class test_sqlite3_session : public arg3::db::sqlite::session, public test_session
{
    friend class test_sqlite3_factory;

   public:
    using arg3::db::sqlite::session::session;

    void setup();

    void teardown();
};

#endif

#if defined(HAVE_LIBMYSQLCLIENT)

class test_mysql_factory : public arg3::db::session_factory
{
   public:
    std::shared_ptr<arg3::db::session_impl> create(const arg3::db::uri &value);
};

class test_mysql_session : public arg3::db::mysql::session, public test_session
{
    friend class test_mysql_factory;

   public:
    using arg3::db::mysql::session::session;

    void setup();

    void teardown();
};

#endif

#if defined(HAVE_LIBPQ)

class test_postgres_factory : public arg3::db::session_factory
{
   public:
    std::shared_ptr<arg3::db::session_impl> create(const arg3::db::uri &value);
};

class test_postgres_session : public arg3::db::postgres::session, public test_session
{
    friend class test_postgres_factory;

   public:
    using arg3::db::postgres::session::session;

    void setup();

    void teardown();
};

#endif

extern std::shared_ptr<arg3::db::session> current_session;

void register_test_sessions();

void setup_current_session();

void teardown_current_session();

class user : public arg3::db::record<user>
{
   public:
    constexpr static const char *const TABLE_NAME = "users";

    using arg3::db::record<user>::record;

    user(const std::shared_ptr<arg3::db::session> &sess = current_session) : record(sess->get_schema(TABLE_NAME))
    {
    }

    user(long long id, const std::shared_ptr<arg3::db::session> &sess = current_session) : user(sess->get_schema(TABLE_NAME))
    {
        set_id(id);
        refresh();
    }

    /*!
     * required constructor
     */
    user(const std::shared_ptr<arg3::db::schema> &schema) : record(schema)
    {
    }

    std::string to_string()
    {
        std::ostringstream buf;

        buf << id() << ": " << get("first_name") << " " << get("last_name");

        return buf.str();
    }
};


#endif
