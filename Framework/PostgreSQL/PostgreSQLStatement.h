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

#include "../Common/IPrecompiledStatement.h"
#include "../Common/Query.h"
#include "../Common/GenericFormatter.h"

#include "PostgreSQLDatabase.h"
#include "PostgreSQLLargeObject.h"
#include "PostgreSQLTransaction.h"

#include <vector>
#include <memory>
#include <boost/shared_ptr.hpp>

namespace OrthancDatabases
{
  class PostgreSQLStatement : public IPrecompiledStatement
  {
  private:
    class ResultWrapper;
    class Inputs;
    friend class PostgreSQLResult;

    PostgreSQLDatabase& database_;
    bool readOnly_;
    std::string id_;
    std::string sql_;
    std::vector<unsigned int /*Oid*/>  oids_;
    std::vector<int>  binary_;
    boost::shared_ptr<Inputs> inputs_;
    GenericFormatter formatter_;

    void Prepare();

    void Unprepare();

    void DeclareInputInternal(unsigned int param,
                              unsigned int /*Oid*/ type);

    void* /* PGresult* */ Execute();

  public:
    PostgreSQLStatement(PostgreSQLDatabase& database,
                        const std::string& sql,
                        bool readOnly);

    PostgreSQLStatement(PostgreSQLDatabase& database,
                        const Query& query);

    ~PostgreSQLStatement();
    
    virtual bool IsReadOnly() const
    {
      return readOnly_;
    }

    void DeclareInputInteger(unsigned int param);
    
    void DeclareInputInteger64(unsigned int param);

    void DeclareInputString(unsigned int param);

    void DeclareInputBinary(unsigned int param);

    void DeclareInputLargeObject(unsigned int param);

    void Run();

    void BindNull(unsigned int param);

    void BindInteger(unsigned int param, int value);

    void BindInteger64(unsigned int param, int64_t value);

    void BindString(unsigned int param, const std::string& value);

    void BindLargeObject(unsigned int param, const PostgreSQLLargeObject& value);

    PostgreSQLDatabase& GetDatabase() const
    {
      return database_;
    }

    IResult* Execute(ITransaction& transaction,
                     const Dictionary& parameters);

    void ExecuteWithoutResult(ITransaction& transaction,
                              const Dictionary& parameters);
  };
}
