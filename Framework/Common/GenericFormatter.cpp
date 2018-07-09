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


#include "GenericFormatter.h"

#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

namespace OrthancDatabases
{
  void GenericFormatter::Format(std::string& target,
                                const std::string& source,
                                ValueType type)
  {
    if (source.empty())
    {
      // This is the default parameter for INSERT
      switch (dialect_)
      {
        case Dialect_PostgreSQL:
          target = "DEFAULT";
          break;

        case Dialect_MySQL:
        case Dialect_SQLite:
          target = "NULL";
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
    else
    {
      switch (dialect_)
      {
        case Dialect_PostgreSQL:
          target = "$" + boost::lexical_cast<std::string>(parametersName_.size() + 1);
          break;

        case Dialect_MySQL:
        case Dialect_SQLite:
          target = "?";
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      parametersName_.push_back(source);
      parametersType_.push_back(type);
    }
  }


  const std::string& GenericFormatter::GetParameterName(size_t index) const
  {
    if (index >= parametersName_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return parametersName_[index];
    }
  }

    
  ValueType GenericFormatter::GetParameterType(size_t index) const
  {
    if (index >= parametersType_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return parametersType_[index];
    }
  }
}
