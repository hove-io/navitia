#include "mssql.h"

#include <iostream>
#include <cstdlib>
namespace Sql {
    MSSql::MSSql(const std::string & host, const std::string & name, const std::string & passwd, const std::string & db){
        //    dbmsghandle(msg_handler);

        if (dbinit() == FAIL)
            throw SqlException("dbinit() failed");

        if ((login = dblogin()) == NULL) 
            throw SqlException("Unable to allocate login structure");

        DBSETLUSER(login, name.c_str());
        DBSETLPWD(login, passwd.c_str());

        if ((dbproc = dbopen(login, host.c_str())) == NULL) 
            throw SqlException("Unable to connect to server");

        if (dbuse(dbproc, db.c_str()) == FAIL) 
            throw SqlException("Unable to use database");
    }

    Result MSSql::exec(const std::string & query){
        if (dbfcmd(dbproc, query.c_str()) == FAIL) 
            throw SqlException("Unable to build request");
        if (dbsqlexec(dbproc) == FAIL)
            throw SqlException("Unable to execute request");

        Result res(dbproc);
        return res;
    }
    
    Result::Result(DBPROCESS * dbproc) : dbproc(dbproc){
        if(dbresults(dbproc) == FAIL)
            throw SqlException("Unable to get results");

        ncols = dbnumcols(dbproc);
        if ((columns = (COL*)calloc(ncols, sizeof(struct COL))) == NULL) 
            throw SqlException("Unable to allocate memory for memory data");

        for (pcol = columns; pcol - columns < ncols; pcol++) {
            int c = pcol - columns + 1;

            pcol->name = dbcolname(dbproc, c);		
            pcol->type = dbcoltype(dbproc, c);
            pcol->size = dbcollen(dbproc, c);

            if (SYBCHAR != pcol->type) {			
                pcol->size = dbwillconvert(pcol->type, SYBCHAR);
            }

            std::cout << pcol->size << " " << pcol->name << std::endl;

            if ((pcol->buffer = (char*)calloc(1, pcol->size + 1)) == NULL)
                throw SqlException("Unable to allocate memory for column data");
            if(dbbind(dbproc, c, NTBSTRINGBIND,	pcol->size+1, (BYTE*)pcol->buffer) == FAIL)
                throw SqlException("dbbind failed");
            if(dbnullbind(dbproc, c, &pcol->status) == FAIL)	
                throw SqlException("dnullbind failed");
        }

    }

    Result::~Result(){
        for (pcol=columns; pcol - columns < ncols; pcol++) {
            free(pcol->buffer);
        }
        free(columns);
    }

    void Result::iterator::get_row() {
        data.clear();
        if(result == NULL)
            throw SqlException("Unable to get a result");
        else {
            int row_code = dbnextrow(result->dbproc);
            switch (row_code) {
                case FAIL:
                    throw SqlException("Unable to get next row");
                    break;
                case NO_MORE_ROWS:
                    result == NULL;
                    std::cout << "result est null" << std::endl;
                    break;
                case REG_ROW:
                    data.reserve(result->ncols);
                    for (result->pcol=result->columns; result->pcol - result->columns < result->ncols; result->pcol++) {
                        if(result->pcol->status == -1)
                            data.push_back("NULL");
                        else
                            data.push_back(result->pcol->buffer);
                    }
                    break;

                case BUF_FULL:
                    throw SqlException("Buffer full, can't get row");
                    break;
            }
            read_rows++;
        }
    }


    MSSql::~MSSql() {
        dbloginfree(login);
        dbclose(dbproc);
        dbexit();
    }
};
