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


#include "MySQLStatement.h"

#include "../Common/BinaryStringValue.h"
#include "../Common/FileValue.h"
#include "../Common/Integer64Value.h"
#include "../Common/NullValue.h"
#include "../Common/Utf8StringValue.h"
#include "MySQLResult.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <list>
#include <memory>

namespace OrthancDatabases
{
  class MySQLStatement::ResultField : public boost::noncopyable
  {
  private:     
    IValue* CreateIntegerValue(MYSQL_BIND& bind) const
    {
      if (length_ != buffer_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
            
      switch (mysqlType_)
      {
        case MYSQL_TYPE_TINY:
          if (bind.is_unsigned)
          {
            return new Integer64Value(*reinterpret_cast<const uint8_t*>(&buffer_[0]));
          }
          else
          {
            return new Integer64Value(*reinterpret_cast<const int8_t*>(&buffer_[0]));
          }
                
        case MYSQL_TYPE_SHORT:
          if (bind.is_unsigned)
          {
            return new Integer64Value(*reinterpret_cast<const uint16_t*>(&buffer_[0]));
          }
          else
          {
            return new Integer64Value(*reinterpret_cast<const int16_t*>(&buffer_[0]));
          }

          break;

        case MYSQL_TYPE_LONG:
          if (bind.is_unsigned)
          {
            return new Integer64Value(*reinterpret_cast<const uint32_t*>(&buffer_[0]));
          }
          else
          {
            return new Integer64Value(*reinterpret_cast<const int32_t*>(&buffer_[0]));
          }

          break;
            
        case MYSQL_TYPE_LONGLONG:
          if (bind.is_unsigned)
          {
            uint64_t value = *reinterpret_cast<const uint64_t*>(&buffer_[0]);
            if (static_cast<uint64_t>(static_cast<int64_t>(value)) != value)
            {
              LOG(WARNING) << "Overflow in a 64 bit integer";
            }

            return new Integer64Value(static_cast<int64_t>(value));
          }
          else
          {
            return new Integer64Value(*reinterpret_cast<const int64_t*>(&buffer_[0]));
          }

          break;
              
        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);              
      }
    }
        

    enum enum_field_types   mysqlType_;
    ValueType               orthancType_;
    std::string             buffer_;
    my_bool                 isNull_;
    my_bool                 isError_;
    unsigned long           length_;


