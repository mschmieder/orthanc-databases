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

#if ORTHANC_ENABLE_POSTGRESQL != 1
#  error PostgreSQL support must be enabled to use this file
#endif

#include "PostgreSQLStatement.h"

namespace OrthancDatabases
{
  class PostgreSQLResult : public boost::noncopyable
  {
  private:
    void                *result_;  /* Object of type "PGresult*" */
    int                  position_;
    PostgreSQLDatabase&  database_;
    unsigned int         columnsCount_;

    void Clear();

    void CheckDone();

    void CheckColumn(unsigned int column, /*Oid*/ unsigned int expectedType) const;

  public:
    explicit PostgreSQLResult(PostgreSQLStatement& statement);

    ~PostgreSQLResult();

    void Next();

    bool IsDone() const
    {
      return result_ == NULL;
    }

    unsigned int GetColumnsCount() const
    {
      return columnsCount_;
    }

    bool IsNull(unsigned int column) const;

    bool GetBoolean(unsigned int column) const;

    int GetInteger(unsigned int column) const;

    int64_t GetInteger64(unsigned int column) const;

    std::string GetString(unsigned int column) const;

    void GetLargeObject(std::string& result,
                        unsigned int column) const;

    void GetLargeObject(void*& result,
                        size_t& size,
                        unsigned int column) const;

    IValue* GetValue(unsigned int column) const;
  };
}
