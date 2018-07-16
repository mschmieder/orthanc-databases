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
#include "../Plugins/MySQLStorageArea.h"

OrthancDatabases::MySQLParameters globalParameters_;

#include "../../Framework/Common/Integer64Value.h"
#include "../../Framework/MySQL/MySQLDatabase.h"
#include "../../Framework/MySQL/MySQLResult.h"
#include "../../Framework/MySQL/MySQLStatement.h"
#include "../../Framework/MySQL/MySQLTransaction.h"
#include "../../Framework/Plugins/IndexUnitTests.h"

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


static int64_t CountFiles(OrthancDatabases::MySQLDatabase& db)
{
  OrthancDatabases::Query query("SELECT COUNT(*) FROM StorageArea", true);
  OrthancDatabases::MySQLStatement s(db, query);
  OrthancDatabases::MySQLTransaction t(db);
  OrthancDatabases::Dictionary d;
  std::auto_ptr<OrthancDatabases::IResult> result(s.Execute(t, d));
  return dynamic_cast<const OrthancDatabases::Integer64Value&>(result->GetField(0)).GetValue();
}


TEST(MySQL, StorageArea)
{
  OrthancDatabases::MySQLStorageArea storageArea(globalParameters_);
  storageArea.SetClearAll(true);

  {
    OrthancDatabases::DatabaseManager::Transaction transaction(storageArea.GetManager());
    OrthancDatabases::MySQLDatabase& db = 
      dynamic_cast<OrthancDatabases::MySQLDatabase&>(transaction.GetDatabase());

    ASSERT_EQ(0, CountFiles(db));
  
    for (int i = 0; i < 10; i++)
    {
      std::string uuid = boost::lexical_cast<std::string>(i);
      std::string value = "Value " + boost::lexical_cast<std::string>(i * 2);
      storageArea.Create(transaction, uuid, value.c_str(), value.size(), OrthancPluginContentType_Unknown);
    }

    std::string tmp;
    ASSERT_THROW(storageArea.ReadToString(tmp, transaction, "nope", OrthancPluginContentType_Unknown), 
                 Orthanc::OrthancException);
  
    ASSERT_EQ(10, CountFiles(db));
    storageArea.Remove(transaction, "5", OrthancPluginContentType_Unknown);

    ASSERT_EQ(9, CountFiles(db));

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

    ASSERT_EQ(0, CountFiles(db));

    transaction.Commit();
  }
}


TEST(MySQL, ImplicitTransaction)
{
  OrthancDatabases::MySQLDatabase::ClearDatabase(globalParameters_);  
  OrthancDatabases::MySQLDatabase db(globalParameters_);
  db.Open();

  {
    OrthancDatabases::MySQLTransaction t(db);
    ASSERT_FALSE(db.DoesTableExist(t, "test"));
    ASSERT_FALSE(db.DoesTableExist(t, "test2"));
  }

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

  {
    OrthancDatabases::MySQLTransaction t(db);
    ASSERT_TRUE(db.DoesTableExist(t, "test"));
    ASSERT_TRUE(db.DoesTableExist(t, "test2"));
  }
}


int main(int argc, char **argv)
{
  if (argc < 5)
  {
    std::cerr << "Usage (UNIX):    " << argv[0] << " <socket> <username> <password> <database>"
              << std::endl
              << "Usage (Windows): " << argv[0] << " <host> <port> <username> <password> <database>"
              << std::endl << std::endl
              << "Example (UNIX):    " << argv[0] << " /var/run/mysqld/mysqld.sock root root orthanctest"
              << std::endl
              << "Example (Windows): " << argv[0] << " localhost 3306 root root orthanctest"
              << std::endl << std::endl;
    return -1;
  }

  std::vector<std::string> args;
  for (int i = 1; i < argc; i++)
  {
    // Ignore arguments beginning with "-" to allow passing arguments
    // to Google Test such as "--gtest_filter="
    if (argv[i] != NULL &&
        argv[i][0] != '-')
    {
      args.push_back(std::string(argv[i]));
    }
  }
  
  ::testing::InitGoogleTest(&argc, argv);
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);
  Orthanc::Logging::EnableTraceLevel(true);
  
  if (args.size() == 4)
  {
    // UNIX flavor
    globalParameters_.SetUnixSocket(args[0]);
    globalParameters_.SetUsername(args[1]);
    globalParameters_.SetPassword(args[2]);
    globalParameters_.SetDatabase(args[3]);
  }
  else if (args.size() == 5)
  {
    // Windows flavor
    globalParameters_.SetHost(args[0]);
    globalParameters_.SetPort(boost::lexical_cast<unsigned int>(args[1]));
    globalParameters_.SetUsername(args[2]);
    globalParameters_.SetPassword(args[3]);
    globalParameters_.SetDatabase(args[4]);
  }
  else
  {
    LOG(ERROR) << "Bad number of arguments";
    return -1;
  }

  Json::Value config;
  globalParameters_.Format(config);
  std::cout << "Parameters of the MySQL connection: " << std::endl
            << config.toStyledString() << std::endl;

  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  OrthancDatabases::MySQLDatabase::GlobalFinalization();

  return result;
}
