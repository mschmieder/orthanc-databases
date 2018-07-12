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


#include <gtest/gtest.h>

#if defined(_WIN32)
// Fix redefinition of symbols on MinGW (these symbols are manually
// defined both by PostgreSQL and Google Test)
#  undef S_IRGRP
#  undef S_IROTH
#  undef S_IRWXG
#  undef S_IRWXO
#  undef S_IWGRP
#  undef S_IWOTH
#  undef S_IXGRP
#  undef S_IXOTH
#endif

#include "../Plugins/PostgreSQLStorageArea.h"
#include "../../Framework/PostgreSQL/PostgreSQLTransaction.h"
#include "../../Framework/PostgreSQL/PostgreSQLResult.h"
#include "../../Framework/PostgreSQL/PostgreSQLLargeObject.h"

#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

using namespace OrthancDatabases;

extern PostgreSQLParameters  globalParameters_;


static PostgreSQLDatabase* CreateTestDatabase()
{
  std::auto_ptr<PostgreSQLDatabase> pg
    (new PostgreSQLDatabase(globalParameters_));

  pg->Open();
  pg->ClearAll();

  return pg.release();
}


static int64_t CountLargeObjects(PostgreSQLDatabase& db)
{
  // Count the number of large objects in the DB
  PostgreSQLStatement s(db, "SELECT COUNT(*) FROM pg_catalog.pg_largeobject", true);
  PostgreSQLResult r(s);
  return r.GetInteger64(0);
}


TEST(PostgreSQL, Basic)
{
  std::auto_ptr<PostgreSQLDatabase> pg(CreateTestDatabase());

  ASSERT_FALSE(pg->DoesTableExist("Test"));
  pg->Execute("CREATE TABLE Test(name INTEGER, value BIGINT)");
  ASSERT_TRUE(pg->DoesTableExist("Test"));

  PostgreSQLStatement s(*pg, "INSERT INTO Test VALUES ($1,$2)", false);
  s.DeclareInputInteger(0);
  s.DeclareInputInteger64(1);

  s.BindInteger(0, 43);
  s.BindNull(0);
  s.BindInteger(0, 42);
  s.BindInteger64(1, -4242);
  s.Run();

  s.BindInteger(0, 43);
  s.BindNull(1);
  s.Run();

  s.BindNull(0);
  s.BindInteger64(1, 4444);
  s.Run();

  {
    PostgreSQLStatement t(*pg, "SELECT name, value FROM Test ORDER BY name", true);
    PostgreSQLResult r(t);

    ASSERT_FALSE(r.IsDone());
    ASSERT_FALSE(r.IsNull(0)); ASSERT_EQ(42, r.GetInteger(0));
    ASSERT_FALSE(r.IsNull(1)); ASSERT_EQ(-4242, r.GetInteger64(1));

    r.Next();
    ASSERT_FALSE(r.IsDone());
    ASSERT_FALSE(r.IsNull(0)); ASSERT_EQ(43, r.GetInteger(0));
    ASSERT_TRUE(r.IsNull(1));

    r.Next();
    ASSERT_FALSE(r.IsDone());
    ASSERT_TRUE(r.IsNull(0));
    ASSERT_FALSE(r.IsNull(1)); ASSERT_EQ(4444, r.GetInteger64(1));

    r.Next();
    ASSERT_TRUE(r.IsDone());
  }

  {
    PostgreSQLStatement t(*pg, "SELECT name, value FROM Test WHERE name=$1", true);
    t.DeclareInputInteger(0);

    {
      t.BindInteger(0, 42);
      PostgreSQLResult r(t);
      ASSERT_FALSE(r.IsDone());
      ASSERT_FALSE(r.IsNull(0)); ASSERT_EQ(42, r.GetInteger(0));
      ASSERT_FALSE(r.IsNull(1)); ASSERT_EQ(-4242, r.GetInteger64(1));

      r.Next();
      ASSERT_TRUE(r.IsDone());
    }

    {
      t.BindInteger(0, 40);
      PostgreSQLResult r(t);
      ASSERT_TRUE(r.IsDone());
    }
  }
  
}


