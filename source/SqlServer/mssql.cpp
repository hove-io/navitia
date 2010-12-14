#include "mssql.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace Sql {

    /** Callback utilisé pour remonter les erreurs de la lib freeTDS */
    int err_handler(DBPROCESS *, int severity, int dberr, int, char *dberrstr, char *) {
        std::cout << "ERR HANDLER" << std::endl;
        if (dberr) {							
            std::cerr << "Error " << dberr << ", level " << severity << std::endl;
            std::cerr << dberrstr << std::endl;
        }

        else {
            std::cerr << "Lib error" << std::endl;
            std::cerr << dberrstr << std::endl;
        }
        return 0;
    }

    /** Callback utilisé pour remonter les messages de la lib freeTDS */
    int msg_handler(DBPROCESS *, DBINT msgno, int msgstate, int severity,
            char *msgtext, char *srvname, char *procname, int line)
    {
        std::cout << "MSG HANDLER " << msgno << " " << msgtext << std::endl;
        enum {changed_database = 5701, changed_language = 5703 };	

        if (msgno == changed_database || msgno == changed_language) {
            return 0;
        }

        if (msgno > 0) {
            fprintf(stderr, "Msg %ld, Level %d, State %d\n", 
                    (long) msgno, severity, msgstate);

            if (strlen(srvname) > 0)
                fprintf(stderr, "Server '%s', ", srvname);
            if (strlen(procname) > 0)
                fprintf(stderr, "Procedure '%s', ", procname);
            if (line > 0)
                fprintf(stderr, "Line %d", line);

            fprintf(stderr, "\n\t");
        }
        fprintf(stderr, "MSG %s\n", msgtext);

        if (severity > 10) {						
            fprintf(stderr, "%s: error: severity %d > 10, exiting\n", 
                    "bleh", severity);
            exit(severity);
        }
        return 0;							
    }


    RegisteredProcedure::RegisteredProcedure(DBPROCESS * dbproc, const std::string & procedure_name) : dbproc(dbproc) {
        std::string * strp = new std::string(procedure_name);
        strings.push_back(strp);
        if(dbrpcinit(dbproc, const_cast<char*>(strp->c_str()), (DBSMALLINT)0) == FAIL)
            throw SqlException("Unable to initialize rpc");
    }


    RegisteredProcedure::~RegisteredProcedure() {
        for(std::vector<int *>::iterator it = integers.begin(); it != integers.end(); it++)
            delete *it;
        integers.clear();
        for(std::vector<std::string *>::iterator it = strings.begin(); it != strings.end(); it++)
            delete *it;
        strings.clear();
    }

    RegisteredProcedure & RegisteredProcedure::arg(int  i){
        integers.push_back(new int(i));
        if (dbrpcparam(dbproc, NULL, (BYTE)0, SYBINT4, -1, -1, (BYTE *)integers.back())== FAIL)
            throw SqlException("Unable to add parameter to registered procedure call");
        return *this;
    }

    RegisteredProcedure & RegisteredProcedure::arg(const char * str){
        std::string * strp = new std::string(str);
        strings.push_back(strp);
        if (dbrpcparam(dbproc, NULL, (BYTE)0, SYBCHAR, -1, strlen(str), (BYTE *)(strp->c_str()))== FAIL)
            throw SqlException("Unable to add parameter to registered procedure call");
        return *this;
    }


    MSSql::MSSql(const std::string & host, const std::string & name, const std::string & passwd, const std::string & db){
        dberrhandle(err_handler);
        dbmsghandle(msg_handler);

        if (dbinit() == FAIL)
            throw SqlException("dbinit() failed");


        if ((login = dblogin()) == NULL) 
            throw SqlException("Unable to allocate login structure");

        DBSETLUSER(login, name.c_str());
        DBSETLPWD(login, passwd.c_str());
        //login->tds_login->major_version = 8;
        //login->tds_login->minor_version = 0;

        if ((dbproc = dbopen(login, host.c_str())) == NULL) 
            throw SqlException("Unable to connect to server");

        if (dbuse(dbproc, db.c_str()) == FAIL) 
            throw SqlException("Unable to use database");
    }

    Result MSSql::exec(const std::string & query){
        if(dbcancel(dbproc) == FAIL)
            std::cerr << "Unable to cancel" << std::endl;
        if (dbfcmd(dbproc, query.c_str()) == FAIL) 
            throw SqlException("Unable to build request");
        if (dbsqlexec(dbproc) == FAIL)
            throw SqlException("Unable to execute request");

        return Result(dbproc);
    }

    RegisteredProcedure MSSql::exec_proc(const std::string & procedure_name){
        return RegisteredProcedure(dbproc, procedure_name);
    }

    void Result::init() {
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

            if ((pcol->buffer = (char*)calloc(1, pcol->size + 1)) == NULL)
                throw SqlException("Unable to allocate memory for column data");
            if(dbbind(dbproc, c, NTBSTRINGBIND,	pcol->size+1, (BYTE*)pcol->buffer) == FAIL)
                throw SqlException("dbbind failed");
            if(dbnullbind(dbproc, c, &pcol->status) == FAIL)	
                throw SqlException("dnullbind failed");
        }
    }

    Result::Result(DBPROCESS * dbproc) : dbproc(dbproc){
        if(dbresults(dbproc) == FAIL)
            throw SqlException("Unable to get results");
        ncols = dbnumcols(dbproc);

        init();
    }

    
    Result & Result::operator=(const Result & result) {
        if(&result != this){
            cleanup();
            ncols = result.ncols;
            dbproc = result.dbproc;

            init();
        }
        return *this;
    }

    void Result::init(const RegisteredProcedure & proc){
        dbproc = proc.dbproc;
        dbcancel(dbproc);
        if(dbrpcsend(dbproc) == FAIL)
            throw SqlException("Unable to send registered procedure");
        if(dbresults(dbproc) == FAIL)
            throw SqlException("Unable to get results from registered procedure");
        ncols = dbnumcols(dbproc);
        init();
    }


    Result::Result(const RegisteredProcedure & proc){
        init(proc);
    }

    Result & Result::operator=(const RegisteredProcedure & proc){
        cleanup();
        init(proc);
        return *this;
    }

    void Result::cleanup() {
        for (pcol=columns; pcol - columns < ncols; pcol++) {
            free(pcol->buffer);
        }
        free(columns);
    }

    Result::~Result(){
        cleanup();
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
                    result = NULL;
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
                default:
                    std::cout << "WTF ??" << std::endl;
            }
            read_rows++;
        }
    }


    MSSql::~MSSql() {
        dbloginfree(login);
        dbclose(dbproc);
        dbexit();
    }
}
