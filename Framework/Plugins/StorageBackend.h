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

#include "../Common/DatabaseManager.h"
#include <orthanc/OrthancCDatabasePlugin.h>


namespace OrthancDatabases
{
  class StorageBackend : public boost::noncopyable
  {
  private:
    DatabaseManager   manager_;

  protected:
    void ReadFromString(void*& buffer,
                        size_t& size,
                        const std::string& content);

  public:
    StorageBackend(IDatabaseFactory* factory);

    virtual ~StorageBackend()
    {
    }

    DatabaseManager& GetManager() 
    {
      return manager_;
    }
    
    // NB: These methods will always be invoked in mutual exclusion,
    // as having access to some "DatabaseManager::Transaction" implies
    // that the parent "DatabaseManager" is locked
    virtual void Create(DatabaseManager::Transaction& transaction,
                        const std::string& uuid,
                        const void* content,
                        size_t size,
                        OrthancPluginContentType type);

    virtual void Read(void*& content,
                      size_t& size,
                      DatabaseManager::Transaction& transaction, 
                      const std::string& uuid,
                      OrthancPluginContentType type);

    virtual void Remove(DatabaseManager::Transaction& transaction,
                        const std::string& uuid,
                        OrthancPluginContentType type);

    static void Register(OrthancPluginContext* context,
                         StorageBackend* backend);   // Takes ownership

    // For unit testing!
    void ReadToString(std::string& content,
                      DatabaseManager::Transaction& transaction, 
                      const std::string& uuid,
                      OrthancPluginContentType type);

    static void Finalize();
  };
}
