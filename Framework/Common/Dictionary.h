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

#include "IValue.h"

#include <map>

namespace OrthancDatabases
{
  class Dictionary : public boost::noncopyable
  {
  private:
    typedef std::map<std::string, IValue*>   Values;

    Values  values_;

  public:
    ~Dictionary();

    bool HasKey(const std::string& key) const;

    void Remove(const std::string& key);

    void SetValue(const std::string& key,
                  IValue* value);   // Takes ownership

    void SetUtf8Value(const std::string& key,
                      const std::string& utf8);

    void SetBinaryValue(const std::string& key,
                        const std::string& binary);

    void SetIntegerValue(const std::string& key,
                         int64_t value);

    void SetNullValue(const std::string& key);

    const IValue& GetValue(const std::string& key) const;
  };
}
