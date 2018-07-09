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


#include "PostgreSQLDatabase.h"

#include "PostgreSQLResult.h"
#include "PostgreSQLStatement.h"
#include "PostgreSQLTransaction.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

// PostgreSQL includes
#include <libpq-fe.h>
#include <c.h>
#include <catalog/pg_type.h>


namespace OrthancDatabases
{
  void PostgreSQLDatabase::ThrowException(bool log)
  {
    if (log)
    {
      LOG(ERROR) << "PostgreSQL error: "
                 << PQerrorMessage(reinterpret_cast<PGconn*>(pg_));
    }
    
    if (PQstatus(reinterpret_cast<PGconn*>(pg_)) == CONNECTION_OK)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_DatabaseUnavailable);
    }
  }


  void PostgreSQLDatabase::Close()
  {
    if (pg_ != NULL)
    {
      LOG(INFO) << "Closing connection to PostgreSQL";
      PQfinish(reinterpret_cast<PGconn*>(pg_));
      pg_ = NULL;
    }
  }

  
  void PostgreSQLDatabase::Open()
  {
    if (pg_ != NULL)
    {
      // Already connected
      return;
    }

    std::string s;
    parameters_.Format(s);

    pg_ = PQconnectdb(s.c_str());

    if (pg_ == NULL ||
        PQstatus(reinterpret_cast<PGconn*>(pg_)) != CONNECTION_OK)
    {
      std::string message;

      if (pg_)
      {
        message = PQerrorMessage(reinterpret_cast<PGconn*>(pg_));
        PQfinish(reinterpret_cast<PGconn*>(pg_));
        pg_ = NULL;
      }

      LOG(ERROR) << "PostgreSQL error: " << message;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_DatabaseUnavailable);
    }
  }


  void PostgreSQLDatabase::AdvisoryLock(int32_t lock)
  {
    PostgreSQLTransaction transaction(*this);

    PostgreSQLStatement s(*this, "select pg_try_advisory_lock(" + 
                          boost::lexical_cast<std::string>(lock) + ");");

    PostgreSQLResult result(s);
    if (result.IsDone() ||
        !result.GetBoolean(0))
    {
      LOG(ERROR) << "The PostgreSQL database is locked by another instance of Orthanc";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }

    transaction.Commit();
  }


  void PostgreSQLDatabase::Execute(const std::string& sql)
  {
    LOG(TRACE) << "PostgreSQL: " << sql;
    Open();

    PGresult* result = PQexec(reinterpret_cast<PGconn*>(pg_), sql.c_str());
    if (result == NULL)
    {
      ThrowException(true);
    }

    bool ok = (PQresultStatus(result) == PGRES_COMMAND_OK ||
               PQresultStatus(result) == PGRES_TUPLES_OK);

    if (ok)
    {
      PQclear(result);
    }
    else
    {
      std::string message = PQresultErrorMessage(result);
      PQclear(result);

      LOG(ERROR) << "PostgreSQL error: " << message;
      ThrowException(false);
    }
  }


  bool PostgreSQLDatabase::DoesTableExist(const char* name)
  {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(), tolower);

    // http://stackoverflow.com/a/24089729/881731

    PostgreSQLStatement statement(*this, 
                                  "SELECT 1 FROM pg_catalog.pg_class c "
                                  "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
                                  "WHERE n.nspname = 'public' AND c.relkind='r' "
                                  "AND c.relname=$1", true);

    statement.DeclareInputString(0);
    statement.BindString(0, lower);

    PostgreSQLResult result(statement);
    return !result.IsDone();
  }


  void PostgreSQLDatabase::ClearAll()
  {
    PostgreSQLTransaction transaction(*this);
    
    // Remove all the large objects
    Execute("SELECT lo_unlink(loid) FROM (SELECT DISTINCT loid FROM pg_catalog.pg_largeobject) as loids;");

    // http://stackoverflow.com/a/21247009/881731
    Execute("DROP SCHEMA public CASCADE;");
    Execute("CREATE SCHEMA public;");
    Execute("GRANT ALL ON SCHEMA public TO postgres;");
    Execute("GRANT ALL ON SCHEMA public TO public;");
    Execute("COMMENT ON SCHEMA public IS 'standard public schema';");

    transaction.Commit();
  }


  IPrecompiledStatement* PostgreSQLDatabase::Compile(const Query& query)
  {
    return new PostgreSQLStatement(*this, query);
  }


  ITransaction* PostgreSQLDatabase::CreateTransaction()
  {
    return new PostgreSQLTransaction(*this);
  }
}