  public:
    ResultField(const MYSQL_FIELD& field) :
      mysqlType_(field.type)
    {
      // https://dev.mysql.com/doc/refman/8.0/en/c-api-data-structures.html
      // https://dev.mysql.com/doc/refman/8.0/en/mysql-stmt-fetch.html => size of "buffer_"
      switch (field.type)
      {
        case MYSQL_TYPE_TINY:
          orthancType_ = ValueType_Integer64;
          buffer_.resize(1);
          break;
              
        case MYSQL_TYPE_SHORT:
          orthancType_ = ValueType_Integer64;
          buffer_.resize(2);
          break;

        case MYSQL_TYPE_LONG:
          orthancType_ = ValueType_Integer64;
          buffer_.resize(4);
          break;
            
        case MYSQL_TYPE_LONGLONG:
          orthancType_ = ValueType_Integer64;
          buffer_.resize(8);
          break;

        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_BLOB:
          // https://medium.com/@adamhooper/in-mysql-never-use-utf8-use-utf8mb4-11761243e434
          switch (field.charsetnr)
          {
            case 45:   // utf8mb4_general_ci
            case 46:   // utf8mb4_bin
            case 224:  // utf8mb4_unicode_ci  => RECOMMENDED collation
              // https://stackoverflow.com/questions/766809/whats-the-difference-between-utf8-general-ci-and-utf8-unicode-ci
              orthancType_ = ValueType_Utf8String;
              break;

            case 63:
              orthancType_ = ValueType_BinaryString;
              break;

            default:
              LOG(ERROR) << "Unsupported MySQL charset: " << field.charsetnr;
              throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);                
          }

          if (field.max_length > 0)
          {
            buffer_.resize(field.max_length);
          }

          break;
          
        default:
          LOG(ERROR) << "MYSQL_TYPE not implemented: " << field.type;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
        
    enum enum_field_types GetMysqlType() const
    {
      return mysqlType_;
    }

    ValueType  GetOrthancType() const
    {
      return orthancType_;
    }

    void PrepareBind(MYSQL_BIND& bind)
    {
      memset(&bind, 0, sizeof(bind));

      length_ = 0;

      bind.buffer_length = buffer_.size();
      bind.buffer_type = mysqlType_;
      bind.is_null = &isNull_;
      bind.length = &length_;

      if (buffer_.empty())
      {
        // Only fetches the actual size of the field (*):
        // mysql_stmt_fetch_column() must be invoked afterward
        bind.buffer = 0;
        isError_ = false;
      }
      else
      {
        bind.buffer = &buffer_[0];
        bind.error = &isError_;
      }
    }


    IValue* FetchValue(MySQLDatabase& database,
                       MYSQL_STMT& statement,
                       MYSQL_BIND& bind,
                       unsigned int column) const
    {
      if (isError_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }
      else if (isNull_)
      {
        return new NullValue;
      }
      else if (orthancType_ == ValueType_Integer64)
      {
        return CreateIntegerValue(bind);
      }
      else if (orthancType_ == ValueType_Utf8String ||
               orthancType_ == ValueType_BinaryString)
      {
        std::string tmp;
        tmp.resize(length_);

        if (!tmp.empty())
        {
          if (buffer_.empty())
          {
            bind.buffer = &tmp[0];
            bind.buffer_length = tmp.size();

            database.CheckErrorCode(mysql_stmt_fetch_column(&statement, &bind, column, 0));
          }
          else if (tmp.size() <= buffer_.size())
          {
            memcpy(&tmp[0], &buffer_[0], length_);
          }
          else
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }
        }

        if (orthancType_ == ValueType_Utf8String)
        {
          return new Utf8StringValue(tmp);
        }
        else
        {
          return new BinaryStringValue(tmp);
        }
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }          
    }
  };


  class MySQLStatement::ResultMetadata : public boost::noncopyable
  {
  private:
    MYSQL_RES*              metadata_;
      
  public:
    ResultMetadata(MySQLDatabase& db,
                   MySQLStatement& statement) :
      metadata_(NULL)
    {
      metadata_ = mysql_stmt_result_metadata(statement.GetObject());
    }

    ~ResultMetadata()
    {
      if (metadata_ != NULL)
      {
        mysql_free_result(metadata_);
      }
    }

    bool HasFields() const
    {
      return metadata_ != NULL;
    }

    size_t GetFieldsCount()
    {
      if (HasFields())
      {
        return mysql_num_fields(metadata_);
      }
      else
      {
        return 0;
      }
    }

    MYSQL_RES* GetObject()
    {
      return metadata_;
    }
  };
    

  void MySQLStatement::Close()
  {
    for (size_t i = 0; i < result_.size(); i++)
    {
      if (result_[i] != NULL)
      {
        delete result_[i];
      }
    }
      
    if (statement_ != NULL)
    {
      mysql_stmt_close(statement_);
      statement_ = NULL;
    }
  }


