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

#include "IResult.h"

#include <vector>

namespace OrthancDatabases
{
  class ResultBase : public IResult
  {
  private:
    void ClearFields();

    void ConvertFields();

    std::vector<IValue*>   fields_;
    std::vector<ValueType> expectedType_;
    std::vector<bool>      hasExpectedType_;
    
  protected:
    virtual IValue* FetchField(size_t index) = 0;

    void FetchFields();

    void SetFieldsCount(size_t count);
    
  public:
    virtual ~ResultBase()
    {
      ClearFields();
    }

    virtual void SetExpectedType(size_t field,
                                 ValueType type);

    virtual size_t GetFieldsCount() const
    {
      return fields_.size();
    }

    virtual const IValue& GetField(size_t index) const;
  };
}
