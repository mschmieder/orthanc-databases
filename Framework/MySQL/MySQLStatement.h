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

#if ORTHANC_ENABLE_MYSQL != 1
#  error MySQL support must be enabled to use this file
#endif

#include "MySQLDatabase.h"
#include "MySQLTransaction.h"
#include "../Common/GenericFormatter.h"

namespace OrthancDatabases
{
  class MySQLStatement : public IPrecompiledStatement
  {
  private:
    class ResultField;
    class ResultMetadata;    

    void Close();

    MySQLDatabase&             db_;
    bool                       readOnly_;
    MYSQL_STMT                *statement_;
    GenericFormatter           formatter_;
    std::vector<ResultField*>  result_;
    std::vector<MYSQL_BIND>    outputs_;

  public:
    MySQLStatement(MySQLDatabase& db,
                   const Query& query);

    virtual ~MySQLStatement()
    {
      Close();
    }

    virtual bool IsReadOnly() const
    {
      return readOnly_;
    }

    MYSQL_STMT* GetObject();

    size_t GetResultFieldsCount() const
    {
      return result_.size();
    }

    IValue* FetchResultField(size_t i);

    IResult* Execute(MySQLTransaction& transaction,
                     const Dictionary& parameters);

    void ExecuteWithoutResult(MySQLTransaction& transaction,
                              const Dictionary& parameters);
  };
}
