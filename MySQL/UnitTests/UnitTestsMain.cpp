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


#include "../Plugins/MySQLIndex.h"

OrthancDatabases::MySQLParameters globalParameters_;

#include "../../Framework/Plugins/IndexUnitTests.h"
#include "../../Framework/MySQL/MySQLDatabase.h"

#include <Core/Logging.h>

#include <gtest/gtest.h>


TEST(MySQLIndex, Lock)
{
  OrthancDatabases::MySQLParameters noLock = globalParameters_;
  noLock.SetLock(false);

  OrthancDatabases::MySQLParameters lock = globalParameters_;
  lock.SetLock(true);

  OrthancDatabases::MySQLIndex db1(noLock);
  db1.SetClearAll(true);
  db1.Open();

  {
    OrthancDatabases::MySQLIndex db2(lock);
    db2.Open();

    OrthancDatabases::MySQLIndex db3(lock);
    ASSERT_THROW(db3.Open(), Orthanc::OrthancException);
  }

  OrthancDatabases::MySQLIndex db4(lock);
  db4.Open();
}


int main(int argc, char **argv)
{
  if (argc < 5)
  {
    std::cerr << "Usage: " << argv[0] << " <socket> <username> <password> <database>"
              << std::endl << std::endl
              << "Example: " << argv[0] << " /var/run/mysqld/mysqld.sock root root orthanctest"
              << std::endl << std::endl;
    return -1;
  }

  globalParameters_.SetUnixSocket(argv[1]);
  globalParameters_.SetUsername(argv[2]);
  globalParameters_.SetPassword(argv[3]);
  globalParameters_.SetDatabase(argv[4]);

  ::testing::InitGoogleTest(&argc, argv);
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);
  Orthanc::Logging::EnableTraceLevel(true);

  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  OrthancDatabases::MySQLDatabase::GlobalFinalization();

  return result;
}
