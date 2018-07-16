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


#include "Query.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/regex.hpp>

namespace OrthancDatabases
{
  class Query::Token : public boost::noncopyable
  {
  private:
    bool         isParameter_;
    std::string  content_;

  public:
    Token(bool isParameter,
          const std::string& content) :
      isParameter_(isParameter),
      content_(content)
    {
    }

    bool IsParameter() const
    {
      return isParameter_;
    }

    const std::string& GetContent() const
    {
      return content_;
    }
  };


  void Query::Setup(const std::string& sql)
  {
    boost::regex regex("\\$\\{(.*?)\\}");

    std::string::const_iterator last = sql.begin();
    boost::sregex_token_iterator it(sql.begin(), sql.end(), regex, 0);
    boost::sregex_token_iterator end;

    while (it != end)
    {
      if (last != it->first)
      {
        tokens_.push_back(new Token(false, std::string(last, it->first)));
      }

      std::string parameter = *it;
      assert(parameter.size() >= 3);
      parameter = parameter.substr(2, parameter.size() - 3);

      tokens_.push_back(new Token(true, parameter));
      parameters_[parameter] = ValueType_Null;

      last = it->second;

      ++it;
    }

    if (last != sql.end())
    {
      tokens_.push_back(new Token(false, std::string(last, sql.end())));
    }
  }


  Query::Query(const std::string& sql) :
    readOnly_(false)
  {
    Setup(sql);
  }


  Query::Query(const std::string& sql,
               bool readOnly) :
    readOnly_(readOnly)
  {
    Setup(sql);
  }

  
  Query::~Query()
  {
    for (size_t i = 0; i < tokens_.size(); i++)
    {
      assert(tokens_[i] != NULL);
      delete tokens_[i];
    }
  }


  bool Query::HasParameter(const std::string& parameter) const
  {
    return parameters_.find(parameter) != parameters_.end();
  }

  
  ValueType Query::GetType(const std::string& parameter) const
  {
    Parameters::const_iterator found = parameters_.find(parameter);

    if (found == parameters_.end())
    {
      LOG(ERROR) << "Inexistent parameter in a SQL query: " << parameter;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
    else
    {
      return found->second;
    }
  }


  void Query::SetType(const std::string& parameter,
                      ValueType type)
  {
    Parameters::iterator found = parameters_.find(parameter);

    if (found == parameters_.end())
    {
      LOG(ERROR) << "Ignoring inexistent parameter in a SQL query: " << parameter;
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      found->second = type;
    }
  }

  
  void Query::Format(std::string& result,
                     IParameterFormatter& formatter) const
  {
    result.clear();

    for (size_t i = 0; i < tokens_.size(); i++)
    {
      assert(tokens_[i] != NULL);

      const std::string& content = tokens_[i]->GetContent();

      if (tokens_[i]->IsParameter())
      {
        std::string parameter;
        formatter.Format(parameter, content, GetType(content));
        result += parameter;
      }
      else
      {
        result += content;
      }
    }
  }
}
