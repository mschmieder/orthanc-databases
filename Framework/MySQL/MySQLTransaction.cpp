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


#include "MySQLTransaction.h"

#include "MySQLStatement.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <memory>

namespace OrthancDatabases
{
  MySQLTransaction::MySQLTransaction(MySQLDatabase& db) :
    db_(db),
    readOnly_(true),
    active_(false)
  {
    db_.Execute("START TRANSACTION", false);
    active_ = true;
  }

  
  MySQLTransaction::~MySQLTransaction()
  {
    if (active_)
    {
      LOG(WARNING) << "An active MySQL transaction was dismissed";

      try
      {
        db_.Execute("ROLLBACK", false);
      }
      catch (Orthanc::OrthancException&)
      {
      }
    }
  }

  
  void MySQLTransaction::Rollback()
  {
    if (active_)
    {
      db_.Execute("ROLLBACK", false);
      active_ = false;
      readOnly_ = true;
    }
    else
    {
      LOG(ERROR) << "MySQL: This transaction is already finished";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  
  void MySQLTransaction::Commit()
  {
    if (active_)
    {
      db_.Execute("COMMIT", false);
      active_ = false;
      readOnly_ = true;
    }
    else
    {
      LOG(ERROR) << "MySQL: This transaction is already finished";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  IResult* MySQLTransaction::Execute(IPrecompiledStatement& statement,
                                     const Dictionary& parameters)
  {
    std::auto_ptr<IResult> result(dynamic_cast<MySQLStatement&>(statement).Execute(*this, parameters));

    if (!statement.IsReadOnly())
    {
      readOnly_ = false;
    }
    
    return result.release();
  }


  void MySQLTransaction::ExecuteWithoutResult(IPrecompiledStatement& statement,
                                              const Dictionary& parameters)
  {
    dynamic_cast<MySQLStatement&>(statement).ExecuteWithoutResult(*this, parameters);

    if (!statement.IsReadOnly())
    {
      readOnly_ = false;
    }
  }
}
