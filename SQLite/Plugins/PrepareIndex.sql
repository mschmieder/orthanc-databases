CREATE TABLE GlobalProperties(
       property INTEGER PRIMARY KEY,
       value TEXT
       );

CREATE TABLE Resources(
       internalId INTEGER PRIMARY KEY AUTOINCREMENT,
       resourceType INTEGER,
       publicId TEXT,
       parentId INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE MainDicomTags(
       id INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE,
       tagGroup INTEGER,
       tagElement INTEGER,
       value TEXT,
       PRIMARY KEY(id, tagGroup, tagElement)
       );

CREATE TABLE DicomIdentifiers(
       id INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE,
       tagGroup INTEGER,
       tagElement INTEGER,
       value TEXT,
       PRIMARY KEY(id, tagGroup, tagElement)
       );

CREATE TABLE Metadata(
       id INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE,
       type INTEGER,
       value TEXT,
       PRIMARY KEY(id, type)
       );

CREATE TABLE AttachedFiles(
       id INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE,
       fileType INTEGER,
       uuid TEXT,
       compressedSize INTEGER,
       uncompressedSize INTEGER,
       compressionType INTEGER,
       uncompressedHash TEXT,
       compressedHash TEXT,
       PRIMARY KEY(id, fileType)
       );              

CREATE TABLE Changes(
       seq INTEGER PRIMARY KEY AUTOINCREMENT,
       changeType INTEGER,
       internalId INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE,
       resourceType INTEGER,
       date TEXT
       );

CREATE TABLE ExportedResources(
       seq INTEGER PRIMARY KEY AUTOINCREMENT,
       resourceType INTEGER,
       publicId TEXT,
       remoteModality TEXT,
       patientId TEXT,
       studyInstanceUid TEXT,
       seriesInstanceUid TEXT,
       sopInstanceUid TEXT,
       date TEXT
       ); 

CREATE TABLE PatientRecyclingOrder(
       seq INTEGER PRIMARY KEY AUTOINCREMENT,
       patientId INTEGER REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE INDEX ChildrenIndex ON Resources(parentId);
CREATE INDEX PublicIndex ON Resources(publicId);
CREATE INDEX ResourceTypeIndex ON Resources(resourceType);
CREATE INDEX PatientRecyclingIndex ON PatientRecyclingOrder(patientId);

CREATE INDEX MainDicomTagsIndex1 ON MainDicomTags(id);
-- The 2 following indexes were removed in Orthanc 0.8.5 (database v5), to speed up
-- CREATE INDEX MainDicomTagsIndex2 ON MainDicomTags(tagGroup, tagElement);
-- CREATE INDEX MainDicomTagsIndexValues ON MainDicomTags(value COLLATE BINARY);

-- The 3 following indexes were added in Orthanc 0.8.5 (database v5)
CREATE INDEX DicomIdentifiersIndex1 ON DicomIdentifiers(id);
CREATE INDEX DicomIdentifiersIndex2 ON DicomIdentifiers(tagGroup, tagElement);
CREATE INDEX DicomIdentifiersIndexValues ON DicomIdentifiers(value COLLATE BINARY);

CREATE INDEX ChangesIndex ON Changes(internalId);



-- New tables wrt. Orthanc core
CREATE TABLE DeletedFiles(
       uuid TEXT NOT NULL,        -- 0
       fileType INTEGER,          -- 1
       compressedSize INTEGER,    -- 2
       uncompressedSize INTEGER,  -- 3
       compressionType INTEGER,   -- 4
       uncompressedHash TEXT,     -- 5
       compressedHash TEXT        -- 6
       );

CREATE TABLE RemainingAncestor(
       resourceType INTEGER NOT NULL,
       publicId TEXT NOT NULL
       );

CREATE TABLE DeletedResources(
       resourceType INTEGER NOT NULL,
       publicId TEXT NOT NULL
       );
-- End of differences



CREATE TRIGGER AttachedFileDeleted
AFTER DELETE ON AttachedFiles
BEGIN
   INSERT INTO DeletedFiles VALUES(old.uuid, old.filetype, old.compressedSize,
                                   old.uncompressedSize, old.compressionType,
                                   old.uncompressedHash, old.compressedHash);
END;


CREATE TRIGGER ResourceDeleted
AFTER DELETE ON Resources
BEGIN
   INSERT INTO DeletedResources VALUES(old.resourceType, old.publicId);
END;


-- Delete a parent resource when its unique child gets deleted 
CREATE TRIGGER ResourceDeletedParentCleaning
AFTER DELETE ON Resources
FOR EACH ROW WHEN NOT EXISTS (SELECT 1 FROM Resources WHERE parentId = old.parentId)
BEGIN
  DELETE FROM Resources WHERE internalId = old.parentId;
END;

-- Signal that the deleted resource has a remaining parent, if the
-- deleted resource has a sibling resource
CREATE TRIGGER ResourceRemainingAncestorFound
AFTER DELETE ON Resources
FOR EACH ROW WHEN EXISTS (SELECT 1 FROM Resources WHERE parentId = old.parentId)
BEGIN
   INSERT INTO RemainingAncestor(resourceType, publicId)
          SELECT resourceType, publicId FROM Resources WHERE internalId = old.parentId;
END;


CREATE TRIGGER PatientAdded
AFTER INSERT ON Resources
FOR EACH ROW WHEN new.resourceType = 0  -- The "0" corresponds to "OrthancPluginResourceType_Patient"
BEGIN
  INSERT INTO PatientRecyclingOrder VALUES (NULL, new.internalId);
END;
