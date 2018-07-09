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


#pragma once

#include "IDatabaseFactory.h"
#include "StatementLocation.h"

#include <Core/Enumerations.h>

#include <boost/thread/recursive_mutex.hpp>
#include <memory>

namespace OrthancDatabases
{
  class DatabaseManager : public boost::noncopyable
  {
  private:
    typedef std::map<StatementLocation, IPrecompiledStatement*>  CachedStatements;

    boost::recursive_mutex           mutex_;
    std::auto_ptr<IDatabaseFactory>  factory_;
    std::auto_ptr<IDatabase>         database_;
    std::auto_ptr<ITransaction>      transaction_;
    CachedStatements                 cachedStatements_;
    Dialect                          dialect_;

    IDatabase& GetDatabase();

    void CloseIfUnavailable(Orthanc::ErrorCode e);

    IPrecompiledStatement* LookupCachedStatement(const StatementLocation& location) const;

    IPrecompiledStatement& CacheStatement(const StatementLocation& location,
                                          const Query& query);

    ITransaction& GetTransaction();
    
  public:
    DatabaseManager(IDatabaseFactory* factory);  // Takes ownership
    
    ~DatabaseManager()
    {
      Close();
    }

    Dialect GetDialect() const
    {
      return dialect_;
    }

    void Open()
    {
      GetDatabase();
    }

    void Close();
    
    void StartTransaction();

    void CommitTransaction();
    
    void RollbackTransaction();

    class CachedStatement : public boost::noncopyable
    {
    private:
      boost::recursive_mutex::scoped_lock  lock_;
      DatabaseManager&                     manager_;
      StatementLocation                    location_;
      IDatabase&                           database_;
      ITransaction&                        transaction_;
      IPrecompiledStatement*               statement_;
      std::auto_ptr<Query>                 query_;
      std::auto_ptr<IResult>               result_;

      IResult& GetResult() const;

    public:
      CachedStatement(const StatementLocation& location,
                      DatabaseManager& manager,
                      const char* sql);

      void SetReadOnly(bool readOnly);

      void SetParameterType(const std::string& parameter,
                            ValueType type);
      
      void Execute();

      void Execute(const Dictionary& parameters);

      IDatabase& GetDatabase()
      {
        return database_;
      }
      
      bool IsDone() const;
      
      void Next();

      size_t GetResultFieldsCount() const;

      void SetResultFieldType(size_t field,
                              ValueType type);
      
      const IValue& GetResultField(size_t index) const;
    };
  };
}
