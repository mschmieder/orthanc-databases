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


#include "MySQLIndex.h"

#include "../../Framework/Plugins/GlobalProperties.h"
#include "../../Framework/MySQL/MySQLDatabase.h"
#include "../../Framework/MySQL/MySQLTransaction.h"

#include <EmbeddedResources.h>  // Auto-generated file

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <ctype.h>

namespace OrthancDatabases
{
  IDatabase* MySQLIndex::OpenInternal()
  {
    uint32_t expectedVersion = 6;
    if (context_)
    {
      expectedVersion = OrthancPluginGetExpectedDatabaseVersion(context_);
    }
    else
    {
      // This case only occurs during unit testing
      expectedVersion = 6;
    }

    // Check the expected version of the database
    if (expectedVersion != 6)
    {
      LOG(ERROR) << "This database plugin is incompatible with your version of Orthanc "
                 << "expecting the DB schema version " << expectedVersion 
                 << ", but this plugin is only compatible with version 6";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Plugin);
    }

    if (!MySQLDatabase::IsAlphanumericString(parameters_.GetDatabase()))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (clearAll_)
    {
      MySQLDatabase::ClearDatabase(parameters_);
    }
    
    std::auto_ptr<MySQLDatabase> db(new MySQLDatabase(parameters_));

    db->Open();
    
    db->Execute("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE", false);

    if (parameters_.HasLock())
    {
      db->AdvisoryLock(42 /* some arbitrary constant */);
    }
    
    {
      MySQLTransaction t(*db);

      db->Execute("ALTER DATABASE " + parameters_.GetDatabase() + 
                  " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", false);
    
      if (!db->DoesTableExist(t, "Resources"))
      {
        std::string query;

        Orthanc::EmbeddedResources::GetFileResource
          (query, Orthanc::EmbeddedResources::MYSQL_PREPARE_INDEX);
        db->Execute(query, true);

        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_DatabaseSchemaVersion, expectedVersion);
        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_DatabasePatchLevel, 1);
      }

      if (!db->DoesTableExist(t, "Resources"))
      {
        LOG(ERROR) << "Corrupted MySQL database";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);        
      }

      int version = 0;
      if (!LookupGlobalIntegerProperty(version, *db, t, Orthanc::GlobalProperty_DatabaseSchemaVersion) ||
          version != 6)
      {
        LOG(ERROR) << "MySQL plugin is incompatible with database schema version: " << version;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
      }

      int revision;
      if (!LookupGlobalIntegerProperty(revision, *db, t, Orthanc::GlobalProperty_DatabasePatchLevel))
      {
        revision = 1;
        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_DatabasePatchLevel, revision);
      }

      if (revision != 1)
      {
        LOG(ERROR) << "MySQL plugin is incompatible with database schema revision: " << revision;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
      }

      t.Commit();
    }
          
    return db.release();
  }


  MySQLIndex::MySQLIndex(const MySQLParameters& parameters) :
    IndexBackend(new Factory(*this)),
    context_(NULL),
    parameters_(parameters),
    clearAll_(false)
  {
  }


  int64_t MySQLIndex::CreateResource(const char* publicId,
                                     OrthancPluginResourceType type)
  {
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, GetManager(),
        "INSERT INTO Resources VALUES(${}, ${type}, ${id}, NULL)");
    
      statement.SetParameterType("id", ValueType_Utf8String);
      statement.SetParameterType("type", ValueType_Integer64);

      Dictionary args;
      args.SetUtf8Value("id", publicId);
      args.SetIntegerValue("type", static_cast<int>(type));
    
      statement.Execute(args);
    }

    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, GetManager(),
        "SELECT LAST_INSERT_ID()");

      statement.Execute();
      
      return ReadInteger64(statement, 0);
    }
  }


  void MySQLIndex::DeleteResource(int64_t id)
  {
    ClearDeletedFiles();

    // Recursive exploration of resources to be deleted, from the "id"
    // resource to the top of the tree of resources
    
    bool done = false;

    while (!done)
    {
      int64_t parentId;
      
      {
        DatabaseManager::CachedStatement lookupSiblings(
          STATEMENT_FROM_HERE, GetManager(),
          "SELECT parentId FROM Resources "
          "WHERE parentId = (SELECT parentId FROM Resources WHERE internalId=${id});");

        lookupSiblings.SetParameterType("id", ValueType_Integer64);

        Dictionary args;
        args.SetIntegerValue("id", id);
    
        lookupSiblings.Execute(args);

        if (lookupSiblings.IsDone())
        {
          // "id" is a root node
          done = true;
        }
        else
        {
          parentId = ReadInteger64(lookupSiblings, 0);
          lookupSiblings.Next();

          if (lookupSiblings.IsDone())
          {
            // "id" has no sibling node, recursively remove
            done = false;
            id = parentId;
          }
          else
          {
            // "id" has at least one sibling node: the parent node is the remaining ancestor
            done = true;

            DatabaseManager::CachedStatement parent(
              STATEMENT_FROM_HERE, GetManager(),
              "SELECT publicId, resourceType FROM Resources WHERE internalId=${id};");
            
            parent.SetParameterType("id", ValueType_Integer64);

            Dictionary args;
            args.SetIntegerValue("id", parentId);
    
            parent.Execute(args);

            GetOutput().SignalRemainingAncestor(
              ReadString(parent, 0),
              static_cast<OrthancPluginResourceType>(ReadInteger32(parent, 1)));
          }
        }
      }
    }

    {
      DatabaseManager::CachedStatement deleteHierarchy(
        STATEMENT_FROM_HERE, GetManager(),
        "DELETE FROM Resources WHERE internalId IN (SELECT * FROM (SELECT internalId FROM Resources WHERE internalId=${id} OR parentId=${id} OR parentId IN (SELECT internalId FROM Resources WHERE parentId=${id}) OR parentId IN (SELECT internalId FROM Resources WHERE parentId IN (SELECT internalId FROM Resources WHERE parentId=${id}))) as t);");
      
      deleteHierarchy.SetParameterType("id", ValueType_Integer64);
      
      Dictionary args;
      args.SetIntegerValue("id", id);
    
      deleteHierarchy.Execute(args);
    }

    SignalDeletedFiles();
  }
}