TEST(PostgreSQL, String)
{
  std::auto_ptr<PostgreSQLDatabase> pg(CreateTestDatabase());

  pg->Execute("CREATE TABLE Test(name INTEGER, value VARCHAR(40))");

  PostgreSQLStatement s(*pg, "INSERT INTO Test VALUES ($1,$2)", false);
  s.DeclareInputInteger(0);
  s.DeclareInputString(1);

  s.BindInteger(0, 42);
  s.BindString(1, "Hello");
  s.Run();

  s.BindInteger(0, 43);
  s.BindNull(1);
  s.Run();

  s.BindNull(0);
  s.BindString(1, "");
  s.Run();

  {
    PostgreSQLStatement t(*pg, "SELECT name, value FROM Test ORDER BY name", true);
    PostgreSQLResult r(t);

    ASSERT_FALSE(r.IsDone());
    ASSERT_FALSE(r.IsNull(0)); ASSERT_EQ(42, r.GetInteger(0));
    ASSERT_FALSE(r.IsNull(1)); ASSERT_EQ("Hello", r.GetString(1));

    r.Next();
    ASSERT_FALSE(r.IsDone());
    ASSERT_FALSE(r.IsNull(0)); ASSERT_EQ(43, r.GetInteger(0));
    ASSERT_TRUE(r.IsNull(1));

    r.Next();
    ASSERT_FALSE(r.IsDone());
    ASSERT_TRUE(r.IsNull(0));
    ASSERT_FALSE(r.IsNull(1)); ASSERT_EQ("", r.GetString(1));

    r.Next();
    ASSERT_TRUE(r.IsDone());
  }
}


TEST(PostgreSQL, Transaction)
{
  std::auto_ptr<PostgreSQLDatabase> pg(CreateTestDatabase());

  pg->Execute("CREATE TABLE Test(name INTEGER, value INTEGER)");

  {
    PostgreSQLStatement s(*pg, "INSERT INTO Test VALUES ($1,$2)", false);
    s.DeclareInputInteger(0);
    s.DeclareInputInteger(1);
    s.BindInteger(0, 42);
    s.BindInteger(1, 4242);
    s.Run();

    {
      PostgreSQLTransaction t(*pg);
      s.BindInteger(0, 43);
      s.BindInteger(1, 4343);
      s.Run();
      s.BindInteger(0, 44);
      s.BindInteger(1, 4444);
      s.Run();

      PostgreSQLStatement u(*pg, "SELECT COUNT(*) FROM Test", true);
      PostgreSQLResult r(u);
      ASSERT_EQ(3, r.GetInteger64(0));

      // No commit
    }

    {
      PostgreSQLStatement u(*pg, "SELECT COUNT(*) FROM Test", true);
      PostgreSQLResult r(u);
      ASSERT_EQ(1, r.GetInteger64(0));  // Just "1" because of implicit rollback
    }
    
    {
      PostgreSQLTransaction t(*pg);
      s.BindInteger(0, 43);
      s.BindInteger(1, 4343);
      s.Run();
      s.BindInteger(0, 44);
      s.BindInteger(1, 4444);
      s.Run();

      {
        PostgreSQLStatement u(*pg, "SELECT COUNT(*) FROM Test", true);
        PostgreSQLResult r(u);
        ASSERT_EQ(3, r.GetInteger64(0));

        t.Commit();
        ASSERT_THROW(t.Rollback(), Orthanc::OrthancException);
        ASSERT_THROW(t.Commit(), Orthanc::OrthancException);
      }
    }

    {
      PostgreSQLStatement u(*pg, "SELECT COUNT(*) FROM Test", true);
      PostgreSQLResult r(u);
      ASSERT_EQ(3, r.GetInteger64(0));
    }
  }
}





TEST(PostgreSQL, LargeObject)
{
  std::auto_ptr<PostgreSQLDatabase> pg(CreateTestDatabase());
  ASSERT_EQ(0, CountLargeObjects(*pg));

  pg->Execute("CREATE TABLE Test(name VARCHAR, value OID)");

  // Automatically remove the large objects associated with the table
  pg->Execute("CREATE RULE TestDelete AS ON DELETE TO Test DO SELECT lo_unlink(old.value);");

  {
    PostgreSQLStatement s(*pg, "INSERT INTO Test VALUES ($1,$2)", false);
    s.DeclareInputString(0);
    s.DeclareInputLargeObject(1);
    
    for (int i = 0; i < 10; i++)
    {
      PostgreSQLTransaction t(*pg);

      std::string value = "Value " + boost::lexical_cast<std::string>(i * 2);
      PostgreSQLLargeObject obj(*pg, value);

      s.BindString(0, "Index " + boost::lexical_cast<std::string>(i));
      s.BindLargeObject(1, obj);
      s.Run();

      std::string tmp;
      PostgreSQLLargeObject::Read(tmp, *pg, obj.GetOid());
      ASSERT_EQ(value, tmp);

      t.Commit();
    }
  }


  ASSERT_EQ(10, CountLargeObjects(*pg));

  {
    PostgreSQLTransaction t(*pg);
    PostgreSQLStatement s(*pg, "SELECT * FROM Test ORDER BY name DESC", true);
    PostgreSQLResult r(s);

    ASSERT_FALSE(r.IsDone());

    ASSERT_FALSE(r.IsNull(0));
    ASSERT_EQ("Index 9", r.GetString(0));

    std::string data;
    r.GetLargeObject(data, 1);
    ASSERT_EQ("Value 18", data);    

    r.Next();
    ASSERT_FALSE(r.IsDone());

    //ASSERT_TRUE(r.IsString(0));
  }


  {
    PostgreSQLTransaction t(*pg);
    PostgreSQLStatement s(*pg, "DELETE FROM Test WHERE name='Index 9'", false);
    s.Run();
    t.Commit();
  }


  {
    // Count the number of items in the DB
    PostgreSQLTransaction t(*pg);
    PostgreSQLStatement s(*pg, "SELECT COUNT(*) FROM Test", true);
    PostgreSQLResult r(s);
    ASSERT_EQ(9, r.GetInteger64(0));
  }

  ASSERT_EQ(9, CountLargeObjects(*pg));
}


