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


#include "MySQLParameters.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace OrthancDatabases
{
  void MySQLParameters::Reset()
  {
    host_ = "localhost";
    username_.clear();
    password_.clear();
    database_.clear();
    port_ = 3306;

#if defined(_WIN32)
    unixSocket_.clear();
#else
    unixSocket_ = "/var/run/mysqld/mysqld.sock";
#endif
    
    lock_ = true;
  }

  
  MySQLParameters::MySQLParameters()
  {
    Reset();
  }


  MySQLParameters::MySQLParameters(const OrthancPlugins::OrthancConfiguration& configuration)
  {
    Reset();

    std::string s;
    if (configuration.LookupStringValue(s, "Host"))
    {
      SetHost(s);
    }

    if (configuration.LookupStringValue(s, "Username"))
    {
      SetUsername(s);
    }

    if (configuration.LookupStringValue(s, "Password"))
    {
      SetPassword(s);
    }

    if (configuration.LookupStringValue(s, "Database"))
    {
      SetDatabase(s);
    }

    unsigned int port;
    if (configuration.LookupUnsignedIntegerValue(port, "Port"))
    {
      SetPort(port);
    }

    if (configuration.LookupStringValue(s, "UnixSocket"))
    {
      SetUnixSocket(s);
    }

    lock_ = configuration.GetBooleanValue("Lock", true);  // Use locking by default
  }


  void MySQLParameters::SetHost(const std::string& host)
  {
    host_ = host;
  }

  
  void MySQLParameters::SetUsername(const std::string& username)
  {
    username_ = username;
  }

  
  void MySQLParameters::SetPassword(const std::string& password)
  {
    password_ = password;
  }

  
  void MySQLParameters::SetDatabase(const std::string& database)
  {
    if (database.empty())
    {
      LOG(ERROR) << "MySQL: Empty database name";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    for (size_t i = 0; i < database.length(); i++)
    {
      if (!isalnum(database [i]))
      {
        LOG(ERROR) << "MySQL: Only alphanumeric characters are allowed in a "
                   << "database name: \"" << database << "\"";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);          
      }
    }
    
    database_ = database;
  }

  
  void MySQLParameters::SetPort(unsigned int port)
  {
    if (port >= 65535)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      port_ = port;
    }
  }

  
  void MySQLParameters::SetUnixSocket(const std::string& socket)
  {
#if defined(_WIN32)
    if (!socket.empty())
    {
      LOG(WARNING) << "MySQL: Setting an UNIX socket on Windows has no effect";
    }
#endif
    
    unixSocket_ = socket;
  }

  
  void MySQLParameters::Format(Json::Value& target) const
  {
    target = Json::objectValue;
    target["Host"] = host_;
    target["Username"] = username_;
    target["Password"] = password_;
    target["Database"] = database_;
    target["Port"] = port_;
    target["UnixSocket"] = unixSocket_;
    target["Lock"] = lock_;
  }
}
