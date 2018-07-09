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


#include "PostgreSQLStorageArea.h"

#include "../../Framework/Common/FileValue.h"
#include "../../Framework/PostgreSQL/PostgreSQLTransaction.h"

#include <Plugins/Samples/Common/OrthancPluginCppWrapper.h>
#include <Core/Logging.h>


namespace OrthancDatabases
{
  IDatabase* PostgreSQLStorageArea::OpenInternal()
  {
    std::auto_ptr<PostgreSQLDatabase> db(new PostgreSQLDatabase(parameters_));

    db->Open();

    if (parameters_.HasLock())
    {
      db->AdvisoryLock(43 /* some arbitrary constant */);
    }

    if (clearAll_)
    {
      db->ClearAll();
    }

    {
      PostgreSQLTransaction t(*db);

      if (!db->DoesTableExist("StorageArea"))
      {
        db->Execute("CREATE TABLE IF NOT EXISTS StorageArea("
                    "uuid VARCHAR NOT NULL PRIMARY KEY,"
                    "content OID NOT NULL,"
                    "type INTEGER NOT NULL)");

        // Automatically remove the large objects associated with the table
        db->Execute("CREATE OR REPLACE RULE StorageAreaDelete AS ON DELETE "
                    "TO StorageArea DO SELECT lo_unlink(old.content);");
      }

      t.Commit();
    }

    return db.release();
  }


  PostgreSQLStorageArea::PostgreSQLStorageArea(const PostgreSQLParameters& parameters) :
    StorageBackend(new Factory(*this)),
    parameters_(parameters),
    clearAll_(false)
  {
  }


  void PostgreSQLStorageArea::Create(DatabaseManager::Transaction& transaction,
                                     const std::string& uuid,
                                     const void* content,
                                     size_t size,
                                     OrthancPluginContentType type)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "INSERT INTO StorageArea VALUES (${uuid}, ${content}, ${type})");
     
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("content", ValueType_File);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("uuid", uuid);
    args.SetFileValue("content", content, size);
    args.SetIntegerValue("type", type);
     
    statement.Execute(args);
  }


  void PostgreSQLStorageArea::Read(void*& content,
                                   size_t& size,
                                   DatabaseManager::Transaction& transaction, 
                                   const std::string& uuid,
                                   OrthancPluginContentType type) 
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "SELECT content FROM StorageArea WHERE uuid=${uuid} AND type=${type}");
     
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("uuid", uuid);
    args.SetIntegerValue("type", type);
     
    statement.Execute(args);

    if (statement.IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }
    else if (statement.GetResultFieldsCount() != 1 ||
             statement.GetResultField(0).GetType() != ValueType_File)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
    }
    else
    {
      const FileValue& value = dynamic_cast<const FileValue&>(statement.GetResultField(0));
      ReadFromString(content, size, value.GetContent());
    }
  }


  void PostgreSQLStorageArea::Remove(DatabaseManager::Transaction& transaction,
                                     const std::string& uuid,
                                     OrthancPluginContentType type)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "DELETE FROM StorageArea WHERE uuid=${uuid} AND type=${type}");
     
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("uuid", uuid);
    args.SetIntegerValue("type", type);
     
    statement.Execute(args);
  }
}