TEST(PostgreSQL, StorageArea)
{
  PostgreSQLStorageArea storageArea(globalParameters_);
  storageArea.SetClearAll(true);

  {
    DatabaseManager::Transaction transaction(storageArea.GetManager());
    PostgreSQLDatabase& db = 
      dynamic_cast<PostgreSQLDatabase&>(transaction.GetDatabase());

    ASSERT_EQ(0, CountLargeObjects(db));
  
    for (int i = 0; i < 10; i++)
    {
      std::string uuid = boost::lexical_cast<std::string>(i);
      std::string value = "Value " + boost::lexical_cast<std::string>(i * 2);
      storageArea.Create(transaction, uuid, value.c_str(), value.size(), OrthancPluginContentType_Unknown);
    }

    std::string tmp;
    ASSERT_THROW(storageArea.ReadToString(tmp, transaction, "nope", OrthancPluginContentType_Unknown), 
                 Orthanc::OrthancException);
  
    ASSERT_EQ(10, CountLargeObjects(db));
    storageArea.Remove(transaction, "5", OrthancPluginContentType_Unknown);

    ASSERT_EQ(9, CountLargeObjects(db));

    for (int i = 0; i < 10; i++)
    {
      std::string uuid = boost::lexical_cast<std::string>(i);
      std::string expected = "Value " + boost::lexical_cast<std::string>(i * 2);
      std::string content;

      if (i == 5)
      {
        ASSERT_THROW(storageArea.ReadToString(content, transaction, uuid, OrthancPluginContentType_Unknown), 
                     Orthanc::OrthancException);
      }
      else
      {
        storageArea.ReadToString(content, transaction, uuid, OrthancPluginContentType_Unknown);
        ASSERT_EQ(expected, content);
      }
    }

    for (int i = 0; i < 10; i++)
    {
      storageArea.Remove(transaction, boost::lexical_cast<std::string>(i),
                         OrthancPluginContentType_Unknown);
    }

    ASSERT_EQ(0, CountLargeObjects(db));

    transaction.Commit();
  }
}


TEST(PostgreSQL, ImplicitTransaction)
{
  std::auto_ptr<PostgreSQLDatabase> db(CreateTestDatabase());

  ASSERT_FALSE(db->DoesTableExist("test"));
  ASSERT_FALSE(db->DoesTableExist("test2"));

  {
    std::auto_ptr<OrthancDatabases::ITransaction> t(db->CreateTransaction(false));
    ASSERT_FALSE(t->IsImplicit());
  }

  {
    Query query("CREATE TABLE test(id INT)", false);
    std::auto_ptr<IPrecompiledStatement> s(db->Compile(query));
    
    std::auto_ptr<ITransaction> t(db->CreateTransaction(true));
    ASSERT_TRUE(t->IsImplicit());
    ASSERT_THROW(t->Commit(), Orthanc::OrthancException);
    ASSERT_THROW(t->Rollback(), Orthanc::OrthancException);

    Dictionary args;
    t->ExecuteWithoutResult(*s, args);
    ASSERT_THROW(t->Rollback(), Orthanc::OrthancException);
    t->Commit();

    ASSERT_THROW(t->Commit(), Orthanc::OrthancException);
  }

  {
    // An implicit transaction does not need to be explicitely committed
    Query query("CREATE TABLE test2(id INT)", false);
    std::auto_ptr<IPrecompiledStatement> s(db->Compile(query));
    
    std::auto_ptr<ITransaction> t(db->CreateTransaction(true));

    Dictionary args;
    t->ExecuteWithoutResult(*s, args);
  }

  ASSERT_TRUE(db->DoesTableExist("test"));
  ASSERT_TRUE(db->DoesTableExist("test2"));
}

