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


#include "Dictionary.h"

#include "BinaryStringValue.h"
#include "Integer64Value.h"
#include "NullValue.h"
#include "Utf8StringValue.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <cassert>

namespace OrthancDatabases
{
  Dictionary::~Dictionary()
  {
    for (Values::iterator it = values_.begin(); 
         it != values_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }


  bool Dictionary::HasKey(const std::string& key) const
  {
    return values_.find(key) != values_.end();
  }


  void Dictionary::Remove(const std::string& key)
  {
    Values::iterator found = values_.find(key);

    if (found != values_.end())
    {
      assert(found->second != NULL);
      delete found->second;
      values_.erase(found);
    }
  }

  
  void Dictionary::SetValue(const std::string& key,
                            IValue* value)   // Takes ownership
  {
    if (value == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    Values::iterator found = values_.find(key);

    if (found == values_.end())
    {
      values_[key] = value;
    }
    else
    {
      assert(found->second != NULL);
      delete found->second;
      found->second = value;
    }      
  }

  
  void Dictionary::SetUtf8Value(const std::string& key,
                                const std::string& utf8)
  {
    SetValue(key, new Utf8StringValue(utf8));
  }

  
  void Dictionary::SetBinaryValue(const std::string& key,
                                  const std::string& binary)
  {
    SetValue(key, new BinaryStringValue(binary));
  }

  
  void Dictionary::SetIntegerValue(const std::string& key,
                                   int64_t value)
  {
    SetValue(key, new Integer64Value(value));
  }

  
  void Dictionary::SetNullValue(const std::string& key)
  {
    SetValue(key, new NullValue);
  }

  
  const IValue& Dictionary::GetValue(const std::string& key) const
  {
    Values::const_iterator found = values_.find(key);

    if (found == values_.end())
    {
      LOG(ERROR) << "Inexistent value in a dictionary: " << key;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
    else
    {
      assert(found->second != NULL);
      return *found->second;
    }
  }
}
