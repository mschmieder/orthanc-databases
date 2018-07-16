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


#include "IndexBackend.h"

#include "../Common/BinaryStringValue.h"
#include "../Common/Integer64Value.h"
#include "../Common/Utf8StringValue.h"
#include "GlobalProperties.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <OrthancServer/ServerEnumerations.h>


namespace OrthancDatabases
{
  static std::string ConvertWildcardToLike(const std::string& query)
  {
    std::string s = query;

    for (size_t i = 0; i < s.size(); i++)
    {
      if (s[i] == '*')
      {
        s[i] = '%';
      }
      else if (s[i] == '?')
      {
        s[i] = '_';
      }
    }

    return s;
  }

  
  int64_t IndexBackend::ReadInteger64(const DatabaseManager::CachedStatement& statement,
                                      size_t field)
  {
    if (statement.IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }

    const IValue& value = statement.GetResultField(field);
      
    switch (value.GetType())
    {
      case ValueType_Integer64:
        return dynamic_cast<const Integer64Value&>(value).GetValue();

      default:
        //LOG(ERROR) << value.Format();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  int32_t IndexBackend::ReadInteger32(const DatabaseManager::CachedStatement& statement,
                                      size_t field)
  {
    if (statement.IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }

    int64_t value = ReadInteger64(statement, field);

    if (value != static_cast<int64_t>(static_cast<int32_t>(value)))
    {
      LOG(ERROR) << "Integer overflow";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return static_cast<int32_t>(value);
    }
  }

    
  std::string IndexBackend::ReadString(const DatabaseManager::CachedStatement& statement,
                                       size_t field)
  {
    const IValue& value = statement.GetResultField(field);
      
    switch (value.GetType())
    {
      case ValueType_BinaryString:
        return dynamic_cast<const BinaryStringValue&>(value).GetContent();

      case ValueType_Utf8String:
        return dynamic_cast<const Utf8StringValue&>(value).GetContent();

      default:
        //LOG(ERROR) << value.Format();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }

    
  template <typename T>
  void IndexBackend::ReadListOfIntegers(std::list<T>& target,
                                        DatabaseManager::CachedStatement& statement,
                                        const Dictionary& args)
  {
    statement.Execute(args);
      
    target.clear();

    if (!statement.IsDone())
    {
      if (statement.GetResultFieldsCount() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      
      statement.SetResultFieldType(0, ValueType_Integer64);

      while (!statement.IsDone())
      {
        target.push_back(static_cast<T>(ReadInteger64(statement, 0)));
        statement.Next();
      }
    }
  }

    
  void IndexBackend::ReadListOfStrings(std::list<std::string>& target,
                                       DatabaseManager::CachedStatement& statement,
                                       const Dictionary& args)
  {
    statement.Execute(args);

    target.clear();
      
    if (!statement.IsDone())
    {
      if (statement.GetResultFieldsCount() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      
      while (!statement.IsDone())
      {
        target.push_back(ReadString(statement, 0));
        statement.Next();
      }
    }
  }


  void IndexBackend::ReadChangesInternal(bool& done,
                                         DatabaseManager::CachedStatement& statement,
                                         const Dictionary& args,
                                         uint32_t maxResults)
  {
    statement.Execute(args);

    uint32_t count = 0;

    while (count < maxResults &&
           !statement.IsDone())
    {
      GetOutput().AnswerChange(
        ReadInteger64(statement, 0),
        ReadInteger32(statement, 1),
        static_cast<OrthancPluginResourceType>(ReadInteger32(statement, 3)),
        GetPublicId(ReadInteger64(statement, 2)),
        ReadString(statement, 4));

      statement.Next();
      count++;
    }

    done = (count < maxResults ||
            statement.IsDone());
  }


  void IndexBackend::ReadExportedResourcesInternal(bool& done,
                                                   DatabaseManager::CachedStatement& statement,
                                                   const Dictionary& args,
                                                   uint32_t maxResults)
  {
    statement.Execute(args);

    uint32_t count = 0;

    while (count < maxResults &&
           !statement.IsDone())
    {
      int64_t seq = ReadInteger64(statement, 0);
      OrthancPluginResourceType resourceType =
        static_cast<OrthancPluginResourceType>(ReadInteger32(statement, 1));
      std::string publicId = ReadString(statement, 2);

      GetOutput().AnswerExportedResource(seq, 
                                         resourceType,
                                         publicId,
                                         ReadString(statement, 3),  // modality
                                         ReadString(statement, 8),  // date
                                         ReadString(statement, 4),  // patient ID
                                         ReadString(statement, 5),  // study instance UID
                                         ReadString(statement, 6),  // series instance UID
                                         ReadString(statement, 7)); // sop instance UID

      statement.Next();
      count++;
    }

    done = (count < maxResults ||
            statement.IsDone());
  }


  void IndexBackend::ClearDeletedFiles()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "DELETE FROM DeletedFiles");

    statement.Execute();
  }
    

  void IndexBackend::ClearDeletedResources()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "DELETE FROM DeletedResources");

    statement.Execute();
  }
    

  void IndexBackend::SignalDeletedFiles()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM DeletedFiles");

    statement.SetReadOnly(true);
    statement.Execute();

    while (!statement.IsDone())
    {
      std::string a = ReadString(statement, 0);
      std::string b = ReadString(statement, 5);
      std::string c = ReadString(statement, 6);

      GetOutput().SignalDeletedAttachment(a.c_str(),
                                          ReadInteger32(statement, 1),
                                          ReadInteger64(statement, 3),
                                          b.c_str(),
                                          ReadInteger32(statement, 4),
                                          ReadInteger64(statement, 2),
                                          c.c_str());

      statement.Next();
    }
  }


