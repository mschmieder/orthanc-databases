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

#include "../../Framework/Plugins/IndexBackend.h"
#include "../../Framework/MySQL/MySQLParameters.h"

namespace OrthancDatabases
{
  class MySQLIndex : public IndexBackend 
  {
  private:
    class Factory : public IDatabaseFactory
    {
    private:
      MySQLIndex&  that_;

    public:
      Factory(MySQLIndex& that) :
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
    MySQLIndex(const MySQLParameters& parameters);

    void SetOrthancPluginContext(OrthancPluginContext* context)
    {
      context_ = context;
    }

    void SetClearAll(bool clear)
    {
      clearAll_ = clear;
    }

    virtual int64_t CreateResource(const char* publicId,
                                   OrthancPluginResourceType type);

    virtual void DeleteResource(int64_t id);
  };
}
