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


#include "DatabaseManager.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/thread.hpp>

namespace OrthancDatabases
{
  IDatabase& DatabaseManager::GetDatabase()
  {
    static const unsigned int MAX_CONNECTION_ATTEMPTS = 10;   // TODO: Parameter

    unsigned int count = 0;
      
    while (database_.get() == NULL)
    {
      transaction_.reset(NULL);

      try
      {
        database_.reset(factory_->Open());
      }
      catch (Orthanc::OrthancException& e)
      {
        if (e.GetErrorCode() == Orthanc::ErrorCode_DatabaseUnavailable)
        {
          count ++;

          if (count <= MAX_CONNECTION_ATTEMPTS)
          {
            LOG(WARNING) << "Database is currently unavailable, retrying...";
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            continue;
          }
          else
          {
            LOG(ERROR) << "Timeout when connecting to the database, giving up";
          }
        }

        throw;
      }
    }

    if (database_.get() == NULL ||
        database_->GetDialect() != dialect_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return *database_;
    }
  }


  void DatabaseManager::Close()
  {
    LOG(TRACE) << "Closing the connection to the database";

    // Rollback active transaction, if any
    transaction_.reset(NULL);

    // Delete all the cached statements (must occur before closing
    // the database)
    for (CachedStatements::iterator it = cachedStatements_.begin();
         it != cachedStatements_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }

    cachedStatements_.clear();

    // Close the database
    database_.reset(NULL);

    LOG(TRACE) << "Connection to the database is closed";
  }

    
  void DatabaseManager::CloseIfUnavailable(Orthanc::ErrorCode e)
  {
    if (e != Orthanc::ErrorCode_Success)
    {
      transaction_.reset(NULL);
    }

    if (e == Orthanc::ErrorCode_DatabaseUnavailable)
    {
      LOG(ERROR) << "The database is not available, closing the connection";
      Close();
    }
  }


