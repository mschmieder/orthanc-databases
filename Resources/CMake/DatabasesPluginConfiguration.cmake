# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2018 Osimis S.A., Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.



include(${CMAKE_SOURCE_DIR}/../Resources/CMake/DatabasesFrameworkConfiguration.cmake)
include(${ORTHANC_ROOT}/Resources/CMake/AutoGeneratedCode.cmake)


if (STATIC_BUILD OR NOT USE_SYSTEM_ORTHANC_SDK)
  if (ORTHANC_SDK_VERSION STREQUAL "0.9.5")
    include_directories(${ORTHANC_DATABASES_ROOT}/Resources/Orthanc/Sdk-0.9.5)
  elseif (ORTHANC_SDK_VERSION STREQUAL "1.4.0")
    include_directories(${ORTHANC_DATABASES_ROOT}/Resources/Orthanc/Sdk-1.4.0)
  elseif (ORTHANC_SDK_VERSION STREQUAL "framework")
    include_directories(${ORTHANC_ROOT}/Plugins/Include)
  else()
    message(FATAL_ERROR "Unsupported version of the Orthanc plugin SDK: ${ORTHANC_SDK_VERSION}")
  endif()
else ()
  CHECK_INCLUDE_FILE_CXX(orthanc/OrthancCppDatabasePlugin.h HAVE_ORTHANC_H)
  if (NOT HAVE_ORTHANC_H)
    message(FATAL_ERROR "Please install the headers of the Orthanc plugins SDK")
  endif()
endif()


list(APPEND DATABASES_SOURCES
  ${ORTHANC_CORE_SOURCES}
  ${ORTHANC_DATABASES_ROOT}/Framework/Plugins/GlobalProperties.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Plugins/IndexBackend.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Plugins/StorageBackend.cpp
  ${ORTHANC_ROOT}/Plugins/Samples/Common/OrthancPluginCppWrapper.cpp
  )
