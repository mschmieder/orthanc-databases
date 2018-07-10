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
#include "../../Framework/MySQL/MySQLParameters.h"


namespace OrthancDatabases
{
  class MySQLStorageArea : public StorageBackend
  {
  private:
    class Factory : public IDatabaseFactory
    {
    private:
      MySQLStorageArea&  that_;

    public:
      Factory(MySQLStorageArea& that) :
        that_(that)
      {
      }

      virtual Dialect GetDialect() const
      {
        return Dialect_MySQL;
      }

      virtual IDatabase* Open()
      {
        return that_.OpenInternal();
      }
    };

    OrthancPluginContext*  context_;
    MySQLParameters        parameters_;
    bool                   clearAll_;

    IDatabase* OpenInternal();

  public:
    MySQLStorageArea(const MySQLParameters& parameters);

    void SetClearAll(bool clear)
    {
      clearAll_ = clear;
    }
  };
}
