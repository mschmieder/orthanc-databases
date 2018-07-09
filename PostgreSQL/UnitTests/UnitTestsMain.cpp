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


#include "../Plugins/PostgreSQLIndex.h"

#include <Core/Logging.h>
#include <gtest/gtest.h>

OrthancDatabases::PostgreSQLParameters  globalParameters_;

#include "../../Framework/Plugins/IndexUnitTests.h"


#if ORTHANC_POSTGRESQL_STATIC == 1
#  include <c.h>  // PostgreSQL includes

TEST(PostgreSQL, Version)
{
  ASSERT_STREQ("9.6.1", PG_VERSION);
}
#endif


TEST(PostgreSQLParameters, Basic)
{
  OrthancDatabases::PostgreSQLParameters p;
  p.SetDatabase("world");

  ASSERT_EQ("postgresql://localhost:5432/world", p.GetConnectionUri());

  p.ResetDatabase();
  ASSERT_EQ("postgresql://localhost:5432/", p.GetConnectionUri());

  p.SetDatabase("hello");
  ASSERT_EQ("postgresql://localhost:5432/hello", p.GetConnectionUri());

  p.SetHost("server");
  ASSERT_EQ("postgresql://server:5432/hello", p.GetConnectionUri());

  p.SetPortNumber(1234);
  ASSERT_EQ("postgresql://server:1234/hello", p.GetConnectionUri());

  p.SetPortNumber(5432);
  ASSERT_EQ("postgresql://server:5432/hello", p.GetConnectionUri());

  p.SetUsername("user");
  p.SetPassword("pass");
  ASSERT_EQ("postgresql://user:pass@server:5432/hello", p.GetConnectionUri());

  p.SetPassword("");
  ASSERT_EQ("postgresql://user@server:5432/hello", p.GetConnectionUri());

  p.SetUsername("");
  p.SetPassword("pass");
  ASSERT_EQ("postgresql://server:5432/hello", p.GetConnectionUri());

  p.SetUsername("");
  p.SetPassword("");
  ASSERT_EQ("postgresql://server:5432/hello", p.GetConnectionUri());

  p.SetConnectionUri("hello://world");
  ASSERT_EQ("hello://world", p.GetConnectionUri());
}


TEST(PostgreSQLIndex, Lock)
{
  OrthancDatabases::PostgreSQLParameters noLock = globalParameters_;
  noLock.SetLock(false);

  OrthancDatabases::PostgreSQLParameters lock = globalParameters_;
  lock.SetLock(true);

  OrthancDatabases::PostgreSQLIndex db1(noLock);
  db1.SetClearAll(true);
  db1.Open();

  {
    OrthancDatabases::PostgreSQLIndex db2(lock);
    db2.Open();

    OrthancDatabases::PostgreSQLIndex db3(lock);
    ASSERT_THROW(db3.Open(), Orthanc::OrthancException);
  }

  OrthancDatabases::PostgreSQLIndex db4(lock);
  db4.Open();
}


int main(int argc, char **argv)
{
  if (argc < 6)
  {
    std::cerr << "Usage: " << argv[0] << " <host> <port> <username> <password> <database>"
              << std::endl << std::endl
              << "Example: " << argv[0] << " localhost 5432 postgres postgres orthanctest"
              << std::endl << std::endl;
    return -1;
  }

  globalParameters_.SetHost(argv[1]);
  globalParameters_.SetPortNumber(boost::lexical_cast<uint16_t>(argv[2]));
  globalParameters_.SetUsername(argv[3]);
  globalParameters_.SetPassword(argv[4]);
  globalParameters_.SetDatabase(argv[5]);

  ::testing::InitGoogleTest(&argc, argv);
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);
  Orthanc::Logging::EnableTraceLevel(true);

  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  return result;
}
