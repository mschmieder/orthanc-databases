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


#include "SQLiteStatement.h"

#include "../Common/BinaryStringValue.h"
#include "../Common/FileValue.h"
#include "../Common/Integer64Value.h"
#include "../Common/Query.h"
#include "../Common/Utf8StringValue.h"
#include "SQLiteResult.h"

#include <Core/OrthancException.h>

namespace OrthancDatabases
{
  SQLiteStatement::SQLiteStatement(SQLiteDatabase& database,
                                   const Query& query) :
    readOnly_(query.IsReadOnly()),
    formatter_(Dialect_SQLite)
  {
    std::string sql;
    query.Format(sql, formatter_);
    
    statement_.reset(new Orthanc::SQLite::Statement(database.GetObject(), sql));    
  }


  Orthanc::SQLite::Statement& SQLiteStatement::GetObject()
  {
    if (statement_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return *statement_;
    }
  }


  void SQLiteStatement::BindParameters(const Dictionary& parameters)
  {
    statement_->Reset();
    
    for (size_t i = 0; i < formatter_.GetParametersCount(); i++)
    {
      const std::string& name = formatter_.GetParameterName(i);
      
      switch (formatter_.GetParameterType(i))
      {
        case ValueType_BinaryString:
        {
          const BinaryStringValue& blob =
            dynamic_cast<const BinaryStringValue&>(parameters.GetValue(name));
          statement_->BindBlob(i, blob.GetBuffer(), blob.GetSize());
          break;
        }

        case ValueType_File:
        {
          const FileValue& blob =
            dynamic_cast<const FileValue&>(parameters.GetValue(name));
          statement_->BindBlob(i, blob.GetBuffer(), blob.GetSize());
          break;
        }

        case ValueType_Integer64:
          statement_->BindInt64(i, dynamic_cast<const Integer64Value&>
                                (parameters.GetValue(name)).GetValue());
          break;

        case ValueType_Null:
          statement_->BindNull(i);
          break;

        case ValueType_Utf8String:
          statement_->BindString(i, dynamic_cast<const Utf8StringValue&>
                                 (parameters.GetValue(name)).GetContent());
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }    
  }

  
  IResult* SQLiteStatement::Execute(SQLiteTransaction& transaction,
                                    const Dictionary& parameters)
  {
    BindParameters(parameters);
    return new SQLiteResult(*this);
  }


  void SQLiteStatement::ExecuteWithoutResult(SQLiteTransaction& transaction,
                                             const Dictionary& parameters)
  {
    BindParameters(parameters);

    if (!statement_->Run())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
  }
}
