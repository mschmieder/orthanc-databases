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


#include "PluginInitialization.h"

#include "../Common/ImplicitTransaction.h"

#include <Core/Logging.h>

namespace OrthancDatabases
{
  static bool DisplayPerformanceWarning(const std::string& dbms,
                                        bool isIndex)
  {
    (void) DisplayPerformanceWarning;   // Disable warning about unused function
    LOG(WARNING) << "Performance warning in " << dbms
                 << (isIndex ? " index" : " storage area")
                 << ": Non-release build, runtime debug assertions are turned on";
    return true;
  }


  bool InitializePlugin(OrthancPluginContext* context,
                        const std::string& dbms,
                        bool isIndex)
  {
    Orthanc::Logging::Initialize(context);
    ImplicitTransaction::SetErrorOnDoubleExecution(false);

    assert(DisplayPerformanceWarning(dbms, isIndex));

    /* Check the version of the Orthanc core */

    bool useFallback = true;
    bool isOptimal = false;

#if defined(ORTHANC_PLUGINS_VERSION_IS_ABOVE)         // Macro introduced in Orthanc 1.3.1
#  if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 4, 0)
    if (OrthancPluginCheckVersionAdvanced(context, 0, 9, 5) == 0)
    {
      LOG(ERROR) << "Your version of Orthanc (" << context->orthancVersion 
                 << ") must be above 0.9.5 to run this plugin";
      return false;
    }

    if (OrthancPluginCheckVersionAdvanced(context, 1, 4, 0) == 1)
    {
      ImplicitTransaction::SetErrorOnDoubleExecution(true);
      isOptimal = true;
    }

    useFallback = false;
#  endif
#endif

    if (useFallback &&
        OrthancPluginCheckVersion(context) == 0)
    {
      LOG(ERROR) << "Your version of Orthanc (" 
                 << context->orthancVersion << ") must be above "
                 << ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER << "."
                 << ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER << "."
                 << ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER
                 << " to run this plugin";
      return false;
    }

    if (!isOptimal &&
        isIndex)
    {
      LOG(WARNING) << "Performance warning in " << dbms
                   << " index: Your version of Orthanc (" 
                   << context->orthancVersion << ") should be upgraded to 1.4.0 "
                   << "to benefit from best performance";
    }


    std::string description = ("Stores the Orthanc " +
                               std::string(isIndex ? "index" : "storage area") +
                               " into a " + dbms + " database");
    
    OrthancPluginSetDescription(context, description.c_str());

    return true;
  }
}
