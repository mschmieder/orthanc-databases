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


#include "../Plugins/SQLiteIndex.h"
#include "../../Framework/Plugins/IndexUnitTests.h"

#include <Core/Logging.h>
#include <Core/SystemToolbox.h>

#include <gtest/gtest.h>



TEST(SQLiteIndex, Lock)
{
  {
    // No locking if using memory backend
    OrthancDatabases::SQLiteIndex db1;
    OrthancDatabases::SQLiteIndex db2;

    db1.Open();
    db2.Open();
  }

  Orthanc::SystemToolbox::RemoveFile("index.db");

  {
    OrthancDatabases::SQLiteIndex db1("index.db");
    OrthancDatabases::SQLiteIndex db2("index.db");

    db1.Open();
    ASSERT_THROW(db2.Open(), Orthanc::OrthancException);
  }

  {
    OrthancDatabases::SQLiteIndex db3("index.db");
    db3.Open();
  }
}



int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);
  Orthanc::Logging::EnableTraceLevel(true);

  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  return result;
}
