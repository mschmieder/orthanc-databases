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


#include "../../Framework/SQLite/SQLiteDatabase.h"
#include "../Plugins/SQLiteIndex.h"

#include <Core/Logging.h>
#include <Core/SystemToolbox.h>

#include <gtest/gtest.h>


#include "../../Framework/Plugins/IndexUnitTests.h"


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


TEST(SQLite, ImplicitTransaction)
{
  OrthancDatabases::SQLiteDatabase db;
  db.OpenInMemory();

  ASSERT_FALSE(db.DoesTableExist("test"));
  ASSERT_FALSE(db.DoesTableExist("test2"));

  {
    std::auto_ptr<OrthancDatabases::ITransaction> t(db.CreateTransaction(false));
    ASSERT_FALSE(t->IsImplicit());
  }

  {
    OrthancDatabases::Query query("CREATE TABLE test(id INT)", false);
    std::auto_ptr<OrthancDatabases::IPrecompiledStatement> s(db.Compile(query));
    
    std::auto_ptr<OrthancDatabases::ITransaction> t(db.CreateTransaction(true));
    ASSERT_TRUE(t->IsImplicit());
    ASSERT_THROW(t->Commit(), Orthanc::OrthancException);
    ASSERT_THROW(t->Rollback(), Orthanc::OrthancException);

    OrthancDatabases::Dictionary args;
    t->ExecuteWithoutResult(*s, args);
    ASSERT_THROW(t->Rollback(), Orthanc::OrthancException);
    t->Commit();

    ASSERT_THROW(t->Commit(), Orthanc::OrthancException);
  }

  {
    // An implicit transaction does not need to be explicitely committed
    OrthancDatabases::Query query("CREATE TABLE test2(id INT)", false);
    std::auto_ptr<OrthancDatabases::IPrecompiledStatement> s(db.Compile(query));
    
    std::auto_ptr<OrthancDatabases::ITransaction> t(db.CreateTransaction(true));

    OrthancDatabases::Dictionary args;
    t->ExecuteWithoutResult(*s, args);
  }

  ASSERT_TRUE(db.DoesTableExist("test"));
  ASSERT_TRUE(db.DoesTableExist("test2"));
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
