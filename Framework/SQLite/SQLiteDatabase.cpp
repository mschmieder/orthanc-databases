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


#include "SQLiteDatabase.h"

#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include "../Common/ImplicitTransaction.h"

#include <Core/OrthancException.h>

namespace OrthancDatabases
{
  void SQLiteDatabase::Execute(const std::string& sql)
  {
    if (!connection_.Execute(sql))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
  }
    

  IPrecompiledStatement* SQLiteDatabase::Compile(const Query& query)
  {
    return new SQLiteStatement(*this, query);
  }

  
  namespace
  {
    class SQLiteImplicitTransaction : public ImplicitTransaction
    {
    private:
      SQLiteDatabase&  db_;

    protected:
      virtual IResult* ExecuteInternal(IPrecompiledStatement& statement,
                                       const Dictionary& parameters)
      {
        return dynamic_cast<SQLiteStatement&>(statement).Execute(*this, parameters);
      }

      virtual void ExecuteWithoutResultInternal(IPrecompiledStatement& statement,
                                                const Dictionary& parameters)
      {
        dynamic_cast<SQLiteStatement&>(statement).ExecuteWithoutResult(*this, parameters);
      }
      
    public:
      SQLiteImplicitTransaction(SQLiteDatabase&  db) :
        db_(db)
      {
      }
    };
  }
  
  ITransaction* SQLiteDatabase::CreateTransaction(bool isImplicit)
  {
    if (isImplicit)
    {
      return new SQLiteImplicitTransaction(*this);
    }
    else
    {
      return new SQLiteTransaction(*this);
    }
  }
}
