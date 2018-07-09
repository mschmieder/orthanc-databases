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


#include "PostgreSQLParameters.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>


namespace OrthancDatabases
{
  void PostgreSQLParameters::Reset()
  {
    host_ = "localhost";
    port_ = 5432;
    username_ = "";
    password_ = "";
    database_.clear();
    uri_.clear();
    lock_ = true;
  }


  PostgreSQLParameters::PostgreSQLParameters()
  {
    Reset();
  }


  PostgreSQLParameters::PostgreSQLParameters(const OrthancPlugins::OrthancConfiguration& configuration)
  {
    Reset();

    std::string s;

    if (configuration.LookupStringValue(s, "ConnectionUri"))
    {
      SetConnectionUri(s);
    }
    else
    {
      if (configuration.LookupStringValue(s, "Host"))
      {
        SetHost(s);
      }

      unsigned int port;
      if (configuration.LookupUnsignedIntegerValue(port, "Port"))
      {
        SetPortNumber(port);
      }

      if (configuration.LookupStringValue(s, "Database"))
      {
        SetDatabase(s);
      }

      if (configuration.LookupStringValue(s, "Username"))
      {
        SetUsername(s);
      }

      if (configuration.LookupStringValue(s, "Password"))
      {
        SetPassword(s);
      }
    }

    lock_ = configuration.GetBooleanValue("Lock", true);  // Use locking by default
  }


  void PostgreSQLParameters::SetConnectionUri(const std::string& uri)
  {
    uri_ = uri;
  }


  std::string PostgreSQLParameters::GetConnectionUri() const
  {
    if (uri_.empty())
    {
      std::string actualUri = "postgresql://";

      if (!username_.empty())
      {
        actualUri += username_;

        if (!password_.empty())
        {
          actualUri += ":" + password_;
        }

        actualUri += "@" + host_;
      }
      else
      {
        actualUri += host_;
      }
      
      if (port_ > 0)
      {
        actualUri += ":" + boost::lexical_cast<std::string>(port_);
      }

      actualUri += "/" + database_;

      return actualUri;
    }
    else
    {
      return uri_;
    }
  }


  void PostgreSQLParameters::SetHost(const std::string& host)
  {
    uri_.clear();
    host_ = host;
  }

  void PostgreSQLParameters::SetPortNumber(unsigned int port)
  {
    if (port <= 0 ||
        port >= 65535)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    uri_.clear();
    port_ = port;
  }

  void PostgreSQLParameters::SetUsername(const std::string& username)
  {
    uri_.clear();
    username_ = username;
  }

  void PostgreSQLParameters::SetPassword(const std::string& password)
  {
    uri_.clear();
    password_ = password;
  }

  void PostgreSQLParameters::SetDatabase(const std::string& database)
  {
    uri_.clear();
    database_ = database;
  }

  void PostgreSQLParameters::Format(std::string& target) const
  {
    if (uri_.empty())
    {
      target = std::string("sslmode=disable") +  // TODO WHY SSL DOES NOT WORK? ("SSL error: wrong version number")
        " user=" + username_ + 
        " host=" + host_ + 
        " port=" + boost::lexical_cast<std::string>(port_);

      if (!password_.empty())
      {
        target += " password=" + password_;
      }

      if (database_.size() > 0)
      {
        target += " dbname=" + database_;
      }
    }
    else
    {
      target = uri_;
    }
  }
}
