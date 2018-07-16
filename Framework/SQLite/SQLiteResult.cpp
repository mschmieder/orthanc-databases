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


#include "SQLiteResult.h"

#include "../Common/BinaryStringValue.h"
#include "../Common/Integer64Value.h"
#include "../Common/NullValue.h"
#include "../Common/Utf8StringValue.h"

#include <Core/OrthancException.h>

namespace OrthancDatabases
{
  SQLiteResult::SQLiteResult(SQLiteStatement& statement) :
    statement_(statement)
  {
    if (statement_.GetObject().ColumnCount() < 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      SetFieldsCount(statement_.GetObject().ColumnCount());
    }
    
    done_ = !statement_.GetObject().Step();
    FetchFields();
  }


  IValue* SQLiteResult::FetchField(size_t index)
  {
    switch (statement_.GetObject().GetColumnType(index))
    {
      case Orthanc::SQLite::COLUMN_TYPE_INTEGER:
        return new Integer64Value(statement_.GetObject().ColumnInt64(index));
        
      case Orthanc::SQLite::COLUMN_TYPE_TEXT:
        return new Utf8StringValue(statement_.GetObject().ColumnString(index));
        
      case Orthanc::SQLite::COLUMN_TYPE_BLOB:
        return new BinaryStringValue(statement_.GetObject().ColumnString(index));
        
      case Orthanc::SQLite::COLUMN_TYPE_NULL:
        return new NullValue;
        
      case Orthanc::SQLite::COLUMN_TYPE_FLOAT:
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }
  
  
  void SQLiteResult::Next()
  {
    if (done_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      done_ = !statement_.GetObject().Step();
      FetchFields();
    }
  }
}