  MySQLStatement::MySQLStatement(MySQLDatabase& db,
                                 const Query& query) :
    db_(db),
    readOnly_(query.IsReadOnly()),
    statement_(NULL),
    formatter_(Dialect_MySQL)
  {
    std::string sql;
    query.Format(sql, formatter_);

    statement_ = mysql_stmt_init(db.GetObject());
    if (statement_ == NULL)
    {
      db.LogError();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }

    LOG(INFO) << "Preparing MySQL statement: " << sql;

    db_.CheckErrorCode(mysql_stmt_prepare(statement_, sql.c_str(), sql.size()));

    if (mysql_stmt_param_count(statement_) != formatter_.GetParametersCount())
    {
      Close();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    try
    {
      ResultMetadata result(db, *this);

      if (result.HasFields())
      {
        MYSQL_FIELD *field;
        while ((field = mysql_fetch_field(result.GetObject())))
        {
          result_.push_back(new ResultField(*field));
        }
      }

      if (result_.size() != result.GetFieldsCount())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
    catch (Orthanc::OrthancException&)
    {
      Close();
      throw;
    }

    if (query.IsReadOnly())
    {
      unsigned long type = (unsigned long) CURSOR_TYPE_READ_ONLY;
      mysql_stmt_attr_set(statement_, STMT_ATTR_CURSOR_TYPE, (void*) &type);
    }
  }


  MYSQL_STMT* MySQLStatement::GetObject()
  {
    if (statement_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return statement_;
    }
  }


  IValue* MySQLStatement::FetchResultField(size_t i)
  {
    if (i >= result_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(result_[i] != NULL);
      return result_[i]->FetchValue(db_, *statement_, outputs_[i], i);
    }
  }


  IResult* MySQLStatement::Execute(MySQLTransaction& transaction,
                                   const Dictionary& parameters)
  {
    std::list<int>            intParameters;
    std::list<long long int>  int64Parameters;

    std::vector<MYSQL_BIND>  inputs(formatter_.GetParametersCount());

    for (size_t i = 0; i < inputs.size(); i++)
    {
      memset(&inputs[i], 0, sizeof(MYSQL_BIND));

      const std::string& name = formatter_.GetParameterName(i);
      if (!parameters.HasKey(name))
      {
        LOG(ERROR) << "Missing required parameter in a SQL query: " << name;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
      }

      ValueType type = formatter_.GetParameterType(i);

      const IValue& value = parameters.GetValue(name);
      if (value.GetType() != type)
      {
        LOG(ERROR) << "Bad type of argument provided to a SQL query: " << name;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
      }

      // https://dev.mysql.com/doc/refman/8.0/en/c-api-prepared-statement-type-codes.html
      switch (type)
      {
        case ValueType_Integer64:
        {
          int64Parameters.push_back(dynamic_cast<const Integer64Value&>(value).GetValue());
          inputs[i].buffer = &int64Parameters.back();
          inputs[i].buffer_type = MYSQL_TYPE_LONGLONG;
          break;
        }

        case ValueType_Utf8String:
        {
          const std::string& utf8 = dynamic_cast<const Utf8StringValue&>(value).GetContent();
          inputs[i].buffer = const_cast<char*>(utf8.c_str());
          inputs[i].buffer_length = utf8.size();
          inputs[i].buffer_type = MYSQL_TYPE_STRING;
          break;
        }

        case ValueType_BinaryString:
        {
          const std::string& content = dynamic_cast<const BinaryStringValue&>(value).GetContent();
          inputs[i].buffer = const_cast<char*>(content.c_str());
          inputs[i].buffer_length = content.size();
          inputs[i].buffer_type = MYSQL_TYPE_BLOB;
          break;
        }

        case ValueType_File:
        {
          const std::string& content = dynamic_cast<const FileValue&>(value).GetContent();
          inputs[i].buffer = const_cast<char*>(content.c_str());
          inputs[i].buffer_length = content.size();
          inputs[i].buffer_type = MYSQL_TYPE_BLOB;
          break;
        }

        case ValueType_Null:
        {
          inputs[i].buffer = NULL;
          inputs[i].buffer_type = MYSQL_TYPE_NULL;
          break;
        }

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }

    if (!inputs.empty())
    {
      db_.CheckErrorCode(mysql_stmt_bind_param(statement_, &inputs[0]));
    }

    db_.CheckErrorCode(mysql_stmt_execute(statement_));

    outputs_.resize(result_.size());

    for (size_t i = 0; i < result_.size(); i++)
    {
      assert(result_[i] != NULL);
      result_[i]->PrepareBind(outputs_[i]);
    }

    if (!outputs_.empty())
    {
      db_.CheckErrorCode(mysql_stmt_bind_result(statement_, &outputs_[0]));
      db_.CheckErrorCode(mysql_stmt_store_result(statement_));
    }

    return new MySQLResult(db_, *this);
  }


  void MySQLStatement::ExecuteWithoutResult(MySQLTransaction& transaction,
                                            const Dictionary& parameters)
  {
    std::auto_ptr<IResult> dummy(Execute(transaction, parameters));
  }
}
