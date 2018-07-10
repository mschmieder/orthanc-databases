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


#include "../../Framework/Plugins/StorageBackend.h"

#include <Core/Logging.h>


#include "../../Framework/Common/Integer64Value.h"
#include "../../Framework/MySQL/MySQLDatabase.h"
#include "../../Framework/MySQL/MySQLResult.h"
#include "../../Framework/MySQL/MySQLStatement.h"
#include "../../Framework/MySQL/MySQLTransaction.h"

#include <boost/math/special_functions/round.hpp>

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

    IDatabase* OpenInternal()
    {
      std::auto_ptr<MySQLDatabase> db(new MySQLDatabase(parameters_));

      db->Open();

      if (parameters_.HasLock())
      {
        db->AdvisoryLock(43 /* some arbitrary constant */);
      }

      {
        MySQLTransaction t(*db);

        int64_t size;
        if (db->LookupGlobalIntegerVariable(size, "max_allowed_packet"))
        {
          int mb = boost::math::iround(static_cast<double>(size) /
                                       static_cast<double>(1024 * 1024));
          LOG(WARNING) << "Your MySQL server cannot "
                       << "store DICOM files larger than " << mb << "MB";
          LOG(WARNING) << "  => Consider increasing \"max_allowed_packet\" "
                       << "in \"my.cnf\" if this limit is insufficient for your use";
        }
        else
        {
          LOG(WARNING) << "Unable to auto-detect the maximum size of DICOM "
                       << "files that can be stored in this MySQL server";
        }
               
        if (clearAll_)
        {
          db->Execute("DROP TABLE IF EXISTS StorageArea", false);
        }

        db->Execute("CREATE TABLE IF NOT EXISTS StorageArea("
                    "uuid VARCHAR(64) NOT NULL PRIMARY KEY,"
                    "content LONGBLOB NOT NULL,"
                    "type INTEGER NOT NULL)", false);

        t.Commit();
      }

      return db.release();
    }

  public:
    MySQLStorageArea(const MySQLParameters& parameters) :
      StorageBackend(new Factory(*this)),
      parameters_(parameters),
      clearAll_(false)
    {
    }

    void SetClearAll(bool clear)
    {
      clearAll_ = clear;
    }
  };
}




static bool DisplayPerformanceWarning()
{
  (void) DisplayPerformanceWarning;   // Disable warning about unused function
  LOG(WARNING) << "Performance warning in MySQL storage area: "
               << "Non-release build, runtime debug assertions are turned on";
  return true;
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    Orthanc::Logging::Initialize(context);

    assert(DisplayPerformanceWarning());

    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context) == 0)
    {
      char info[1024];
      sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
              context->orthancVersion,
              ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      OrthancPluginLogError(context, info);
      return -1;
    }

    OrthancPluginSetDescription(context, "Stores the Orthanc storage area into a MySQL database.");

    OrthancPlugins::OrthancConfiguration configuration(context);

    if (!configuration.IsSection("MySQL"))
    {
      LOG(WARNING) << "No available configuration for the MySQL storage area plugin";
      return 0;
    }

    OrthancPlugins::OrthancConfiguration mysql;
    configuration.GetSection(mysql, "MySQL");

    bool enable;
    if (!mysql.LookupBooleanValue(enable, "EnableStorage") ||
        !enable)
    {
      LOG(WARNING) << "The MySQL storage area is currently disabled, set \"EnableStorage\" "
                   << "to \"true\" in the \"MySQL\" section of the configuration file of Orthanc";
      return 0;
    }

    try
    {
      OrthancDatabases::MySQLParameters parameters(mysql);
      OrthancDatabases::StorageBackend::Register
        (context, new OrthancDatabases::MySQLStorageArea(parameters));
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << e.What();
      return -1;
    }
    catch (...)
    {
      LOG(ERROR) << "Native exception while initializing the plugin";
      return -1;
    }

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
    LOG(WARNING) << "MySQL storage area is finalizing";

    OrthancDatabases::StorageBackend::Finalize();
    OrthancDatabases::MySQLDatabase::GlobalFinalization();
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "mysql-storage";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_PLUGIN_VERSION;
  }
}
