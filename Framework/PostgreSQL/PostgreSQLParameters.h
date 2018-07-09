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

#if ORTHANC_ENABLE_POSTGRESQL != 1
#  error PostgreSQL support must be enabled to use this file
#endif

#include <Plugins/Samples/Common/OrthancPluginCppWrapper.h>

namespace OrthancDatabases
{
  class PostgreSQLParameters
  {
  private:
    std::string  host_;
    uint16_t     port_;
    std::string  username_;
    std::string  password_;
    std::string  database_;
    std::string  uri_;
    bool         lock_;

    void Reset();

  public:
    PostgreSQLParameters();

    PostgreSQLParameters(const OrthancPlugins::OrthancConfiguration& configuration);

    void SetConnectionUri(const std::string& uri);

    std::string GetConnectionUri() const;

    void SetHost(const std::string& host);

    const std::string& GetHost() const
    {
      return host_;
    }

    void SetPortNumber(unsigned int port);

    uint16_t GetPortNumber() const
    {
      return port_;
    }

    void SetUsername(const std::string& username);

    const std::string& GetUsername() const
    {
      return username_;
    }

    void SetPassword(const std::string& password);

    const std::string& GetPassword() const
    {
      return password_;
    }

    void SetDatabase(const std::string& database);

    void ResetDatabase()
    {
      SetDatabase("");
    }

    const std::string& GetDatabase() const
    {
      return database_;
    }

    void SetLock(bool lock)
    {
      lock_ = lock;
    }

    bool HasLock() const
    {
      return lock_;
    }

    void Format(std::string& target) const;
  };
}
