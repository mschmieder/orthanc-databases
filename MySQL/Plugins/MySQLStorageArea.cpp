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


#include "MySQLStorageArea.h"

#include "../../Framework/MySQL/MySQLDatabase.h"
#include "../../Framework/MySQL/MySQLTransaction.h"

#include <Core/Logging.h>

#include <boost/math/special_functions/round.hpp>


namespace OrthancDatabases
{
  IDatabase* MySQLStorageArea::OpenInternal()
  {
    std::auto_ptr<MySQLDatabase> db(new MySQLDatabase(parameters_));

    db->Open();

    if (parameters_.HasLock())
    {
      db->AdvisoryLock(43 /* some arbitrary constant */);
    }

    {
      MySQLTransaction t(*db);

      int64_t size;
      if (db->LookupGlobalIntegerVariable(size, "max_allowed_packet"))
      {
        int mb = boost::math::iround(static_cast<double>(size) /
                                     static_cast<double>(1024 * 1024));
        LOG(WARNING) << "Your MySQL server cannot "
                     << "store DICOM files larger than " << mb << "MB";
        LOG(WARNING) << "  => Consider increasing \"max_allowed_packet\" "
                     << "in \"my.cnf\" if this limit is insufficient for your use";
      }
      else
      {
        LOG(WARNING) << "Unable to auto-detect the maximum size of DICOM "
                     << "files that can be stored in this MySQL server";
      }
               
      if (clearAll_)
      {
        db->Execute("DROP TABLE IF EXISTS StorageArea", false);
      }

      db->Execute("CREATE TABLE IF NOT EXISTS StorageArea("
                  "uuid VARCHAR(64) NOT NULL PRIMARY KEY,"
                  "content LONGBLOB NOT NULL,"
                  "type INTEGER NOT NULL)", false);

      t.Commit();
    }

    return db.release();
  }


  MySQLStorageArea::MySQLStorageArea(const MySQLParameters& parameters) :
    StorageBackend(new Factory(*this)),
    parameters_(parameters),
    clearAll_(false)
  {
  }
}
