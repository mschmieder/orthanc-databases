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


#include "GlobalProperties.h"

#include "../Common/Utf8StringValue.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

namespace OrthancDatabases
{
  bool LookupGlobalProperty(std::string& target,
                            IDatabase& db,
                            ITransaction& transaction,
                            Orthanc::GlobalProperty property)
  {
    Query query("SELECT value FROM GlobalProperties WHERE property=${property}", true);
    query.SetType("property", ValueType_Integer64);

    std::auto_ptr<IPrecompiledStatement> statement(db.Compile(query));

    Dictionary args;
    args.SetIntegerValue("property", property);

    std::auto_ptr<IResult> result(transaction.Execute(*statement, args));

    if (result->IsDone())
    {
      return false;
    }

    result->SetExpectedType(0, ValueType_Utf8String);

    ValueType type = result->GetField(0).GetType();

    if (type == ValueType_Null)
    {
      return false;
    }
    else if (type != ValueType_Utf8String)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
    else
    {
      target = dynamic_cast<const Utf8StringValue&>(result->GetField(0)).GetContent();
      return true;
    }
  }


  bool LookupGlobalProperty(std::string& target /* out */,
                            DatabaseManager& manager,
                            Orthanc::GlobalProperty property)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager,
      "SELECT value FROM GlobalProperties WHERE property=${property}");

    statement.SetReadOnly(true);
    statement.SetParameterType("property", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("property", property);

    statement.Execute(args);
    statement.SetResultFieldType(0, ValueType_Utf8String);

    if (statement.IsDone())
    {
      return false;
    }

    ValueType type = statement.GetResultField(0).GetType();

    if (type == ValueType_Null)
    {
      return false;
    }
    else if (type != ValueType_Utf8String)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
    else
    {
      target = dynamic_cast<const Utf8StringValue&>(statement.GetResultField(0)).GetContent();
      return true;
    }
  }


  void SetGlobalProperty(IDatabase& db,
                         ITransaction& transaction,
                         Orthanc::GlobalProperty property,
                         const std::string& utf8)
  {
    if (db.GetDialect() == Dialect_SQLite)
    {
      Query query("INSERT OR REPLACE INTO GlobalProperties VALUES (${property}, ${value})", false);
      query.SetType("property", ValueType_Integer64);
      query.SetType("value", ValueType_Utf8String);
      
      std::auto_ptr<IPrecompiledStatement> statement(db.Compile(query));

      Dictionary args;
      args.SetIntegerValue("property", static_cast<int>(property));
      args.SetUtf8Value("value", utf8);
        
      transaction.ExecuteWithoutResult(*statement, args);
    }
    else
    {
      {
        Query query("DELETE FROM GlobalProperties WHERE property=${property}", false);
        query.SetType("property", ValueType_Integer64);
      
        std::auto_ptr<IPrecompiledStatement> statement(db.Compile(query));

        Dictionary args;
        args.SetIntegerValue("property", static_cast<int>(property));
        
        transaction.ExecuteWithoutResult(*statement, args);
      }

      {
        Query query("INSERT INTO GlobalProperties VALUES (${property}, ${value})", false);
        query.SetType("property", ValueType_Integer64);
        query.SetType("value", ValueType_Utf8String);
      
        std::auto_ptr<IPrecompiledStatement> statement(db.Compile(query));

        Dictionary args;
        args.SetIntegerValue("property", static_cast<int>(property));
        args.SetUtf8Value("value", utf8);
        
        transaction.ExecuteWithoutResult(*statement, args);
      }
    }
  }


  void SetGlobalProperty(DatabaseManager& manager,
                         Orthanc::GlobalProperty property,
                         const std::string& utf8)
  {
    if (manager.GetDialect() == Dialect_SQLite)
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager,
        "INSERT OR REPLACE INTO GlobalProperties VALUES (${property}, ${value})");
        
      statement.SetParameterType("property", ValueType_Integer64);
      statement.SetParameterType("value", ValueType_Utf8String);
        
      Dictionary args;
      args.SetIntegerValue("property", static_cast<int>(property));
      args.SetUtf8Value("value", utf8);
        
      statement.Execute(args);
    }
    else
    {
      {
        DatabaseManager::CachedStatement statement(
          STATEMENT_FROM_HERE, manager,
          "DELETE FROM GlobalProperties WHERE property=${property}");
        
        statement.SetParameterType("property", ValueType_Integer64);
        
        Dictionary args;
        args.SetIntegerValue("property", property);
        
        statement.Execute(args);
      }

      {
        DatabaseManager::CachedStatement statement(
          STATEMENT_FROM_HERE, manager,
          "INSERT INTO GlobalProperties VALUES (${property}, ${value})");
        
        statement.SetParameterType("property", ValueType_Integer64);
        statement.SetParameterType("value", ValueType_Utf8String);
        
        Dictionary args;
        args.SetIntegerValue("property", static_cast<int>(property));
        args.SetUtf8Value("value", utf8);
        
        statement.Execute(args);
      }
    }
  }


  bool LookupGlobalIntegerProperty(int& target,
                                   IDatabase& db,
                                   ITransaction& transaction,
                                   Orthanc::GlobalProperty property)
  {
    std::string value;

    if (LookupGlobalProperty(value, db, transaction, property))
    {
      try
      {
        target = boost::lexical_cast<int>(value);
        return true;
      }
      catch (boost::bad_lexical_cast&)
      {
        LOG(ERROR) << "Corrupted PostgreSQL database";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
      }      
    }
    else
    {
      return false;
    }
  }


  void SetGlobalIntegerProperty(IDatabase& db,
                                ITransaction& transaction,
                                Orthanc::GlobalProperty property,
                                int value)
  {
    SetGlobalProperty(db, transaction, property, boost::lexical_cast<std::string>(value));
  }
}
