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

#include "DatabasesEnumerations.h"

#include <map>
#include <vector>
#include <string>
#include <boost/noncopyable.hpp>


namespace OrthancDatabases
{
  class Query : public boost::noncopyable
  {
  public:
    class IParameterFormatter : public boost::noncopyable
    {
    public:
      virtual ~IParameterFormatter()
      {
      }

      virtual void Format(std::string& target,
                          const std::string& source,
                          ValueType type) = 0;
    };

  private:
    typedef std::map<std::string, ValueType>  Parameters;

    class Token;

    std::vector<Token*>  tokens_;
    Parameters           parameters_;
    bool                 readOnly_;

    void Setup(const std::string& sql);

  public:
    explicit Query(const std::string& sql);

    Query(const std::string& sql,
          bool isReadOnly);

    ~Query();

    bool IsReadOnly() const
    {
      return readOnly_;
    }

    void SetReadOnly(bool isReadOnly)
    {
      readOnly_ = isReadOnly;
    }

    bool HasParameter(const std::string& parameter) const;

    ValueType GetType(const std::string& parameter) const;

    void SetType(const std::string& parameter,
                 ValueType type);

    void Format(std::string& result,
                IParameterFormatter& formatter) const;
  };
}
