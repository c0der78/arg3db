
#include <bandit/bandit.h>
#include "../db.test.h"
#include "mysql/resultset.h"

using namespace bandit;

using namespace std;

using namespace rj::db;

shared_ptr<resultset_impl> get_mysql_resultset()
{
    auto rs = current_session->query("select * from users");

    return rs.impl();
}

shared_ptr<resultset_impl> get_mysql_stmt_resultset()
{
    select_query query(current_session, {}, "users");

    auto rs = query.execute();

    return rs.impl();
}


template <typename T>
void test_move_resultset(std::function<shared_ptr<resultset_impl>()> funk)
{
    auto f1 = funk();

    auto f2 = funk();

    T c2(std::move(*static_pointer_cast<T>(f1)));

    Assert::That(c2.is_valid(), IsTrue());

    c2 = std::move(*static_pointer_cast<T>(f2));

    Assert::That(c2.is_valid(), IsTrue());
}

template <typename T>
void test_resultset_row(std::function<shared_ptr<resultset_impl>()> funk)
{
    auto r = funk();

    Assert::That(r->next(), IsTrue());

    Assert::That(r->current_row().is_valid(), IsTrue());

    Assert::That(r->current_row().size() > 0, IsTrue());
}

go_bandit([]() {

    describe("mysql resultset", []() {
        before_each([]() {
            setup_current_session();

            user user1;

            user1.set_id(1);
            user1.set("first_name", "Bryan");
            user1.set("last_name", "Jenkins");

            user1.save();

            user user2;

            user2.set_id(3);

            user2.set("first_name", "Bob");
            user2.set("last_name", "Smith");

            user2.set("dval", 3.1456);

            user2.save();
        });

        after_each([]() { teardown_current_session(); });

        describe("is movable", []() {


            it("as statement results", []() { test_move_resultset<mysql::stmt_resultset>(get_mysql_stmt_resultset); });

            it("as results", []() { test_move_resultset<mysql::resultset>(get_mysql_resultset); });


        });

        describe("can get a row", []() {

            it("as statement results", []() {
                test_resultset_row<mysql::stmt_resultset>(get_mysql_stmt_resultset);

            });

            it("as results", []() { test_resultset_row<mysql::resultset>(get_mysql_resultset); });

        });

        it("can handle a bad query", []() {
            AssertThat(current_session->execute("select * from asdfasdfasdf"), Equals(false));

            select_query query(current_session, {}, "asdfasdfasdf");

            AssertThrows(database_exception, query.execute());
        });
    });

});
