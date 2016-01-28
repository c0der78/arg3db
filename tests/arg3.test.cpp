#include <bandit/bandit.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "db.test.h"

using namespace bandit;

#if !defined(HAVE_LIBMYSQLCLIENT) && !defined(HAVE_LIBSQLITE3) && !defined(HAVE_LIBPQ)
#error "Mysql, postgres or sqlite is not installed on the system"
#endif

int run_db_test(int argc, char *argv[])
{
    if (!testdb) {
        return 1;
    }

    // run the uncached test
    if (bandit::run(argc, argv)) {
        return 1;
    }

    // run the cached test
    testdb->set_cache_level(arg3::db::sqldb::CACHE_RESULTSET);
    cout << "setting cache level" << endl;
    return bandit::run(argc, argv);
}

int main(int argc, char *argv[])
{
    arg3::db::log::set_level(arg3::db::log::Error);

#ifdef HAVE_LIBSQLITE3
#ifdef TEST_SQLITE
    puts("running sqlite3 tests");
    testdb = &sqlite_testdb;
    if (run_db_test(argc, argv)) {
        return 1;
    }
#endif
#else
    cout << "Sqlite not supported" << endl;
#endif

#ifdef HAVE_LIBMYSQLCLIENT
#ifdef TEST_MYSQL
    puts("running mysql tests");
    testdb = &mysql_testdb;
    if (run_db_test(argc, argv)) {
        return 1;
    }
#endif
#else
    cout << "Mysql not supported" << endl;
#endif

#ifdef HAVE_LIBPQ
#ifdef TEST_POSTGRES
    puts("running postgres tests");
    testdb = &postgres_testdb;
    if (run_db_test(argc, argv)) {
        return 1;
    }
#endif
#else
    cout << "Mysql not supported" << endl;
#endif
    return 0;
}
