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


#include "Integer64Value.h"

#include "BinaryStringValue.h"
#include "FileValue.h"
#include "NullValue.h"
#include "Utf8StringValue.h"

#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

namespace OrthancDatabases
{
  IValue* Integer64Value::Convert(ValueType target) const
  {
    std::string s = boost::lexical_cast<std::string>(value_);
            
    switch (target)
    {
      case ValueType_Null:
        return new NullValue;

      case ValueType_BinaryString:
        return new BinaryStringValue(s);

      case ValueType_File:
        return new FileValue(s);

      case ValueType_Utf8String:
        return new Utf8StringValue(s);

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  std::string Integer64Value::Format() const
  {
    return boost::lexical_cast<std::string>(value_);
  }
}