  void IndexBackend::SignalDeletedResources()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM DeletedResources");

    statement.SetReadOnly(true);
    statement.Execute();

    while (!statement.IsDone())
    {
      GetOutput().SignalDeletedResource(
        ReadString(statement, 1),
        static_cast<OrthancPluginResourceType>(ReadInteger32(statement, 0)));

      statement.Next();
    }
  }


  IndexBackend::IndexBackend(IDatabaseFactory* factory) :
    manager_(factory)
  {
  }

    
  void IndexBackend::AddAttachment(int64_t id,
                                   const OrthancPluginAttachment& attachment)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "INSERT INTO AttachedFiles VALUES(${id}, ${type}, ${uuid}, "
      "${compressed}, ${uncompressed}, ${compression}, ${hash}, ${hash-compressed})");

    statement.SetParameterType("id", ValueType_Integer64);
    statement.SetParameterType("type", ValueType_Integer64);
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("compressed", ValueType_Integer64);
    statement.SetParameterType("uncompressed", ValueType_Integer64);
    statement.SetParameterType("compression", ValueType_Integer64);
    statement.SetParameterType("hash", ValueType_Utf8String);
    statement.SetParameterType("hash-compressed", ValueType_Utf8String);

    Dictionary args;
    args.SetIntegerValue("id", id);
    args.SetIntegerValue("type", attachment.contentType);
    args.SetUtf8Value("uuid", attachment.uuid);
    args.SetIntegerValue("compressed", attachment.compressedSize);
    args.SetIntegerValue("uncompressed", attachment.uncompressedSize);
    args.SetIntegerValue("compression", attachment.compressionType);
    args.SetUtf8Value("hash", attachment.uncompressedHash);
    args.SetUtf8Value("hash-compressed", attachment.compressedHash);
    
    statement.Execute(args);
  }

    
  void IndexBackend::AttachChild(int64_t parent,
                                 int64_t child)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "UPDATE Resources SET parentId = ${parent} WHERE internalId = ${child}");

    statement.SetParameterType("parent", ValueType_Integer64);
    statement.SetParameterType("child", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("parent", parent);
    args.SetIntegerValue("child", child);
    
    statement.Execute(args);
  }

    
  void IndexBackend::ClearChanges()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "DELETE FROM Changes");

    statement.Execute();
  }

    
  void IndexBackend::ClearExportedResources()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "DELETE FROM ExportedResources");

    statement.Execute();
  }

    
  void IndexBackend::DeleteAttachment(int64_t id,
                                      int32_t attachment)
  {
    ClearDeletedFiles();

    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager_,
        "DELETE FROM AttachedFiles WHERE id=${id} AND fileType=${type}");

      statement.SetParameterType("id", ValueType_Integer64);
      statement.SetParameterType("type", ValueType_Integer64);

      Dictionary args;
      args.SetIntegerValue("id", id);
      args.SetIntegerValue("type", static_cast<int>(attachment));
    
      statement.Execute(args);
    }

    SignalDeletedFiles();
  }

    
  void IndexBackend::DeleteMetadata(int64_t id,
                                    int32_t metadataType)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "DELETE FROM Metadata WHERE id=${id} and type=${type}");

    statement.SetParameterType("id", ValueType_Integer64);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);
    args.SetIntegerValue("type", static_cast<int>(metadataType));
    
    statement.Execute(args);
  }

    
  void IndexBackend::DeleteResource(int64_t id)
  {
    assert(manager_.GetDialect() != Dialect_MySQL);
    
    ClearDeletedFiles();
    ClearDeletedResources();
    
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, GetManager(),
        "DELETE FROM RemainingAncestor");

      statement.Execute();
    }
      
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, GetManager(),
        "DELETE FROM Resources WHERE internalId=${id}");

      statement.SetParameterType("id", ValueType_Integer64);

      Dictionary args;
      args.SetIntegerValue("id", id);
    
      statement.Execute(args);
    }


    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, GetManager(),
        "SELECT * FROM RemainingAncestor");

      statement.Execute();

      if (!statement.IsDone())
      {
        GetOutput().SignalRemainingAncestor(
          ReadString(statement, 1),
          static_cast<OrthancPluginResourceType>(ReadInteger32(statement, 0)));
          
        // There is at most 1 remaining ancestor
        assert((statement.Next(), statement.IsDone()));
      }
    }
    
    SignalDeletedFiles();
    SignalDeletedResources();
  }


  void IndexBackend::GetAllInternalIds(std::list<int64_t>& target,
                                       OrthancPluginResourceType resourceType)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT internalId FROM Resources WHERE resourceType=${type}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("type", static_cast<int>(resourceType));

    ReadListOfIntegers<int64_t>(target, statement, args);
  }

    
  void IndexBackend::GetAllPublicIds(std::list<std::string>& target,
                                     OrthancPluginResourceType resourceType)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT publicId FROM Resources WHERE resourceType=${type}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("type", static_cast<int>(resourceType));

    ReadListOfStrings(target, statement, args);
  }

    
  void IndexBackend::GetAllPublicIds(std::list<std::string>& target,
                                     OrthancPluginResourceType resourceType,
                                     uint64_t since,
                                     uint64_t limit)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT publicId FROM (SELECT publicId FROM Resources "
      "WHERE resourceType=${type}) AS tmp "
      "ORDER BY tmp.publicId LIMIT ${limit} OFFSET ${since}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("type", ValueType_Integer64);
    statement.SetParameterType("limit", ValueType_Integer64);
    statement.SetParameterType("since", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("type", static_cast<int>(resourceType));
    args.SetIntegerValue("limit", limit);
    args.SetIntegerValue("since", since);

    ReadListOfStrings(target, statement, args);
  }

    
  /* Use GetOutput().AnswerChange() */
  void IndexBackend::GetChanges(bool& done /*out*/,
                                int64_t since,
                                uint32_t maxResults)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM Changes WHERE seq>${since} ORDER BY seq LIMIT ${limit}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("limit", ValueType_Integer64);
    statement.SetParameterType("since", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("limit", maxResults + 1);
    args.SetIntegerValue("since", since);

    ReadChangesInternal(done, statement, args, maxResults);
  }

    
  void IndexBackend::GetChildrenInternalId(std::list<int64_t>& target /*out*/,
                                           int64_t id)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT a.internalId FROM Resources AS a, Resources AS b  "
      "WHERE a.parentId = b.internalId AND b.internalId = ${id}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    ReadListOfIntegers<int64_t>(target, statement, args);
  }

    
  void IndexBackend::GetChildrenPublicId(std::list<std::string>& target /*out*/,
                                         int64_t id)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT a.publicId FROM Resources AS a, Resources AS b  "
      "WHERE a.parentId = b.internalId AND b.internalId = ${id}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    ReadListOfStrings(target, statement, args);
  }

    
  /* Use GetOutput().AnswerExportedResource() */
  void IndexBackend::GetExportedResources(bool& done /*out*/,
                                          int64_t since,
                                          uint32_t maxResults)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM ExportedResources WHERE seq>${since} ORDER BY seq LIMIT ${limit}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("limit", ValueType_Integer64);
    statement.SetParameterType("since", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("limit", maxResults + 1);
    args.SetIntegerValue("since", since);

    ReadExportedResourcesInternal(done, statement, args, maxResults);
  }

    
  /* Use GetOutput().AnswerChange() */
  void IndexBackend::GetLastChange()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM Changes ORDER BY seq DESC LIMIT 1");

    statement.SetReadOnly(true);
      
    Dictionary args;

    bool done;  // Ignored
    ReadChangesInternal(done, statement, args, 1);
  }

    
  /* Use GetOutput().AnswerExportedResource() */
  void IndexBackend::GetLastExportedResource()
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM ExportedResources ORDER BY seq DESC LIMIT 1");

    statement.SetReadOnly(true);
      
    Dictionary args;

    bool done;  // Ignored
    ReadExportedResourcesInternal(done, statement, args, 1);
  }

    
  /* Use GetOutput().AnswerDicomTag() */
  void IndexBackend::GetMainDicomTags(int64_t id)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM MainDicomTags WHERE id=${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    statement.Execute(args);

    while (!statement.IsDone())
    {
      GetOutput().AnswerDicomTag(static_cast<uint16_t>(ReadInteger64(statement, 1)),
                                 static_cast<uint16_t>(ReadInteger64(statement, 2)),
                                 ReadString(statement, 3));
      statement.Next();
    }
  }

    
  std::string IndexBackend::GetPublicId(int64_t resourceId)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT publicId FROM Resources WHERE internalId=${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", resourceId);

    statement.Execute(args);

    if (statement.IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }
    else
    {
      return ReadString(statement, 0);
    }
  }

    
  uint64_t IndexBackend::GetResourceCount(OrthancPluginResourceType resourceType)
  {
    std::auto_ptr<DatabaseManager::CachedStatement> statement;

    switch (manager_.GetDialect())
    {
      case Dialect_MySQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COUNT(*) AS UNSIGNED INT) FROM Resources WHERE resourceType=${type}"));
        break;

      case Dialect_PostgreSQL:
        statement.reset(new DatabaseManager::CachedStatement(
                        STATEMENT_FROM_HERE, GetManager(),
                        "SELECT CAST(COUNT(*) AS BIGINT) FROM Resources WHERE resourceType=${type}"));
        break;

      case Dialect_SQLite:
        statement.reset(new DatabaseManager::CachedStatement(
                        STATEMENT_FROM_HERE, GetManager(),
                        "SELECT COUNT(*) FROM Resources WHERE resourceType=${type}"));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    statement->SetReadOnly(true);
    statement->SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("type", resourceType);

    statement->Execute(args);

    return static_cast<uint64_t>(ReadInteger64(*statement, 0));
  }

    
  OrthancPluginResourceType IndexBackend::GetResourceType(int64_t resourceId)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT resourceType FROM Resources WHERE internalId=${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", resourceId);

    statement.Execute(args);

    if (statement.IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }
    else
    {
      return static_cast<OrthancPluginResourceType>(ReadInteger32(statement, 0));
    }
  }

    
  uint64_t IndexBackend::GetTotalCompressedSize()
  {
    std::auto_ptr<DatabaseManager::CachedStatement> statement;

    // NB: "COALESCE" is used to replace "NULL" by "0" if the number of rows is empty

    switch (manager_.GetDialect())
    {
      case Dialect_MySQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COALESCE(SUM(compressedSize), 0) AS UNSIGNED INTEGER) FROM AttachedFiles"));
        break;
        
      case Dialect_PostgreSQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COALESCE(SUM(compressedSize), 0) AS BIGINT) FROM AttachedFiles"));
        break;

      case Dialect_SQLite:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT COALESCE(SUM(compressedSize), 0) FROM AttachedFiles"));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    statement->SetReadOnly(true);
    statement->Execute();

    return static_cast<uint64_t>(ReadInteger64(*statement, 0));
  }

    
  uint64_t IndexBackend::GetTotalUncompressedSize()
  {
    std::auto_ptr<DatabaseManager::CachedStatement> statement;

    // NB: "COALESCE" is used to replace "NULL" by "0" if the number of rows is empty

    switch (manager_.GetDialect())
    {
      case Dialect_MySQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COALESCE(SUM(uncompressedSize), 0) AS UNSIGNED INTEGER) FROM AttachedFiles"));
        break;
        
      case Dialect_PostgreSQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COALESCE(SUM(uncompressedSize), 0) AS BIGINT) FROM AttachedFiles"));
        break;

      case Dialect_SQLite:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT COALESCE(SUM(uncompressedSize), 0) FROM AttachedFiles"));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    statement->SetReadOnly(true);
    statement->Execute();

    return static_cast<uint64_t>(ReadInteger64(*statement, 0));
  }

    
  bool IndexBackend::IsExistingResource(int64_t internalId)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM Resources WHERE internalId=${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", internalId);

    statement.Execute(args);

    return !statement.IsDone();
  }

    
  bool IndexBackend::IsProtectedPatient(int64_t internalId)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT * FROM PatientRecyclingOrder WHERE patientId = ${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", internalId);

    statement.Execute(args);

    return statement.IsDone();
  }

    
  void IndexBackend::ListAvailableMetadata(std::list<int32_t>& target /*out*/,
                                           int64_t id)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT type FROM Metadata WHERE id=${id}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    ReadListOfIntegers<int32_t>(target, statement, args);
  }

    
  void IndexBackend::ListAvailableAttachments(std::list<int32_t>& target /*out*/,
                                              int64_t id)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT fileType FROM AttachedFiles WHERE id=${id}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    ReadListOfIntegers<int32_t>(target, statement, args);
  }

    
  void IndexBackend::LogChange(const OrthancPluginChange& change)
  {
    int64_t id;
    OrthancPluginResourceType type;
    if (!LookupResource(id, type, change.publicId) ||
        type != change.resourceType)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }
      
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "INSERT INTO Changes VALUES(${}, ${changeType}, ${id}, ${resourceType}, ${date})");

    statement.SetParameterType("changeType", ValueType_Integer64);
    statement.SetParameterType("id", ValueType_Integer64);
    statement.SetParameterType("resourceType", ValueType_Integer64);
    statement.SetParameterType("date", ValueType_Utf8String);

    Dictionary args;
    args.SetIntegerValue("changeType", change.changeType);
    args.SetIntegerValue("id", id);
    args.SetIntegerValue("resourceType", change.resourceType);
    args.SetUtf8Value("date", change.date);

    statement.Execute(args);
  }

    
  void IndexBackend::LogExportedResource(const OrthancPluginExportedResource& resource)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "INSERT INTO ExportedResources VALUES(${}, ${type}, ${publicId}, "
      "${modality}, ${patient}, ${study}, ${series}, ${instance}, ${date})");

    statement.SetParameterType("type", ValueType_Integer64);
    statement.SetParameterType("publicId", ValueType_Utf8String);
    statement.SetParameterType("modality", ValueType_Utf8String);
    statement.SetParameterType("patient", ValueType_Utf8String);
    statement.SetParameterType("study", ValueType_Utf8String);
    statement.SetParameterType("series", ValueType_Utf8String);
    statement.SetParameterType("instance", ValueType_Utf8String);
    statement.SetParameterType("date", ValueType_Utf8String);

    Dictionary args;
    args.SetIntegerValue("type", resource.resourceType);
    args.SetUtf8Value("publicId", resource.publicId);
    args.SetUtf8Value("modality", resource.modality);
    args.SetUtf8Value("patient", resource.patientId);
    args.SetUtf8Value("study", resource.studyInstanceUid);
    args.SetUtf8Value("series", resource.seriesInstanceUid);
    args.SetUtf8Value("instance", resource.sopInstanceUid);
    args.SetUtf8Value("date", resource.date);

    statement.Execute(args);
  }

    
  /* Use GetOutput().AnswerAttachment() */
  bool IndexBackend::LookupAttachment(int64_t id,
                                      int32_t contentType)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT uuid, uncompressedSize, compressionType, compressedSize, "
      "uncompressedHash, compressedHash FROM AttachedFiles WHERE id=${id} AND fileType=${type}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);
    args.SetIntegerValue("type", static_cast<int>(contentType));

    statement.Execute(args);

    if (statement.IsDone())
    {
      return false;
    }
    else
    {
      GetOutput().AnswerAttachment(ReadString(statement, 0),
                                   contentType,
                                   ReadInteger64(statement, 1),
                                   ReadString(statement, 4),
                                   ReadInteger32(statement, 2),
                                   ReadInteger64(statement, 3),
                                   ReadString(statement, 5));
      return true;
    }
  }

    
  bool IndexBackend::LookupGlobalProperty(std::string& target /*out*/,
                                          int32_t property)
  {
    return ::OrthancDatabases::LookupGlobalProperty(target, manager_, static_cast<Orthanc::GlobalProperty>(property));
  }

    
  void IndexBackend::LookupIdentifier(std::list<int64_t>& target /*out*/,
                                      OrthancPluginResourceType resourceType,
                                      uint16_t group,
                                      uint16_t element,
                                      OrthancPluginIdentifierConstraint constraint,
                                      const char* value)
  {
    std::auto_ptr<DatabaseManager::CachedStatement> statement;

    std::string header =
      "SELECT d.id FROM DicomIdentifiers AS d, Resources AS r WHERE "
      "d.id = r.internalId AND r.resourceType=${type} AND d.tagGroup=${group} "
      "AND d.tagElement=${element} AND ";
      
    switch (constraint)
    {
      case OrthancPluginIdentifierConstraint_Equal:
        header += "d.value = ${value}";
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, manager_, header.c_str()));
        break;
        
      case OrthancPluginIdentifierConstraint_SmallerOrEqual:
        header += "d.value <= ${value}";
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, manager_, header.c_str()));
        break;
        
      case OrthancPluginIdentifierConstraint_GreaterOrEqual:
        header += "d.value >= ${value}";
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, manager_, header.c_str()));
        break;
        
      case OrthancPluginIdentifierConstraint_Wildcard:
        header += "d.value LIKE ${value}";
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, manager_, header.c_str()));
        break;
        
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
    }

    statement->SetReadOnly(true);
    statement->SetParameterType("type", ValueType_Integer64);
    statement->SetParameterType("group", ValueType_Integer64);
    statement->SetParameterType("element", ValueType_Integer64);
    statement->SetParameterType("value", ValueType_Utf8String);

    Dictionary args;
    args.SetIntegerValue("type", resourceType);
    args.SetIntegerValue("group", group);
    args.SetIntegerValue("element", element);

    if (constraint == OrthancPluginIdentifierConstraint_Wildcard)
    {
      args.SetUtf8Value("value", ConvertWildcardToLike(value));
    }
    else
    {
      args.SetUtf8Value("value", value);
    }

    statement->Execute(args);

    target.clear();
    while (!statement->IsDone())
    {
      target.push_back(ReadInteger64(*statement, 0));
      statement->Next();
    }
  }

    
  void IndexBackend::LookupIdentifierRange(std::list<int64_t>& target /*out*/,
                                           OrthancPluginResourceType resourceType,
                                           uint16_t group,
                                           uint16_t element,
                                           const char* start,
                                           const char* end)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT d.id FROM DicomIdentifiers AS d, Resources AS r WHERE "
      "d.id = r.internalId AND r.resourceType=${type} AND d.tagGroup=${group} "
      "AND d.tagElement=${element} AND d.value>=${start} AND d.value<=${end}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("type", ValueType_Integer64);
    statement.SetParameterType("group", ValueType_Integer64);
    statement.SetParameterType("element", ValueType_Integer64);
    statement.SetParameterType("start", ValueType_Utf8String);
    statement.SetParameterType("end", ValueType_Utf8String);
    
    Dictionary args;
    args.SetIntegerValue("type", resourceType);
    args.SetIntegerValue("group", group);
    args.SetIntegerValue("element", element);
    args.SetUtf8Value("start", start);
    args.SetUtf8Value("end", end);

    statement.Execute(args);

    target.clear();
    while (!statement.IsDone())
    {
      target.push_back(ReadInteger64(statement, 0));
      statement.Next();
    }
  }

    
  bool IndexBackend::LookupMetadata(std::string& target /*out*/,
                                    int64_t id,
                                    int32_t metadataType)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT value FROM Metadata WHERE id=${id} and type=${type}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);
    args.SetIntegerValue("type", metadataType);

    statement.Execute(args);

    if (statement.IsDone())
    {
      return false;
    }
    else
    {
      target = ReadString(statement, 0);
      return true;
    }
  } 

    
  bool IndexBackend::LookupParent(int64_t& parentId /*out*/,
                                  int64_t resourceId)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT parentId FROM Resources WHERE internalId=${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", resourceId);

    statement.Execute(args);

    if (statement.IsDone() ||
        statement.GetResultField(0).GetType() == ValueType_Null)
    {
      return false;
    }
    else
    {
      parentId = ReadInteger64(statement, 0);
      return true;
    }
  }

    
  bool IndexBackend::LookupResource(int64_t& id /*out*/,
                                    OrthancPluginResourceType& type /*out*/,
                                    const char* publicId)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT internalId, resourceType FROM Resources WHERE publicId=${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Utf8String);

    Dictionary args;
    args.SetUtf8Value("id", publicId);

    statement.Execute(args);

    if (statement.IsDone())
    {
      return false;
    }
    else
    {
      id = ReadInteger64(statement, 0);
      type = static_cast<OrthancPluginResourceType>(ReadInteger32(statement, 1));
      return true;
    }
  }

    
  bool IndexBackend::SelectPatientToRecycle(int64_t& internalId /*out*/)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT patientId FROM PatientRecyclingOrder ORDER BY seq ASC LIMIT 1");

    statement.SetReadOnly(true);
    statement.Execute();

    if (statement.IsDone())
    {
      return false;
    }
    else
    {
      internalId = ReadInteger64(statement, 0);
      return true;
    }
  }

    
  bool IndexBackend::SelectPatientToRecycle(int64_t& internalId /*out*/,
                                            int64_t patientIdToAvoid)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "SELECT patientId FROM PatientRecyclingOrder "
      "WHERE patientId != ${id} ORDER BY seq ASC LIMIT 1");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", patientIdToAvoid);

    statement.Execute(args);

    if (statement.IsDone())
    {
      return false;
    }
    else
    {
      internalId = ReadInteger64(statement, 0);
      return true;
    }
  }

    
  void IndexBackend::SetGlobalProperty(int32_t property,
                                       const char* value)
  {
    return ::OrthancDatabases::SetGlobalProperty(manager_, static_cast<Orthanc::GlobalProperty>(property), value);
  }


  static void ExecuteSetTag(DatabaseManager::CachedStatement& statement,
                            int64_t id,
                            uint16_t group,
                            uint16_t element,
                            const char* value)
  {
    statement.SetParameterType("id", ValueType_Integer64);
    statement.SetParameterType("group", ValueType_Integer64);
    statement.SetParameterType("element", ValueType_Integer64);
    statement.SetParameterType("value", ValueType_Utf8String);
        
    Dictionary args;
    args.SetIntegerValue("id", id);
    args.SetIntegerValue("group", group);
    args.SetIntegerValue("element", element);
    args.SetUtf8Value("value", value);
        
    statement.Execute(args);
  }

    
  void IndexBackend::SetMainDicomTag(int64_t id,
                                     uint16_t group,
                                     uint16_t element,
                                     const char* value)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "INSERT INTO MainDicomTags VALUES(${id}, ${group}, ${element}, ${value})");

    ExecuteSetTag(statement, id, group, element, value);
  }

    
  void IndexBackend::SetIdentifierTag(int64_t id,
                                      uint16_t group,
                                      uint16_t element,
                                      const char* value)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, manager_,
      "INSERT INTO DicomIdentifiers VALUES(${id}, ${group}, ${element}, ${value})");
        
    ExecuteSetTag(statement, id, group, element, value);
  } 

    
  void IndexBackend::SetMetadata(int64_t id,
                                 int32_t metadataType,
                                 const char* value)
  {
    if (manager_.GetDialect() == Dialect_SQLite)
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager_,
        "INSERT OR REPLACE INTO Metadata VALUES (${id}, ${type}, ${value})");
        
      statement.SetParameterType("id", ValueType_Integer64);
      statement.SetParameterType("type", ValueType_Integer64);
      statement.SetParameterType("value", ValueType_Utf8String);
        
      Dictionary args;
      args.SetIntegerValue("id", id);
      args.SetIntegerValue("type", metadataType);
      args.SetUtf8Value("value", value);
        
      statement.Execute(args);
    }
    else
    {
      {
        DatabaseManager::CachedStatement statement(
          STATEMENT_FROM_HERE, manager_,
          "DELETE FROM Metadata WHERE id=${id} AND type=${type}");
        
        statement.SetParameterType("id", ValueType_Integer64);
        statement.SetParameterType("type", ValueType_Integer64);
        
        Dictionary args;
        args.SetIntegerValue("id", id);
        args.SetIntegerValue("type", metadataType);
        
        statement.Execute(args);
      }

      {
        DatabaseManager::CachedStatement statement(
          STATEMENT_FROM_HERE, manager_,
          "INSERT INTO Metadata VALUES (${id}, ${type}, ${value})");
        
        statement.SetParameterType("id", ValueType_Integer64);
        statement.SetParameterType("type", ValueType_Integer64);
        statement.SetParameterType("value", ValueType_Utf8String);
        
        Dictionary args;
        args.SetIntegerValue("id", id);
        args.SetIntegerValue("type", metadataType);
        args.SetUtf8Value("value", value);
        
        statement.Execute(args);
      }
    }
  }

    
  void IndexBackend::SetProtectedPatient(int64_t internalId, 
                                         bool isProtected)
  {
    if (isProtected)
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager_,
        "DELETE FROM PatientRecyclingOrder WHERE patientId=${id}");
        
      statement.SetParameterType("id", ValueType_Integer64);
        
      Dictionary args;
      args.SetIntegerValue("id", internalId);
        
      statement.Execute(args);
    }
    else if (IsProtectedPatient(internalId))
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager_,
        "INSERT INTO PatientRecyclingOrder VALUES(${}, ${id})");
        
      statement.SetParameterType("id", ValueType_Integer64);
        
      Dictionary args;
      args.SetIntegerValue("id", internalId);
        
      statement.Execute(args);
    }
    else
    {
      // Nothing to do: The patient is already unprotected
    }
  }

    
  uint32_t IndexBackend::GetDatabaseVersion()
  {
    std::string version = "unknown";
      
    if (LookupGlobalProperty(version, Orthanc::GlobalProperty_DatabaseSchemaVersion))
    {
      try
      {
        return boost::lexical_cast<unsigned int>(version);
      }
      catch (boost::bad_lexical_cast&)
      {
      }
    }

    LOG(ERROR) << "The database is corrupted. Drop it manually for Orthanc to recreate it";
    throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);
  }

    
  /**
   * Upgrade the database to the specified version of the database
   * schema.  The upgrade script is allowed to make calls to
   * OrthancPluginReconstructMainDicomTags().
   **/
  void IndexBackend::UpgradeDatabase(uint32_t  targetVersion,
                                     OrthancPluginStorageArea* storageArea)
  {
    LOG(ERROR) << "Upgrading database is not implemented by this plugin";
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }

    
  void IndexBackend::ClearMainDicomTags(int64_t internalId)
  {
    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager_,
        "DELETE FROM MainDicomTags WHERE id=${id}");
        
      statement.SetParameterType("id", ValueType_Integer64);
        
      Dictionary args;
      args.SetIntegerValue("id", internalId);
        
      statement.Execute(args);
    }

    {
      DatabaseManager::CachedStatement statement(
        STATEMENT_FROM_HERE, manager_,
        "DELETE FROM DicomIdentifiers WHERE id=${id}");
        
      statement.SetParameterType("id", ValueType_Integer64);
        
      Dictionary args;
      args.SetIntegerValue("id", internalId);
        
      statement.Execute(args);
    }
  }


  // For unit testing only!
  uint64_t IndexBackend::GetResourcesCount()
  {
    std::auto_ptr<DatabaseManager::CachedStatement> statement;

    switch (manager_.GetDialect())
    {
      case Dialect_MySQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COUNT(*) AS UNSIGNED INT) FROM Resources"));
        break;
        
      case Dialect_PostgreSQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COUNT(*) AS BIGINT) FROM Resources"));
        break;

      case Dialect_SQLite:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT COUNT(*) FROM Resources"));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    statement->SetReadOnly(true);
    statement->Execute();

    return static_cast<uint64_t>(ReadInteger64(*statement, 0));
  }    


  // For unit testing only!
  uint64_t IndexBackend::GetUnprotectedPatientsCount()
  {
    std::auto_ptr<DatabaseManager::CachedStatement> statement;

    switch (manager_.GetDialect())
    {
      case Dialect_MySQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COUNT(*) AS UNSIGNED INT) FROM PatientRecyclingOrder"));
        break;
        
      case Dialect_PostgreSQL:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT CAST(COUNT(*) AS BIGINT) FROM PatientRecyclingOrder"));
        break;

      case Dialect_SQLite:
        statement.reset(new DatabaseManager::CachedStatement(
                          STATEMENT_FROM_HERE, GetManager(),
                          "SELECT COUNT(*) FROM PatientRecyclingOrder"));
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    statement->SetReadOnly(true);
    statement->Execute();

    return static_cast<uint64_t>(ReadInteger64(*statement, 0));
  }    


  // For unit testing only!
  bool IndexBackend::GetParentPublicId(std::string& target,
                                       int64_t id)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "SELECT a.publicId FROM Resources AS a, Resources AS b "
      "WHERE a.internalId = b.parentId AND b.internalId = ${id}");

    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    statement.Execute(args);

    if (statement.IsDone())
    {
      return false;
    }
    else
    {
      target = ReadString(statement, 0);
      return true;
    }
  }


  // For unit tests only!
  void IndexBackend::GetChildren(std::list<std::string>& childrenPublicIds,
                                 int64_t id)
  { 
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "SELECT publicId FROM Resources WHERE parentId=${id}");
      
    statement.SetReadOnly(true);
    statement.SetParameterType("id", ValueType_Integer64);

    Dictionary args;
    args.SetIntegerValue("id", id);

    ReadListOfStrings(childrenPublicIds, statement, args);
  }
}
