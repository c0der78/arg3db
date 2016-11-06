#ifndef RJ_DB_MYSQL_TRANSACTION_H
#define RJ_DB_MYSQL_TRANSACTION_H

#include <mysql/mysql.h>
#include "../transaction.h"

namespace rj
{
    namespace db
    {
        namespace mysql
        {
            class transaction : public transaction_impl
            {
                friend class session;

               public:
                typedef struct {
                    rj::db::transaction::type type;
                    isolation::level isolation;
                } mode;

                transaction(const std::shared_ptr<MYSQL> &db, const transaction::mode &mode = {});
                transaction(const transaction &other);
                transaction(transaction &&other);
                virtual ~transaction();
                transaction &operator=(const transaction &other);
                transaction &operator=(transaction &&other);

                void start();
                bool is_active() const;

               private:
                std::shared_ptr<MYSQL> db_;
                transaction::mode mode_;
                bool active_;
            };
        }
    }
}

#endif
