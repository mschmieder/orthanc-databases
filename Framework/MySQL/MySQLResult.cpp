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


#include "MySQLResult.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <mysql/errmsg.h>
#include <mysqld_error.h>

namespace OrthancDatabases
{
  void MySQLResult::Step()
  {
    int code = mysql_stmt_fetch(statement_.GetObject());

    if (code == 1)
    {
      unsigned int error = mysql_errno(database_.GetObject());
      if (error == 0)
      {
        // This case can occur if the SQL request is not a SELECT
        done_ = true;
      }
      else if (error == CR_SERVER_GONE_ERROR ||
               error == CR_SERVER_LOST ||
               error == ER_QUERY_INTERRUPTED)
      {
        database_.LogError();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_DatabaseUnavailable);
      }
      else
      {
        database_.LogError();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }
    }
    else
    {
      done_ = (code != 0 &&
               code != MYSQL_DATA_TRUNCATED);  // Occurs if mysql_stmt_fetch_column() must be called

      FetchFields();
    }
  }


  IValue* MySQLResult::FetchField(size_t index)
  {
    return statement_.FetchResultField(index);
  }
  
  
  MySQLResult::MySQLResult(MySQLDatabase& db,
                           MySQLStatement& statement) :
    database_(db),
    statement_(statement)
  {
    // !!! https://dev.mysql.com/doc/refman/5.7/en/mysql-stmt-fetch.html
    // https://gist.github.com/hoterran/6365915
    // https://github.com/hholzgra/connector-c-examples/blob/master/mysql_stmt_bind_result.c

    SetFieldsCount(statement_.GetResultFieldsCount());

    Step();
  }
    

  MySQLResult::~MySQLResult()
  {
    // Reset the statement for further use
    if (mysql_stmt_reset(statement_.GetObject()) != 0)
    {
      LOG(ERROR) << "Cannot reset the statement, expect an error";
    }
  }
    

  void MySQLResult::Next()
  {
    if (IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      Step();
    }
  }
}
