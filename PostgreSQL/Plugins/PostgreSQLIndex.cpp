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


#include "PostgreSQLIndex.h"

#include "../../Framework/Plugins/GlobalProperties.h"
#include "../../Framework/PostgreSQL/PostgreSQLDatabase.h"
#include "../../Framework/PostgreSQL/PostgreSQLTransaction.h"

#include <EmbeddedResources.h>  // Auto-generated file

#include <Core/Logging.h>
#include <Core/OrthancException.h>


namespace Orthanc
{
  // Some aliases for internal properties
  static const GlobalProperty GlobalProperty_HasTrigramIndex = GlobalProperty_DatabaseInternal0;
}


namespace OrthancDatabases
{
  IDatabase* PostgreSQLIndex::OpenInternal()
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

    std::auto_ptr<PostgreSQLDatabase> db(new PostgreSQLDatabase(parameters_));

    db->Open();

    if (parameters_.HasLock())
    {
      db->AdvisoryLock(42 /* some arbitrary constant */);
    }

    if (clearAll_)
    {
      db->ClearAll();
    }

    {
      PostgreSQLTransaction t(*db);

      if (!db->DoesTableExist("Resources"))
      {
        std::string query;

        Orthanc::EmbeddedResources::GetFileResource
          (query, Orthanc::EmbeddedResources::POSTGRESQL_PREPARE_INDEX);
        db->Execute(query);

        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_DatabaseSchemaVersion, expectedVersion);
        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_DatabasePatchLevel, 1);
        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_HasTrigramIndex, 0);
      }
          
      if (!db->DoesTableExist("Resources"))
      {
        LOG(ERROR) << "Corrupted PostgreSQL database";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);        
      }

      int version = 0;
      if (!LookupGlobalIntegerProperty(version, *db, t, Orthanc::GlobalProperty_DatabaseSchemaVersion) ||
          version != 6)
      {
        LOG(ERROR) << "PostgreSQL plugin is incompatible with database schema version: " << version;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
      }

      int revision;
      if (!LookupGlobalIntegerProperty(revision, *db, t, Orthanc::GlobalProperty_DatabasePatchLevel))
      {
        revision = 1;
        SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_DatabasePatchLevel, revision);
      }

      int hasTrigram = 0;
      if (!LookupGlobalIntegerProperty(hasTrigram, *db, t, Orthanc::GlobalProperty_HasTrigramIndex) ||
          hasTrigram != 1)
      {
        /**
         * Apply fix for performance issue (speed up wildcard search
         * by using GIN trigrams). This implements the patch suggested
         * in issue #47, BUT we also keep the original
         * "DicomIdentifiersIndexValues", as it leads to better
         * performance for "strict" searches (i.e. searches involving
         * no wildcard).
         * https://www.postgresql.org/docs/current/static/pgtrgm.html
         * https://bitbucket.org/sjodogne/orthanc/issues/47/index-improvements-for-pg-plugin
         **/
        try
        {
          LOG(INFO) << "Trying to enable trigram matching on the PostgreSQL database to speed up wildcard searches";
          db->Execute(
            "CREATE EXTENSION pg_trgm; "
            "CREATE INDEX DicomIdentifiersIndexValues2 ON DicomIdentifiers USING gin(value gin_trgm_ops);");

          SetGlobalIntegerProperty(*db, t, Orthanc::GlobalProperty_HasTrigramIndex, 1);
        }
        catch (Orthanc::OrthancException&)
        {
          LOG(WARNING) << "Performance warning: Your PostgreSQL server does not support trigram matching";
          LOG(WARNING) << "-> Consider installing the \"pg_trgm\" extension on the PostgreSQL server, e.g. on Debian: sudo apt install postgresql-contrib";
        }
      }

      if (revision != 1)
      {
        LOG(ERROR) << "PostgreSQL plugin is incompatible with database schema revision: " << revision;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
      }

      t.Commit();
    }

    return db.release();
  }


  PostgreSQLIndex::PostgreSQLIndex(const PostgreSQLParameters& parameters) :
    IndexBackend(new Factory(*this)),
    context_(NULL),
    parameters_(parameters),
    clearAll_(false)
  {
  }

  
  int64_t PostgreSQLIndex::CreateResource(const char* publicId,
                                          OrthancPluginResourceType type)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "INSERT INTO Resources VALUES(${}, ${type}, ${id}, NULL) RETURNING internalId");
     
    statement.SetParameterType("id", ValueType_Utf8String);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("id", publicId);
    args.SetIntegerValue("type", static_cast<int>(type));
     
    statement.Execute(args);

    return ReadInteger64(statement, 0);
  }
}