  IPrecompiledStatement* DatabaseManager::LookupCachedStatement(const StatementLocation& location) const
  {
    CachedStatements::const_iterator found = cachedStatements_.find(location);

    if (found == cachedStatements_.end())
    {
      return NULL;
    }
    else
    {
      assert(found->second != NULL);
      return found->second;
    }
  }

    
  IPrecompiledStatement& DatabaseManager::CacheStatement(const StatementLocation& location,
                                                         const Query& query)
  {
    LOG(TRACE) << "Caching statement from " << location.GetFile() << ":" << location.GetLine();
      
    std::auto_ptr<IPrecompiledStatement> statement(GetDatabase().Compile(query));
      
    IPrecompiledStatement* tmp = statement.get();
    if (tmp == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    assert(cachedStatements_.find(location) == cachedStatements_.end());
    cachedStatements_[location] = statement.release();

    return *tmp;
  }

    
  ITransaction& DatabaseManager::GetTransaction()
  {
    if (transaction_.get() == NULL)
    {
      LOG(TRACE) << "Automatically creating an implicit database transaction";

      try
      {
        transaction_.reset(GetDatabase().CreateTransaction(true));
      }
      catch (Orthanc::OrthancException& e)
      {
        CloseIfUnavailable(e.GetErrorCode());
        throw;
      }
    }

    assert(transaction_.get() != NULL);
    return *transaction_;
  }


  void DatabaseManager::ReleaseImplicitTransaction()
  {
    if (transaction_.get() != NULL &&
        transaction_->IsImplicit())
    {
      LOG(TRACE) << "Committing an implicit database transaction";

      try
      {
        transaction_->Commit();
        transaction_.reset(NULL);
      }
      catch (Orthanc::OrthancException& e)
      {
        // Don't throw the exception, as we are in CachedStatement destructor
        LOG(ERROR) << "Error while committing an implicit database transaction: " << e.What();
      }
    }
  }

    
  DatabaseManager::DatabaseManager(IDatabaseFactory* factory) :  // Takes ownership
    factory_(factory)
  {
    if (factory == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    dialect_ = factory->GetDialect();
  }

  
  void DatabaseManager::StartTransaction()
  {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    try
    {
      if (transaction_.get() != NULL)
      {
        LOG(ERROR) << "Cannot start another transaction while there is an uncommitted transaction";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }

      transaction_.reset(GetDatabase().CreateTransaction(false));
    }
    catch (Orthanc::OrthancException& e)
    {
      CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }
  

  void DatabaseManager::CommitTransaction()
  {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    if (transaction_.get() == NULL)
    {
      LOG(ERROR) << "Cannot commit a non-existing transaction";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      try
      {
        transaction_->Commit();
        transaction_.reset(NULL);
      }
      catch (Orthanc::OrthancException& e)
      {
        CloseIfUnavailable(e.GetErrorCode());
        throw;
      }
    }
  }


  void DatabaseManager::RollbackTransaction()
  {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    if (transaction_.get() == NULL)
    {
      LOG(ERROR) << "Cannot rollback a non-existing transaction";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      try
      {
        transaction_->Rollback();
        transaction_.reset(NULL);
      }
      catch (Orthanc::OrthancException& e)
      {
        CloseIfUnavailable(e.GetErrorCode());
        throw;
      }
    }
  }


  IResult& DatabaseManager::CachedStatement::GetResult() const
  {
    if (result_.get() == NULL)
    {
      LOG(ERROR) << "Accessing the results of a statement without having executed it";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return *result_;
  }


  void DatabaseManager::CachedStatement::Setup(const char* sql)
  {
    statement_ = manager_.LookupCachedStatement(location_);

    if (statement_ == NULL)
    {
      query_.reset(new Query(sql));
    }
    else
    {
      LOG(TRACE) << "Reusing cached statement from "
                 << location_.GetFile() << ":" << location_.GetLine();
    }
  }


  DatabaseManager::Transaction::Transaction(DatabaseManager& manager) :
    lock_(manager.mutex_),
    manager_(manager),
    database_(manager.GetDatabase()),
    committed_(false)
  {
    manager_.StartTransaction();
  }


  DatabaseManager::Transaction::~Transaction()
  {
    if (!committed_)
    {
      try
      {
        manager_.RollbackTransaction();
      }
      catch (Orthanc::OrthancException& e)
      {
        // Don't rethrow the exception as we are in a destructor
        LOG(ERROR) << "Uncatched error during some transaction rollback: " << e.What();
      }
    }
  }

  
  void DatabaseManager::Transaction::Commit()
  {
    if (committed_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      manager_.CommitTransaction();
      committed_ = true;
    }
  }

  
  DatabaseManager::CachedStatement::CachedStatement(const StatementLocation& location,
                                                    DatabaseManager& manager,
                                                    const char* sql) :
    lock_(manager.mutex_),
    manager_(manager),
    database_(manager_.GetDatabase()),
    location_(location),
    transaction_(manager_.GetTransaction())
  {
    Setup(sql);
  }

      
  DatabaseManager::CachedStatement::CachedStatement(const StatementLocation& location,
                                                    Transaction& transaction,
                                                    const char* sql) :
    manager_(transaction.GetManager()),
    lock_(manager_.mutex_),
    database_(manager_.GetDatabase()),
    location_(location),
    transaction_(manager_.GetTransaction())
  {
    Setup(sql);
  }


  DatabaseManager::CachedStatement::~CachedStatement()
  {
    manager_.ReleaseImplicitTransaction();
  }
  
      
  void DatabaseManager::CachedStatement::SetReadOnly(bool readOnly)
  {
    if (query_.get() != NULL)
    {
      query_->SetReadOnly(readOnly);
    }
  }


  void DatabaseManager::CachedStatement::SetParameterType(const std::string& parameter,
                                                          ValueType type)
  {
    if (query_.get() != NULL)
    {
      query_->SetType(parameter, type);
    }
  }
      
      
  void DatabaseManager::CachedStatement::Execute()
  {
    Dictionary parameters;
    Execute(parameters);
  }


  void DatabaseManager::CachedStatement::Execute(const Dictionary& parameters)
  {
    if (result_.get() != NULL)
    {
      LOG(ERROR) << "Cannot execute twice a statement";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    try
    {
      if (query_.get() != NULL)
      {
        // Register the newly-created statement
        assert(statement_ == NULL);
        statement_ = &manager_.CacheStatement(location_, *query_);
        query_.reset(NULL);
      }
        
      assert(statement_ != NULL);
      result_.reset(transaction_.Execute(*statement_, parameters));
    }
    catch (Orthanc::OrthancException& e)
    {
      manager_.CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }


  bool DatabaseManager::CachedStatement::IsDone() const
  {
    try
    {
      return GetResult().IsDone();
    }
    catch (Orthanc::OrthancException& e)
    {
      manager_.CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }


  void DatabaseManager::CachedStatement::Next()
  {
    try
    {
      GetResult().Next();
    }
    catch (Orthanc::OrthancException& e)
    {
      manager_.CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }


  size_t DatabaseManager::CachedStatement::GetResultFieldsCount() const
  {
    try
    {
      return GetResult().GetFieldsCount();
    }
    catch (Orthanc::OrthancException& e)
    {
      manager_.CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }


  void DatabaseManager::CachedStatement::SetResultFieldType(size_t field,
                                                            ValueType type)
  {
    try
    {
      if (!GetResult().IsDone())
      {
        GetResult().SetExpectedType(field, type);
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      manager_.CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }


  const IValue& DatabaseManager::CachedStatement::GetResultField(size_t index) const
  {
    try
    {
      return GetResult().GetField(index);
    }
    catch (Orthanc::OrthancException& e)
    {
      manager_.CloseIfUnavailable(e.GetErrorCode());
      throw;
    }
  }
}
