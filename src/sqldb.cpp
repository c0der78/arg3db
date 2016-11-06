#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "exception.h"
#include "sqldb.h"

namespace rj
{
    namespace db
    {
        RJ_IMPLEMENT_EXCEPTION(database_exception, std::exception);

        RJ_IMPLEMENT_EXCEPTION(no_such_column_exception, database_exception);

        RJ_IMPLEMENT_EXCEPTION(record_not_found_exception, database_exception);

        RJ_IMPLEMENT_EXCEPTION(binding_error, database_exception);

        RJ_IMPLEMENT_EXCEPTION(transaction_exception, database_exception);

        RJ_IMPLEMENT_EXCEPTION(no_primary_key_exception, database_exception);

        RJ_IMPLEMENT_EXCEPTION(value_conversion_error, database_exception);

        std::shared_ptr<session> sqldb::create_session(const std::string &uristr)
        {
            db::uri uri(uristr);
            return create_session(uri);
        }
        std::shared_ptr<session> sqldb::create_session(const uri &uri)
        {
            auto factory = instance()->factories_[uri.protocol];

            if (factory == nullptr) {
                throw database_exception("unknown database " + uri.value);
            }

            return std::make_shared<session>(factory->create(uri));
        }

        std::shared_ptr<session> sqldb::open_session(const std::string &uristr)
        {
            db::uri uri(uristr);
            return open_session(uri);
        }

        std::shared_ptr<session> sqldb::open_session(const uri &uri)
        {
            auto factory = instance()->factories_[uri.protocol];

            if (factory == nullptr) {
                throw database_exception("unknown database " + uri.value);
            }

            auto value = std::make_shared<session>(factory->create(uri));

            value->open();

            return value;
        }

        void sqldb::register_session(const std::string &protocol, const std::shared_ptr<session_factory> &factory)
        {
            if (protocol.empty()) {
                throw database_exception("invalid protocol for session factory registration");
            }

            if (factory == nullptr) {
                throw database_exception("invalid factory for session factory registration");
            }

            instance()->factories_[protocol] = factory;
        }

        sqldb *sqldb::instance()
        {
            static sqldb instance_;
            return &instance_;
        }

        sqldb::sqldb()
        {
        }

        sqldb::~sqldb()
        {
        }
    }
}