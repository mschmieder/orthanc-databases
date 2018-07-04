/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#if ORTHANC_ENABLE_POSTGRESQL != 1
#  error PostgreSQL support must be enabled to use this file
#endif

#include "PostgreSQLDatabase.h"

#include <libpq-fe.h>

namespace OrthancDatabases
{  
  class PostgreSQLLargeObject : public boost::noncopyable
  {
  private:
    class Reader;

    PostgreSQLDatabase& database_;
    Oid oid_;

    void Create();

    void Write(const void* data, 
               size_t size);

  public:
    PostgreSQLLargeObject(PostgreSQLDatabase& database,
                          const void* data,
                          size_t size);

    PostgreSQLLargeObject(PostgreSQLDatabase& database,
                          const std::string& s);

    std::string GetOid() const;

    static void Read(std::string& target,
                     PostgreSQLDatabase& database,
                     const std::string& oid);

    static void Read(void*& target,
                     size_t& size,
                     PostgreSQLDatabase& database,
                     const std::string& oid);

    static void Delete(PostgreSQLDatabase& database,
                       const std::string& oid);
  };

}
