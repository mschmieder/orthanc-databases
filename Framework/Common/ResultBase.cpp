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


#include "ResultBase.h"

#include "../Common/BinaryStringValue.h"
#include "../Common/Integer64Value.h"
#include "../Common/NullValue.h"
#include "../Common/Utf8StringValue.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <cassert>
#include <memory>

namespace OrthancDatabases
{
  void ResultBase::ClearFields()
  {
    for (size_t i = 0; i < fields_.size(); i++)
    {
      if (fields_[i] != NULL)
      {
        delete fields_[i];
        fields_[i] = NULL;
      }
    }
  }


  void ResultBase::ConvertFields()
  {
    assert(expectedType_.size() == fields_.size() &&
           hasExpectedType_.size() == fields_.size());
      
    for (size_t i = 0; i < fields_.size(); i++)
    {
      if (fields_[i] == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      ValueType sourceType = fields_[i]->GetType();
      ValueType targetType = expectedType_[i];

      if (hasExpectedType_[i] &&
          sourceType != ValueType_Null &&
          sourceType != targetType)
      {
        std::auto_ptr<IValue> converted(fields_[i]->Convert(targetType));
        
        if (converted.get() == NULL)
        {
          LOG(ERROR) << "Cannot convert between data types from a database";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
        }
        else
        {
          assert(fields_[i] != NULL);
          delete fields_[i];
          fields_[i] = converted.release();
        }
      }
    }
  }


  void ResultBase::FetchFields()
  {
    ClearFields();

    if (!IsDone())
    {
      for (size_t i = 0; i < fields_.size(); i++)
      {
        fields_[i] = FetchField(i);

        if (fields_[i] == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      ConvertFields();
    }
  }
    

  void ResultBase::SetFieldsCount(size_t count)
  {
    if (!fields_.empty())
    {
      // This method can only be invoked once
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    
    fields_.resize(count);
    expectedType_.resize(count, ValueType_Null);
    hasExpectedType_.resize(count, false);
  }
    

  void ResultBase::SetExpectedType(size_t field,
                                   ValueType type)
  {
    assert(expectedType_.size() == fields_.size() &&
           hasExpectedType_.size() == fields_.size());
      
    if (field < fields_.size())
    {
      expectedType_[field] = type;
      hasExpectedType_[field] = true;

      if (!IsDone())
      {
        ConvertFields();
      }
    }
  }
  

  const IValue& ResultBase::GetField(size_t index) const
  {
    if (IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else if (index >= fields_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else if (fields_[index] == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return *fields_[index];
    }
  }
}
