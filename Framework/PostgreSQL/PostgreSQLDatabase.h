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

#include "PostgreSQLParameters.h"
#include "../Common/IDatabase.h"

namespace OrthancDatabases
{
  class PostgreSQLDatabase : public IDatabase
  {
  private:
    friend class PostgreSQLStatement;
    friend class PostgreSQLLargeObject;

    PostgreSQLParameters  parameters_;
    void*                 pg_;   /* Object of type "PGconn*" */

    void ThrowException(bool log);

    void Close();

  public:
    PostgreSQLDatabase(const PostgreSQLParameters& parameters) :
    parameters_(parameters),
    pg_(NULL)
    {
    }

    ~PostgreSQLDatabase()
    {
      Close();
    }

    void Open();

    void AdvisoryLock(int32_t lock);

    void Execute(const std::string& sql);

    bool DoesTableExist(const char* name);

    void ClearAll();   // Only for unit tests!

    virtual Dialect GetDialect() const
    {
      return Dialect_PostgreSQL;
    }

    virtual IPrecompiledStatement* Compile(const Query& query);

    virtual ITransaction* CreateTransaction(bool isImplicit);
  };
}
