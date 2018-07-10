CREATE TABLE GlobalProperties(
       property INTEGER PRIMARY KEY,
       value TEXT
       );

CREATE TABLE Resources(
       internalId BIGSERIAL NOT NULL PRIMARY KEY,
       resourceType INTEGER NOT NULL,
       publicId VARCHAR(64) NOT NULL,
       parentId BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE MainDicomTags(
       id BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE,
       tagGroup INTEGER,
       tagElement INTEGER,
       value TEXT,
       PRIMARY KEY(id, tagGroup, tagElement)
       );

CREATE TABLE DicomIdentifiers(
       id BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE,
       tagGroup INTEGER,
       tagElement INTEGER,
       value TEXT,
       PRIMARY KEY(id, tagGroup, tagElement)
       );

CREATE TABLE Metadata(
       id BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE,
       type INTEGER NOT NULL,
       value TEXT,
       PRIMARY KEY(id, type)
       );

CREATE TABLE AttachedFiles(
       id BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE,
       fileType INTEGER,
       uuid VARCHAR(64) NOT NULL,
       compressedSize BIGINT,
       uncompressedSize BIGINT,
       compressionType INTEGER,
       uncompressedHash VARCHAR(40),
       compressedHash VARCHAR(40),
       PRIMARY KEY(id, fileType)
       );              

CREATE TABLE Changes(
       seq BIGSERIAL NOT NULL PRIMARY KEY,
       changeType INTEGER,
       internalId BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE,
       resourceType INTEGER,
       date VARCHAR(64)
       );

CREATE TABLE ExportedResources(
       seq BIGSERIAL NOT NULL PRIMARY KEY,
       resourceType INTEGER,
       publicId VARCHAR(64),
       remoteModality TEXT,
       patientId VARCHAR(64),
       studyInstanceUid TEXT,
       seriesInstanceUid TEXT,
       sopInstanceUid TEXT,
       date VARCHAR(64)
       ); 

CREATE TABLE PatientRecyclingOrder(
       seq BIGSERIAL NOT NULL PRIMARY KEY,
       patientId BIGINT REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE INDEX ChildrenIndex ON Resources(parentId);
CREATE INDEX PublicIndex ON Resources(publicId);
CREATE INDEX ResourceTypeIndex ON Resources(resourceType);
CREATE INDEX PatientRecyclingIndex ON PatientRecyclingOrder(patientId);

CREATE INDEX MainDicomTagsIndex ON MainDicomTags(id);
CREATE INDEX DicomIdentifiersIndex1 ON DicomIdentifiers(id);
CREATE INDEX DicomIdentifiersIndex2 ON DicomIdentifiers(tagGroup, tagElement);
CREATE INDEX DicomIdentifiersIndexValues ON DicomIdentifiers(value);

CREATE INDEX ChangesIndex ON Changes(internalId);


-- New tables wrt. Orthanc core
CREATE TABLE DeletedFiles(
       uuid VARCHAR(64) NOT NULL,      -- 0
       fileType INTEGER,               -- 1
       compressedSize BIGINT,          -- 2
       uncompressedSize BIGINT,        -- 3
       compressionType INTEGER,        -- 4
       uncompressedHash VARCHAR(40),   -- 5
       compressedHash VARCHAR(40)      -- 6
       );

CREATE TABLE RemainingAncestor(
       resourceType INTEGER NOT NULL,
       publicId VARCHAR(64) NOT NULL
       );

CREATE TABLE DeletedResources(
       resourceType INTEGER NOT NULL,
       publicId VARCHAR(64) NOT NULL
       );
-- End of differences


CREATE FUNCTION AttachedFileDeletedFunc() 
RETURNS TRIGGER AS $body$
BEGIN
  INSERT INTO DeletedFiles VALUES
    (old.uuid, old.filetype, old.compressedSize,
     old.uncompressedSize, old.compressionType,
     old.uncompressedHash, old.compressedHash);
  RETURN NULL;
END;
$body$ LANGUAGE plpgsql;

CREATE TRIGGER AttachedFileDeleted
AFTER DELETE ON AttachedFiles
FOR EACH ROW
EXECUTE PROCEDURE AttachedFileDeletedFunc();


-- The following trigger combines 2 triggers from SQLite:
-- ResourceDeleted + ResourceDeletedParentCleaning
CREATE FUNCTION ResourceDeletedFunc() 
RETURNS TRIGGER AS $body$
BEGIN
  --RAISE NOTICE 'Delete resource %', old.parentId;
  INSERT INTO DeletedResources VALUES (old.resourceType, old.publicId);
  
  -- http://stackoverflow.com/a/11299968/881731
  IF EXISTS (SELECT 1 FROM Resources WHERE parentId = old.parentId) THEN
    -- Signal that the deleted resource has a remaining parent
    INSERT INTO RemainingAncestor
      SELECT resourceType, publicId FROM Resources WHERE internalId = old.parentId;
  ELSE
    -- Delete a parent resource when its unique child is deleted 
    DELETE FROM Resources WHERE internalId = old.parentId;
  END IF;
  RETURN NULL;
END;
$body$ LANGUAGE plpgsql;

CREATE TRIGGER ResourceDeleted
AFTER DELETE ON Resources
FOR EACH ROW
EXECUTE PROCEDURE ResourceDeletedFunc();



CREATE FUNCTION PatientAddedFunc() 
RETURNS TRIGGER AS $body$
BEGIN
  -- The "0" corresponds to "OrthancPluginResourceType_Patient"
  IF new.resourceType = 0 THEN
    INSERT INTO PatientRecyclingOrder VALUES (DEFAULT, new.internalId);
  END IF;
  RETURN NULL;
END;
$body$ LANGUAGE plpgsql;

CREATE TRIGGER PatientAdded
AFTER INSERT ON Resources
FOR EACH ROW
EXECUTE PROCEDURE PatientAddedFunc();
