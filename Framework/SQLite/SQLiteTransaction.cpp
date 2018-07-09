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


#include "SQLiteTransaction.h"

#include "SQLiteResult.h"
#include "SQLiteStatement.h"

#include <Core/OrthancException.h>

namespace OrthancDatabases
{
  SQLiteTransaction::SQLiteTransaction(SQLiteDatabase& database) :
    transaction_(database.GetObject()),
    readOnly_(true)
  {
    transaction_.Begin();

    if (!transaction_.IsOpen())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }

  IResult* SQLiteTransaction::Execute(IPrecompiledStatement& statement,
                                      const Dictionary& parameters)
  {
    std::auto_ptr<IResult> result(dynamic_cast<SQLiteStatement&>(statement).Execute(*this, parameters));

    if (!statement.IsReadOnly())
    {
      readOnly_ = false;
    }
    
    return result.release();
  }

  void SQLiteTransaction::ExecuteWithoutResult(IPrecompiledStatement& statement,
                                               const Dictionary& parameters)
  {
    dynamic_cast<SQLiteStatement&>(statement).ExecuteWithoutResult(*this, parameters);

    if (!statement.IsReadOnly())
    {
      readOnly_ = false;
    }
  }
}
