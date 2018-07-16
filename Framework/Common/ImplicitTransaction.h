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

#include "ITransaction.h"

namespace OrthancDatabases
{
  class ImplicitTransaction : public ITransaction
  {
  private:
    enum State
    {
      State_Ready,
      State_Executed,
      State_Committed
    };

    State   state_;
    bool    readOnly_;

    void CheckStateForExecution();
    
  protected:
    virtual IResult* ExecuteInternal(IPrecompiledStatement& statement,
                                     const Dictionary& parameters) = 0;

    virtual void ExecuteWithoutResultInternal(IPrecompiledStatement& statement,
                                              const Dictionary& parameters) = 0;
    
  public:
    ImplicitTransaction();

    virtual ~ImplicitTransaction();
    
    virtual bool IsImplicit() const
    {
      return true;
    }
    
    virtual bool IsReadOnly() const
    {
      return readOnly_;
    }

    virtual void Rollback();
    
    virtual void Commit();
    
    virtual IResult* Execute(IPrecompiledStatement& statement,
                             const Dictionary& parameters);

    virtual void ExecuteWithoutResult(IPrecompiledStatement& statement,
                                      const Dictionary& parameters);

    static void SetErrorOnDoubleExecution(bool isError);

    static bool IsErrorOnDoubleExecution();
  };
}
