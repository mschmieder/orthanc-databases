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


#include "MySQLDatabase.h"

#include "MySQLResult.h"
#include "MySQLStatement.h"
#include "MySQLTransaction.h"
#include "../Common/Integer64Value.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <errmsg.h>
#include <mysqld_error.h>

#include <memory>

namespace OrthancDatabases
{
  void MySQLDatabase::Close()
  {
    if (mysql_ != NULL)
    {
      LOG(INFO) << "Closing connection to MySQL database";
      mysql_close(mysql_);
      mysql_ = NULL;
    }
  }


  void MySQLDatabase::CheckErrorCode(int code)
  {
    if (code == 0)
    {
      return;
    }
    else
    {
      LogError();
      
      unsigned int error = mysql_errno(mysql_);
      if (error == CR_SERVER_GONE_ERROR ||
          error == CR_SERVER_LOST ||
          error == ER_QUERY_INTERRUPTED)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_DatabaseUnavailable);
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }
    }
  }


  MySQLDatabase::MySQLDatabase(const MySQLParameters& parameters) :
    parameters_(parameters),
    mysql_(NULL)
  {
  }


  void MySQLDatabase::LogError()
  {
    if (mysql_ != NULL)
    {
      LOG(ERROR) << "MySQL error (" << mysql_errno(mysql_)
                 << "," << mysql_sqlstate(mysql_)
                 << "): " << mysql_error(mysql_);
    }
  }

  
  MYSQL* MySQLDatabase::GetObject()
  {
    if (mysql_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return mysql_;
    }
  }

  
  void MySQLDatabase::Open()
  {
    if (mysql_ != NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    mysql_ = mysql_init(NULL);
    if (mysql_ == NULL)
    {
      LOG(ERROR) << "Cannot initialize the MySQL connector";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
      
    const char* db = (parameters_.GetDatabase().empty() ? NULL :
                      parameters_.GetDatabase().c_str());

    const char* socket = (parameters_.GetUnixSocket().empty() ? NULL :
                          parameters_.GetUnixSocket().c_str());

    if (mysql_real_connect(mysql_,
                           parameters_.GetHost().c_str(),
                           parameters_.GetUsername().c_str(),
                           parameters_.GetPassword().c_str(), db,
                           parameters_.GetPort(), socket, 0) == 0)
    {
      LogError();
      Close();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_DatabaseUnavailable);
    }
    else
    {
      LOG(INFO) << "Successful connection to MySQL database";
    }

    if (mysql_set_character_set(mysql_, "utf8mb4") != 0)
    {
      LOG(ERROR) << "Cannot set the character set to UTF8";
      Close();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
    }
  }


  namespace
  {
    class ResultWrapper : public boost::noncopyable
    {
    private:
      MYSQL_RES *result_;

    public:
      ResultWrapper(MySQLDatabase& mysql,
                    const std::string& sql) :
        result_(NULL)
      {
        if (mysql_real_query(mysql.GetObject(), sql.c_str(), sql.size()))
        {
          mysql.LogError();
          throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
        }

        result_ = mysql_use_result(mysql.GetObject());
        if (result_ == NULL)
        {
          mysql.LogError();
          throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
        }
      }

      ~ResultWrapper()
      {
        if (result_ != NULL)
        {
          mysql_free_result(result_);
          result_ = NULL;
        }
      }

      MYSQL_RES *GetObject()
      {
        return result_;
      }
    };
  }


  bool MySQLDatabase::LookupGlobalStringVariable(std::string& value,
                                                 const std::string& variable)
  {
    ResultWrapper result(*this, "SELECT @@global." + variable);

    MYSQL_ROW row = mysql_fetch_row(result.GetObject());
    if (mysql_errno(mysql_) == 0 &&
        row &&
        row[0])
    {
      value = std::string(row[0]);
      return true;
    }
    else
    {
      return false;
    }
  }

  
  bool MySQLDatabase::LookupGlobalIntegerVariable(int64_t& value,
                                                  const std::string& variable)
  {
    std::string s;
    
    if (LookupGlobalStringVariable(s, variable))
    {
      try
      {
        value = boost::lexical_cast<int64_t>(s);
        return true;
      }
      catch (boost::bad_lexical_cast&)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }
    }
    else
    {
      return false;
    }
  }


  void MySQLDatabase::AdvisoryLock(int32_t lock)
  {
    try
    {
      Query query("SELECT GET_LOCK('Lock" +
                  boost::lexical_cast<std::string>(lock) + "', 0);", false);
      MySQLStatement statement(*this, query);

      MySQLTransaction t(*this);
      Dictionary args;

      std::auto_ptr<IResult> result(t.Execute(statement, args));

      if (result->IsDone() ||
          result->GetField(0).GetType() != ValueType_Integer64 ||
          dynamic_cast<const Integer64Value&>(result->GetField(0)).GetValue() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }

      t.Commit();
    }
    catch (Orthanc::OrthancException&)
    {
      LOG(ERROR) << "The MySQL database is locked by another instance of Orthanc";
      Close();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
  }
  

  bool MySQLDatabase::DoesTableExist(MySQLTransaction& transaction,
                                     const std::string& name)
  {
    if (mysql_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    for (size_t i = 0; i < name.length(); i++)
    {
      if (!isalnum(name[i]))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    Query query("SELECT COUNT(*) FROM information_schema.TABLES WHERE "
                "(TABLE_SCHEMA = ${database}) AND (TABLE_NAME = ${table})", true);
    query.SetType("database", ValueType_Utf8String);
    query.SetType("table", ValueType_Utf8String);
    
    MySQLStatement statement(*this, query);

    Dictionary args;
    args.SetUtf8Value("database", parameters_.GetDatabase());
    args.SetUtf8Value("table", name);

    std::auto_ptr<IResult> result(statement.Execute(transaction, args));
    return (!result->IsDone() &&
            result->GetFieldsCount() == 1 &&
            result->GetField(0).GetType() == ValueType_Integer64 &&
            dynamic_cast<const Integer64Value&>(result->GetField(0)).GetValue() == 1);            
  }


  void MySQLDatabase::Execute(const std::string& sql,
                              bool arobaseSeparator)
  {
    if (mysql_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    // This emulates the behavior of "CLIENT_MULTI_STATEMENTS" in
    // "mysql_real_connect()", avoiding to implement a loop over
    // "mysql_query()"
    std::vector<std::string> commands;
    Orthanc::Toolbox::TokenizeString(commands, sql, ';');

    for (size_t i = 0; i < commands.size(); i++)
    {
      std::string s = Orthanc::Toolbox::StripSpaces(commands[i]);

      if (!s.empty())
      {
        if (arobaseSeparator)
        {
          // Replace the escape character "@" by a semicolon
          std::replace(s.begin(), s.end(), '@', ';');
        }
      
        LOG(TRACE) << "MySQL: " << s;
        CheckErrorCode(mysql_query(mysql_, s.c_str()));
      }
    }
  }

  
  IPrecompiledStatement* MySQLDatabase::Compile(const Query& query)
  {
    if (mysql_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return new MySQLStatement(*this, query);
  }


  ITransaction* MySQLDatabase::CreateTransaction()
  {
    if (mysql_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return new MySQLTransaction(*this);
  }

  
  void MySQLDatabase::GlobalFinalization()
  {
    mysql_library_end();
  } 
}
