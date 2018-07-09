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
#include "../../Framework/MySQL/MySQLDatabase.h"

#include <Plugins/Samples/Common/OrthancPluginCppWrapper.h>
#include <Core/Logging.h>

static std::auto_ptr<OrthancDatabases::MySQLIndex> backend_;


static bool DisplayPerformanceWarning()
{
  (void) DisplayPerformanceWarning;   // Disable warning about unused function
  LOG(WARNING) << "Performance warning in MySQL index: "
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

    OrthancPluginSetDescription(context, "Stores the Orthanc index into a MySQL database.");

    OrthancPlugins::OrthancConfiguration configuration(context);

    if (!configuration.IsSection("MySQL"))
    {
      LOG(WARNING) << "No available configuration for the MySQL index plugin";
      return 0;
    }

    OrthancPlugins::OrthancConfiguration mysql;
    configuration.GetSection(mysql, "MySQL");

    bool enable;
    if (!mysql.LookupBooleanValue(enable, "EnableIndex") ||
        !enable)
    {
      LOG(WARNING) << "The MySQL index is currently disabled, set \"EnableIndex\" "
                   << "to \"true\" in the \"MySQL\" section of the configuration file of Orthanc";
      return 0;
    }

    try
    {
      OrthancDatabases::MySQLParameters parameters(mysql);

      /* Create the database back-end */
      backend_.reset(new OrthancDatabases::MySQLIndex(parameters));

      /* Register the MySQL index into Orthanc */
      OrthancPlugins::DatabaseBackendAdapter::Register(context, *backend_);
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
    LOG(WARNING) << "MySQL index is finalizing";

    backend_.reset(NULL);
    OrthancDatabases::MySQLDatabase::GlobalFinalization();
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "mysql-index";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_PLUGIN_VERSION;
  }
}
