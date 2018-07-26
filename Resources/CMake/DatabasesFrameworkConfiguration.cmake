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



#####################################################################
## Enable the Orthanc subcomponents depending on the configuration
#####################################################################

if (ENABLE_SQLITE_BACKEND)
  set(ENABLE_SQLITE ON)
endif()

if (ENABLE_POSTGRESQL_BACKEND)
  set(ENABLE_SSL ON)
  set(ENABLE_ZLIB ON)
endif()

if (ENABLE_MYSQL_BACKEND)
  set(ENABLE_CRYPTO_OPTIONS ON)
  set(ENABLE_SSL ON)
  set(ENABLE_ZLIB ON)
  set(ENABLE_LOCALE ON)      # iconv is needed
  set(ENABLE_WEB_CLIENT ON)  # libcurl is needed

  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(ENABLE_OPENSSL_ENGINES ON)
  endif()
endif()


#####################################################################
## Configure the Orthanc Framework
#####################################################################

# Those modules of the Orthanc framework are not needed when dealing
# with databases
set(ENABLE_MODULE_IMAGES OFF)
set(ENABLE_MODULE_JOBS OFF)
set(ENABLE_MODULE_DICOM OFF)

include(${ORTHANC_ROOT}/Resources/CMake/OrthancFrameworkConfiguration.cmake)
include_directories(${ORTHANC_ROOT})


#####################################################################
## Common source files for the databases
#####################################################################

set(ORTHANC_DATABASES_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

set(DATABASES_SOURCES
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/BinaryStringValue.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/DatabaseManager.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/Dictionary.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/FileValue.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/GenericFormatter.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/ImplicitTransaction.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/Integer64Value.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/NullValue.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/Query.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/ResultBase.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/StatementLocation.cpp
  ${ORTHANC_DATABASES_ROOT}/Framework/Common/Utf8StringValue.cpp
  )


#####################################################################
## Configure SQLite if need be
#####################################################################

if (ENABLE_SQLITE_BACKEND)
  list(APPEND DATABASES_SOURCES
    ${ORTHANC_DATABASES_ROOT}/Framework/SQLite/SQLiteDatabase.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/SQLite/SQLiteResult.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/SQLite/SQLiteStatement.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/SQLite/SQLiteTransaction.cpp
    )
endif()


#####################################################################
## Configure MySQL client (MariaDB) if need be
#####################################################################

if (ENABLE_MYSQL_BACKEND)
  include(${CMAKE_CURRENT_LIST_DIR}/MariaDBConfiguration.cmake)
  add_definitions(-DORTHANC_ENABLE_MYSQL=1)
  list(APPEND DATABASES_SOURCES
    ${ORTHANC_DATABASES_ROOT}/Framework/MySQL/MySQLDatabase.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/MySQL/MySQLParameters.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/MySQL/MySQLResult.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/MySQL/MySQLStatement.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/MySQL/MySQLTransaction.cpp
    ${MYSQL_CLIENT_SOURCES}
    )
else()
  unset(USE_SYSTEM_MYSQL_CLIENT CACHE)
  add_definitions(-DORTHANC_ENABLE_MYSQL=0)
endif()



#####################################################################
## Configure PostgreSQL client if need be
#####################################################################

if (ENABLE_POSTGRESQL_BACKEND)
  include(${CMAKE_CURRENT_LIST_DIR}/PostgreSQLConfiguration.cmake)
  add_definitions(-DORTHANC_ENABLE_POSTGRESQL=1)
  list(APPEND DATABASES_SOURCES
    ${ORTHANC_DATABASES_ROOT}/Framework/PostgreSQL/PostgreSQLDatabase.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/PostgreSQL/PostgreSQLLargeObject.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/PostgreSQL/PostgreSQLParameters.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/PostgreSQL/PostgreSQLResult.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/PostgreSQL/PostgreSQLStatement.cpp
    ${ORTHANC_DATABASES_ROOT}/Framework/PostgreSQL/PostgreSQLTransaction.cpp
    ${LIBPQ_SOURCES}
    )
else()
  unset(USE_SYSTEM_LIBPQ CACHE)
  add_definitions(-DORTHANC_ENABLE_POSTGRESQL=0)
endif()
