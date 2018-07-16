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

#include "../../Framework/Plugins/StorageBackend.h"
#include "../../Framework/PostgreSQL/PostgreSQLParameters.h"

namespace OrthancDatabases
{
  class PostgreSQLStorageArea : public StorageBackend
  {
  private:
    class Factory : public IDatabaseFactory
    {
    private:
      PostgreSQLStorageArea&  that_;

    public:
      Factory(PostgreSQLStorageArea& that) :
      that_(that)
      {
      }

      virtual Dialect GetDialect() const
      {
        return Dialect_PostgreSQL;
      }

      virtual IDatabase* Open()
      {
        return that_.OpenInternal();
      }
    };

    OrthancPluginContext*  context_;
    PostgreSQLParameters   parameters_;
    bool                   clearAll_;

    IDatabase* OpenInternal();

  public:
    PostgreSQLStorageArea(const PostgreSQLParameters& parameters);

    void SetClearAll(bool clear)
    {
      clearAll_ = clear;
    }
  };
}
