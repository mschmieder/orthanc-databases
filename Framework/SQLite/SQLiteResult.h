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

#if ORTHANC_ENABLE_SQLITE != 1
#  error SQLite support must be enabled to use this file
#endif

#include "SQLiteStatement.h"
#include "../Common/ResultBase.h"

namespace OrthancDatabases
{
  class SQLiteResult : public ResultBase
  {
  private:
    SQLiteStatement&   statement_;
    bool               done_;

    void StepInternal();

  protected:
    virtual IValue* FetchField(size_t index);
    
  public:
    SQLiteResult(SQLiteStatement& statement);
    
    virtual bool IsDone() const
    {
      return done_;
    }

    virtual void Next();
  };
}
