CREATE TABLE GlobalProperties(
       property INTEGER PRIMARY KEY,
       value TEXT
       );

-- Set GlobalProperty_DatabaseSchemaVersion
INSERT INTO GlobalProperties VALUES (1, '6');
       
CREATE TABLE Resources(
       internalId BIGINT NOT NULL AUTO_INCREMENT,
       resourceType INTEGER NOT NULL,
       publicId VARCHAR(64) NOT NULL,
       parentId BIGINT,
       PRIMARY KEY(internalId)
       -- MySQL does not allow recursive foreign keys on the same table
       -- CONSTRAINT Resources1 FOREIGN KEY (parentId) REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE MainDicomTags(
       id BIGINT NOT NULL,
       tagGroup INTEGER NOT NULL,
       tagElement INTEGER NOT NULL,
       value VARCHAR(255),
       PRIMARY KEY(id, tagGroup, tagElement),
       CONSTRAINT MainDicomTags1 FOREIGN KEY (id) REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE DicomIdentifiers(
       id BIGINT NOT NULL,
       tagGroup INTEGER NOT NULL,
       tagElement INTEGER NOT NULL,
       value VARCHAR(255),
       PRIMARY KEY(id, tagGroup, tagElement),
       CONSTRAINT DicomIdentifiers1 FOREIGN KEY (id) REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE Metadata(
       id BIGINT NOT NULL,
       type INTEGER NOT NULL,
       value TEXT,
       PRIMARY KEY(id, type),
       CONSTRAINT Metadata1 FOREIGN KEY (id) REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE AttachedFiles(
       id BIGINT NOT NULL,
       fileType INTEGER,
       uuid VARCHAR(64) NOT NULL,
       compressedSize BIGINT,
       uncompressedSize BIGINT,
       compressionType INTEGER,
       uncompressedHash VARCHAR(40),
       compressedHash VARCHAR(40),
       PRIMARY KEY(id, fileType),
       CONSTRAINT AttachedFiles1 FOREIGN KEY (id) REFERENCES Resources(internalId) ON DELETE CASCADE
       );              

CREATE TABLE Changes(
       seq BIGINT NOT NULL AUTO_INCREMENT,
       changeType INTEGER,
       internalId BIGINT NOT NULL,
       resourceType INTEGER,
       date VARCHAR(64),
       PRIMARY KEY(seq),
       CONSTRAINT Changes1 FOREIGN KEY (internalId) REFERENCES Resources(internalId) ON DELETE CASCADE
       );

CREATE TABLE ExportedResources(
       seq BIGINT NOT NULL AUTO_INCREMENT,
       resourceType INTEGER,
       publicId VARCHAR(64),
       remoteModality TEXT,
       patientId VARCHAR(64),
       studyInstanceUid TEXT,
       seriesInstanceUid TEXT,
       sopInstanceUid TEXT,
       date VARCHAR(64),
       PRIMARY KEY(seq)
       ); 

CREATE TABLE PatientRecyclingOrder(
       seq BIGINT NOT NULL AUTO_INCREMENT,
       patientId BIGINT NOT NULL,
       PRIMARY KEY(seq),
       CONSTRAINT PatientRecyclingOrder1 FOREIGN KEY (patientId) REFERENCES Resources(internalId) ON DELETE CASCADE
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
-- End of differences



-- NB: Character "@" is used to replace the semicolon characters in triggers

-- In MySQL, this trigger is only used if replacing some attachment
CREATE TRIGGER AttachedFileDeleted
AFTER DELETE ON AttachedFiles
FOR EACH ROW
BEGIN
  INSERT INTO DeletedFiles VALUES(old.uuid, old.filetype, old.compressedSize,
                                  old.uncompressedSize, old.compressionType,
                                  old.uncompressedHash, old.compressedHash)@
END;


CREATE TRIGGER ResourceDeleted
BEFORE DELETE ON Resources   -- WARNING: Must be "BEFORE", otherwise the attached file is already deleted
FOR EACH ROW
BEGIN
   INSERT INTO DeletedFiles SELECT uuid, fileType, compressedSize, uncompressedSize, compressionType, uncompressedHash, compressedHash FROM AttachedFiles WHERE id=old.internalId@
END;


CREATE TRIGGER PatientAdded
AFTER INSERT ON Resources
FOR EACH ROW
BEGIN
  IF new.resourceType = 0 THEN  -- The "0" corresponds to "OrthancPluginResourceType_Patient"
    INSERT INTO PatientRecyclingOrder VALUES (NULL, new.internalId)@
  END IF@
END;
