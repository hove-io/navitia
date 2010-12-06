#include <iostream>
#include "mssql.h"
#include <boost/foreach.hpp>

//int err_handler(DBPROCESS*, int, int, int, char*, char*);
//int msg_handler(DBPROCESS*, DBINT, int, int, char*, char*, char*, int);


int main(int, char**){
    Sql::MSSql conn("10.2.0.16", "sa", "159can87*", "testTDS");
    Sql::Result res = conn.exec("SELECT * from foo");
/*    std::cout << res.num_cols() << std::endl;
    Sql::Result::iterator it = res.begin();
    std::cout << (*it)[0] << " " << (*it)[1];*/
//    BOOST_FOREACH(Sql::StringVect row, res){
    for(Sql::Result::iterator it = res.begin(); it != res.end(); ++it){    
      /*  Sql::StringVect row = *it;
        if(row.size()<=0){
            std::cout << "y'a plus rien" << std::endl;
//            return 1;
        }
        else
            std::cout << row[0] << " " << row[1] << std::endl;*/
    }
//    DBSETLUSER(login, "sa");
  //  DBSETLPWD(login, "159can87*");

/*    if ((dbproc = dbopen(login, "10.2.0.16")) == NULL) {
        std::cerr << "unable to connect" << std::endl;
        return 1;
    }
    std::cout << "Connecté au serveur" << std::endl;

    if (erc = dbuse(dbproc, "testTDS") == FAIL) {
        std::cerr << "unable to use to database" << std::endl;
        return 1;
    }
    std::cout << "Connecté à la base" << std::endl;

    if (erc = dbfcmd(dbproc, "CREATE TABLE foo(id int, name char(50))") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;


    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "INSERT INTO foo VALUES(1, \"aaa\")") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête " << erc << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "INSERT INTO foo VALUES(2, \"ccc\")") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "INSERT INTO foo VALUES(3, \"bbb\")") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "SELECT * from foo") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    while ((erc = dbresults(dbproc)) != NO_MORE_RESULTS) {
        struct COL 						
        { 
            char *name; 
            char *buffer; 
            int type, size, status; 
        } *columns, *pcol;
        int ncols;
        int row_code;

        if (erc == FAIL) {
            std::cerr << "Dbresults failed" << std::endl;
            return 1;
        }

        ncols = dbnumcols(dbproc);

        if ((columns = (COL*)calloc(ncols, sizeof(struct COL))) == NULL) {
            return 1;
        }
*/
        /* 
         * Read metadata and bind.  
         */
  /*      for (pcol = columns; pcol - columns < ncols; pcol++) {
            int c = pcol - columns + 1;

            pcol->name = dbcolname(dbproc, c);		
                pcol->type = dbcoltype(dbproc, c);
            pcol->size = dbcollen(dbproc, c);

            if (SYBCHAR != pcol->type) {			
                pcol->size = dbwillconvert(pcol->type, SYBCHAR);
            }

            std::cout << pcol->size << " " << pcol->name << std::endl;

            if ((pcol->buffer = (char*)calloc(1, pcol->size + 1)) == NULL){
                return 1;
            }

            erc = dbbind(dbproc, c, NTBSTRINGBIND,	
                    pcol->size+1, (BYTE*)pcol->buffer);
            if (erc == FAIL) {
                std::cerr << "dbbind failed" << c << std::endl;
                return 1;
            }
            erc = dbnullbind(dbproc, c, &pcol->status);	
                if (erc == FAIL) {
                    std::cerr << "dnullbind failed " << c << std::endl;
                    return 1;
                }
        }
*/
        /* 
         * Print the data to stdout.  
         */
/*        while ((row_code = dbnextrow(dbproc)) != NO_MORE_ROWS){
            switch (row_code) {
                case REG_ROW:
                    for (pcol=columns; pcol - columns < ncols; pcol++) {
                        if(pcol->status == -1)
                            std::cout << "NULL" << std::endl;
                        else
                            std::cout << pcol->buffer << std::endl;
                    }
                    break;

                case BUF_FULL:
                    break;

                case FAIL:
                    std::cerr << "dbresults failed" << std::endl;
                    return 1;
                    break;

                default: 					
                    std::cout << " Data ignoreds for computeid " << row_code << std::endl;
            }


        }
*/
        /* free metadata and data buffers */
 /*       for (pcol=columns; pcol - columns < ncols; pcol++) {
            free(pcol->buffer);
        }
        free(columns);
*/
        /* 
         * Get row count, if available.   
         */
  /*      if (DBCOUNT(dbproc) > -1)
            std::cerr << DBCOUNT(dbproc) << " rows affected" << std::endl;
*/
        /* 
         * Check return status 
         */
      /*  if (dbhasretstat(dbproc) == TRUE) {
            std::cout << "Procedure returned " << dbretstatus(dbproc) << std::endl;
        }
    }
*/
    return 0;
}
/*int

msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity,
        char *msgtext, char *srvname, char *procname, int line)
{
    enum {changed_database = 5701, changed_language = 5703 };

    if (msgno == changed_database || msgno == changed_language)
        return 0;

    if (msgno > 0) {
        std::cerr << "Msg "<< msgno << ", Level " << severity <<  ", State " << msgstate << std::endl;


        if (strlen(srvname) > 0)
            std::cerr << "Server " << srvname << std::endl;
        if (strlen(procname) > 0)
            std::cerr << "Procedure " << procname << std::endl;
        if (line > 0)
            std::cerr << "Line " << line << std::endl;
    }
    std::cout << msgtext << std::endl;

    if (severity > 10) {
        std::cerr <<  "error: severity " << severity << " > 10, exiting" << std::endl;

        return severity;
    }

    return 0;
}

    int
err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, 
        char *dberrstr, char *oserrstr)
{									
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
*/

