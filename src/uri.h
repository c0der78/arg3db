#ifndef CODA_DB_URI_H
#define CODA_DB_URI_H

#include <string>

namespace coda {
  namespace db {
    typedef struct uri_type uri;

    /*! small utility to parse a uri */
    struct uri_type {
      uri_type();

      /*!
       * @param url the url to parse
       */
      uri_type(const std::string &url);

      /*!
       * decomposes a uri into its parts
       * @param url the url to parse
       */
      void parse(const std::string &url);

      operator std::string() const;

      std::string protocol, user, password, host, port, path, query, value;
    };
  } // namespace db
} // namespace coda

#endif
