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


#include "StorageBackend.h"

#if HAS_ORTHANC_EXCEPTION != 1
#  error HAS_ORTHANC_EXCEPTION must be set to 1
#endif

#include "../../Framework/Common/BinaryStringValue.h"
#include "../../Framework/Common/FileValue.h"

#include <Core/OrthancException.h>


#define ORTHANC_PLUGINS_DATABASE_CATCH                                  \
  catch (::Orthanc::OrthancException& e)                                \
  {                                                                     \
    return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());       \
  }                                                                     \
  catch (::std::runtime_error& e)                                       \
  {                                                                     \
    std::string s = "Exception in storage area back-end: " + std::string(e.what()); \
    OrthancPluginLogError(context_, s.c_str());                         \
    return OrthancPluginErrorCode_DatabasePlugin;                       \
  }                                                                     \
  catch (...)                                                           \
  {                                                                     \
    OrthancPluginLogError(context_, "Native exception");                \
    return OrthancPluginErrorCode_DatabasePlugin;                       \
  }


namespace OrthancDatabases
{
  void StorageBackend::ReadFromString(void*& buffer,
                                      size_t& size,
                                      const std::string& content)
  {
    size = content.size();

    if (content.empty())
    {
      buffer = NULL;
    }
    else
    {
      buffer = malloc(size);

      if (buffer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory);
      }

      memcpy(buffer, content.c_str(), size);
    }
  }

  
  StorageBackend::StorageBackend(IDatabaseFactory* factory) :
    manager_(factory)
  {
  }


  void StorageBackend::ReadToString(std::string& content,
                                    DatabaseManager::Transaction& transaction, 
                                    const std::string& uuid,
                                    OrthancPluginContentType type)
  {
    void* buffer = NULL; 
    size_t size;
    Read(buffer, size, transaction, uuid, type);

    try
    {
      content.resize(size);
    }
    catch (std::bad_alloc&)
    {
      if (size != 0)
      {
        free(buffer);
      }

      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory);
    }

    if (size != 0)
    {
      assert(buffer != NULL);
      memcpy(&content[0], buffer, size);
    }

    free(buffer);
  }


  void StorageBackend::Create(DatabaseManager::Transaction& transaction,
                              const std::string& uuid,
                              const void* content,
                              size_t size,
                              OrthancPluginContentType type)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "INSERT INTO StorageArea VALUES (${uuid}, ${content}, ${type})");
     
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("content", ValueType_File);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("uuid", uuid);
    args.SetFileValue("content", content, size);
    args.SetIntegerValue("type", type);
     
    statement.Execute(args);
  }


  void StorageBackend::Read(void*& content,
                            size_t& size,
                            DatabaseManager::Transaction& transaction, 
                            const std::string& uuid,
                            OrthancPluginContentType type) 
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "SELECT content FROM StorageArea WHERE uuid=${uuid} AND type=${type}");
     
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("uuid", uuid);
    args.SetIntegerValue("type", type);
     
    statement.Execute(args);

    if (statement.IsDone())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }
    else if (statement.GetResultFieldsCount() != 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
    }
    else
    {
      const IValue& value = statement.GetResultField(0);
      
      switch (value.GetType())
      {
        case ValueType_File:
          ReadFromString(content, size,
                         dynamic_cast<const FileValue&>(value).GetContent());
          break;

        case ValueType_BinaryString:
          ReadFromString(content, size,
                         dynamic_cast<const BinaryStringValue&>(value).GetContent());
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_Database);        
      }
    }
  }


  void StorageBackend::Remove(DatabaseManager::Transaction& transaction,
                              const std::string& uuid,
                              OrthancPluginContentType type)
  {
    DatabaseManager::CachedStatement statement(
      STATEMENT_FROM_HERE, GetManager(),
      "DELETE FROM StorageArea WHERE uuid=${uuid} AND type=${type}");
     
    statement.SetParameterType("uuid", ValueType_Utf8String);
    statement.SetParameterType("type", ValueType_Integer64);

    Dictionary args;
    args.SetUtf8Value("uuid", uuid);
    args.SetIntegerValue("type", type);
     
    statement.Execute(args);
  }



  static OrthancPluginContext* context_ = NULL;
  static std::auto_ptr<StorageBackend>  backend_;
    

  static OrthancPluginErrorCode StorageCreate(const char* uuid,
                                              const void* content,
                                              int64_t size,
                                              OrthancPluginContentType type)
  {
    try
    {
      DatabaseManager::Transaction transaction(backend_->GetManager());
      backend_->Create(transaction, uuid, content, static_cast<size_t>(size), type);
      transaction.Commit();
      return OrthancPluginErrorCode_Success;
    }
    ORTHANC_PLUGINS_DATABASE_CATCH;
  }


  static OrthancPluginErrorCode StorageRead(void** content,
                                            int64_t* size,
                                            const char* uuid,
                                            OrthancPluginContentType type)
  {
    try
    {
      DatabaseManager::Transaction transaction(backend_->GetManager());
      size_t tmp;
      backend_->Read(*content, tmp, transaction, uuid, type);
      *size = static_cast<int64_t>(tmp);
      transaction.Commit();
      return OrthancPluginErrorCode_Success;
    }
    ORTHANC_PLUGINS_DATABASE_CATCH;
  }


  static OrthancPluginErrorCode StorageRemove(const char* uuid,
                                              OrthancPluginContentType type)
  {
    try
    {
      DatabaseManager::Transaction transaction(backend_->GetManager());
      backend_->Remove(transaction, uuid, type);
      transaction.Commit();
      return OrthancPluginErrorCode_Success;
    }
    ORTHANC_PLUGINS_DATABASE_CATCH;
  }

  
  void StorageBackend::Register(OrthancPluginContext* context,
                                StorageBackend* backend)
  {
    if (context == NULL ||
        backend == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    
    if (context_ != NULL ||
        backend_.get() != NULL)
    {
      // This function can only be invoked once in the plugin
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      context_ = context;
      backend_.reset(backend);
      backend_->GetManager().Open();

      OrthancPluginRegisterStorageArea(context_, StorageCreate, StorageRead, StorageRemove);
    }
  }


  void StorageBackend::Finalize()
  {
    backend_.reset(NULL);
    context_ = NULL;
  }
}
