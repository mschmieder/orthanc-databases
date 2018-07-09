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


if (STATIC_BUILD OR NOT USE_SYSTEM_MYSQL_CLIENT)
  set(MARIADB_CLIENT_VERSION_MAJOR "10")
  set(MARIADB_CLIENT_VERSION_MINOR "3")
  set(MARIADB_CLIENT_VERSION_PATCH "6")
  set(MARIADB_PACKAGE_VERSION "3.0.5")
  set(MARIADB_CLIENT_SOURCES_DIR ${CMAKE_BINARY_DIR}/mariadb-connector-c-${MARIADB_PACKAGE_VERSION}-src)
  set(MARIADB_CLIENT_MD5 "b846584b8b7a39c51a6e83986b57c71c")
  set(MARIADB_CLIENT_URL "http://www.orthanc-server.com/downloads/third-party/mariadb-connector-c-${MARIADB_PACKAGE_VERSION}-src.tar.gz")

  if (IS_DIRECTORY "${MARIADB_CLIENT_SOURCES_DIR}")
    set(FirstRun OFF)
  else()
    set(FirstRun ON)
  endif()

  DownloadPackage(${MARIADB_CLIENT_MD5} ${MARIADB_CLIENT_URL} "${MARIADB_CLIENT_SOURCES_DIR}")

  if (FirstRun)
    execute_process(
      COMMAND ${PATCH_EXECUTABLE} -p0 -N -i
      ${CMAKE_CURRENT_LIST_DIR}/../MariaDB/mariadb-connector-c-3.0.5.patch
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      RESULT_VARIABLE Failure
      )

    if (Failure)
      message(FATAL_ERROR "Error while patching a file")
    endif()
  endif()


  include(${MARIADB_CLIENT_SOURCES_DIR}/cmake/CheckIncludeFiles.cmake)
  include(${MARIADB_CLIENT_SOURCES_DIR}/cmake/CheckFunctions.cmake)
  include(${MARIADB_CLIENT_SOURCES_DIR}/cmake/CheckTypes.cmake)

  set(MARIADB_CLIENT_VERSION "${MARIADB_CLIENT_VERSION_MAJOR}.${MARIADB_CLIENT_VERSION_MINOR}.${MARIADB_CLIENT_VERSION_PATCH}")
  set(MARIADB_BASE_VERSION "mariadb-${MARIADB_CLIENT_VERSION_MAJOR}.${MARIADB_CLIENT_VERSION_MINOR}")
  math(EXPR MARIADB_VERSION_ID "${MARIADB_CLIENT_VERSION_MAJOR} * 10000 +
                              ${MARIADB_CLIENT_VERSION_MINOR} * 100   +
                              ${MARIADB_CLIENT_VERSION_PATCH}")

  set(HAVE_DLOPEN 1)
  set(PROTOCOL_VERSION ${MARIADB_CLIENT_VERSION_MAJOR})
  set(MARIADB_PORT 3306)
  set(MARIADB_UNIX_ADDR "/var/run/mysqld/mysqld.sock")
  set(DEFAULT_CHARSET "latin1")

  FOREACH(plugin mysql_native_password mysql_old_password pvio_socket)
    set(EXTERNAL_PLUGINS "${EXTERNAL_PLUGINS} extern struct st_mysql_client_plugin ${plugin}_client_plugin;\n")
    set(BUILTIN_PLUGINS "${BUILTIN_PLUGINS}   (struct st_mysql_client_plugin *)&${plugin}_client_plugin,\n")
  ENDFOREACH()

  configure_file(
    ${MARIADB_CLIENT_SOURCES_DIR}/include/ma_config.h.in
    ${MARIADB_CLIENT_SOURCES_DIR}/include/ma_config.h
    )

  configure_file(
    ${MARIADB_CLIENT_SOURCES_DIR}/include/mariadb_version.h.in
    ${MARIADB_CLIENT_SOURCES_DIR}/include/mariadb_version.h
    )

  configure_file(
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_client_plugin.c.in
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_client_plugin.c
    )

  include_directories(
    ${MARIADB_CLIENT_SOURCES_DIR}/include
    )

  set(MYSQL_CLIENT_SOURCES
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_alloc.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_array.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_charset.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_client_plugin.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_compress.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_context.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_default.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_dtoa.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_errmsg.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_hash.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_init.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_io.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_list.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_ll2str.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_loaddata.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_net.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_password.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_pvio.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_sha1.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_stmt_codec.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_string.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_time.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/ma_tls.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/mariadb_async.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/mariadb_charset.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/mariadb_dyncol.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/mariadb_lib.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/mariadb_stmt.c
    ${MARIADB_CLIENT_SOURCES_DIR}/libmariadb/secure/openssl.c
    ${MARIADB_CLIENT_SOURCES_DIR}/plugins/auth/my_auth.c
    ${MARIADB_CLIENT_SOURCES_DIR}/plugins/auth/old_password.c
    ${MARIADB_CLIENT_SOURCES_DIR}/plugins/pvio/pvio_socket.c
    )

  set_property(
    SOURCE ${MYSQL_CLIENT_SOURCES}
    PROPERTY COMPILE_DEFINITIONS "HAVE_OPENSSL=1;HAVE_TLS=1;HAVE_REMOTEIO=1;HAVE_COMPRESS=1;LIBMARIADB;THREAD"
    )

  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    link_libraries(shlwapi)
  endif()

else()
  find_path(MYSQLCLIENT_INCLUDE_DIR mysql.h
    /usr/local/include/mysql
    /usr/include/mysql
    )

  if (MYSQLCLIENT_INCLUDE_DIR)
    include_directories(${MYSQLCLIENT_INCLUDE_DIR})
    set(CMAKE_REQUIRED_INCLUDES "${MYSQLCLIENT_INCLUDE_DIR}")
  endif()

  check_include_file(mysql.h HAVE_MYSQL_CLIENT_H)
  if (NOT HAVE_MYSQL_CLIENT_H)
    message(FATAL_ERROR "Please install the libmysqlclient-dev package")
  endif()

  check_library_exists(mysqlclient mysql_init "" HAVE_MYSQL_CLIENT_LIB)
  if (NOT HAVE_MYSQL_CLIENT_LIB)
    message(FATAL_ERROR "Unable to find the mysqlclient library")
  endif()

  link_libraries(mysqlclient)
endif()
