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

#include "../Common/DatabaseManager.h"
#include "OrthancCppDatabasePlugin.h"


namespace OrthancDatabases
{
  class IndexBackend : public OrthancPlugins::IDatabaseBackend
  {
  private:
    DatabaseManager   manager_;

  protected:
    DatabaseManager& GetManager()
    {
      return manager_;
    }
    
    static int64_t ReadInteger64(const DatabaseManager::CachedStatement& statement,
                                 size_t field);

    static int32_t ReadInteger32(const DatabaseManager::CachedStatement& statement,
                                 size_t field);
    
    static std::string ReadString(const DatabaseManager::CachedStatement& statement,
                                  size_t field);
    
    template <typename T>
    static void ReadListOfIntegers(std::list<T>& target,
                                   DatabaseManager::CachedStatement& statement,
                                   const Dictionary& args);
    
    static void ReadListOfStrings(std::list<std::string>& target,
                                  DatabaseManager::CachedStatement& statement,
                                  const Dictionary& args);

    void ClearDeletedFiles();

    void ClearDeletedResources();

    void SignalDeletedFiles();

    void SignalDeletedResources();

  private:
    void ReadChangesInternal(bool& done,
                             DatabaseManager::CachedStatement& statement,
                             const Dictionary& args,
                             uint32_t maxResults);

    void ReadExportedResourcesInternal(bool& done,
                                       DatabaseManager::CachedStatement& statement,
                                       const Dictionary& args,
                                       uint32_t maxResults);

  public:
    IndexBackend(IDatabaseFactory* factory);
    
    Dialect GetDialect() const
    {
      return manager_.GetDialect();
    }
    
    virtual void Open()
    {
      manager_.Open();
    }
    
    virtual void Close()
    {
      manager_.Close();
    }
    
    virtual void AddAttachment(int64_t id,
                               const OrthancPluginAttachment& attachment);
    
    virtual void AttachChild(int64_t parent,
                             int64_t child);
    
    virtual void ClearChanges();
    
    virtual void ClearExportedResources();

    virtual void DeleteAttachment(int64_t id,
                                  int32_t attachment);
    
    virtual void DeleteMetadata(int64_t id,
                                int32_t metadataType);
    
    virtual void DeleteResource(int64_t id);

    virtual void GetAllInternalIds(std::list<int64_t>& target,
                                   OrthancPluginResourceType resourceType);
    
    virtual void GetAllPublicIds(std::list<std::string>& target,
                                 OrthancPluginResourceType resourceType);
    
    virtual void GetAllPublicIds(std::list<std::string>& target,
                                 OrthancPluginResourceType resourceType,
                                 uint64_t since,
                                 uint64_t limit);
    
    virtual void GetChanges(bool& done /*out*/,
                            int64_t since,
                            uint32_t maxResults);
    
    virtual void GetChildrenInternalId(std::list<int64_t>& target /*out*/,
                                       int64_t id);
    
    virtual void GetChildrenPublicId(std::list<std::string>& target /*out*/,
                                     int64_t id);
    
    virtual void GetExportedResources(bool& done /*out*/,
                                      int64_t since,
                                      uint32_t maxResults);
    
    virtual void GetLastChange();
    
    virtual void GetLastExportedResource();
    
    virtual void GetMainDicomTags(int64_t id);
    
    virtual std::string GetPublicId(int64_t resourceId);
    
    virtual uint64_t GetResourceCount(OrthancPluginResourceType resourceType);
    
    virtual OrthancPluginResourceType GetResourceType(int64_t resourceId);
    
    virtual uint64_t GetTotalCompressedSize();
    
    virtual uint64_t GetTotalUncompressedSize();
    
    virtual bool IsExistingResource(int64_t internalId);
    
    virtual bool IsProtectedPatient(int64_t internalId);
    
    virtual void ListAvailableMetadata(std::list<int32_t>& target /*out*/,
                                       int64_t id);
    
    virtual void ListAvailableAttachments(std::list<int32_t>& target /*out*/,
                                          int64_t id);
    
    virtual void LogChange(const OrthancPluginChange& change);
    
    virtual void LogExportedResource(const OrthancPluginExportedResource& resource);
    
    virtual bool LookupAttachment(int64_t id,
                                  int32_t contentType);
    
    virtual bool LookupGlobalProperty(std::string& target /*out*/,
                                      int32_t property);
    
    virtual void LookupIdentifier(std::list<int64_t>& target /*out*/,
                                  OrthancPluginResourceType resourceType,
                                  uint16_t group,
                                  uint16_t element,
                                  OrthancPluginIdentifierConstraint constraint,
                                  const char* value);
    
    virtual void LookupIdentifierRange(std::list<int64_t>& target /*out*/,
                                       OrthancPluginResourceType resourceType,
                                       uint16_t group,
                                       uint16_t element,
                                       const char* start,
                                       const char* end);

    virtual bool LookupMetadata(std::string& target /*out*/,
                                int64_t id,
                                int32_t metadataType);

    virtual bool LookupParent(int64_t& parentId /*out*/,
                              int64_t resourceId);
    
    virtual bool LookupResource(int64_t& id /*out*/,
                                OrthancPluginResourceType& type /*out*/,
                                const char* publicId);
    
    virtual bool SelectPatientToRecycle(int64_t& internalId /*out*/);
    
    virtual bool SelectPatientToRecycle(int64_t& internalId /*out*/,
                                        int64_t patientIdToAvoid);
    
    virtual void SetGlobalProperty(int32_t property,
                                   const char* value);

    virtual void SetMainDicomTag(int64_t id,
                                 uint16_t group,
                                 uint16_t element,
                                 const char* value);
    
    virtual void SetIdentifierTag(int64_t id,
                                  uint16_t group,
                                  uint16_t element,
                                  const char* value);

    virtual void SetMetadata(int64_t id,
                             int32_t metadataType,
                             const char* value);
    
    virtual void SetProtectedPatient(int64_t internalId, 
                                     bool isProtected);
    
    virtual void StartTransaction()
    {
      manager_.StartTransaction();
    }

    
    virtual void RollbackTransaction()
    {
      manager_.RollbackTransaction();
    }

    
    virtual void CommitTransaction()
    {
      manager_.CommitTransaction();
    }

    
    virtual uint32_t GetDatabaseVersion();
    
    virtual void UpgradeDatabase(uint32_t  targetVersion,
                                 OrthancPluginStorageArea* storageArea);
    
    virtual void ClearMainDicomTags(int64_t internalId);


    // For unit testing only!
    virtual uint64_t GetResourcesCount();

    // For unit testing only!
    virtual uint64_t GetUnprotectedPatientsCount();

    // For unit testing only!
    virtual bool GetParentPublicId(std::string& target,
                                   int64_t id);

    // For unit tests only!
    virtual void GetChildren(std::list<std::string>& childrenPublicIds,
                             int64_t id);
  };
}
