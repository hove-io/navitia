#include <iostream>
#include <sqlfront.h>	/* sqlfront.h always comes first */
#include <sybdb.h>	/* sybdb.h is the only other file you need */

int err_handler(DBPROCESS*, int, int, int, char*, char*);
int msg_handler(DBPROCESS*, DBINT, int, int, char*, char*, char*, int);


int main(int, char**){
    int i, ch;
    LOGINREC *login;
    DBPROCESS *dbproc;
    dbmsghandle(msg_handler);

    RETCODE erc;
    if (dbinit() == FAIL) {
        std::cerr << "dbinit() failed" << std::endl;
        return 1;
    }
    std::cout << "Initialisé" << std::endl;

    if ((login = dblogin()) == NULL) {
        std::cerr << "unable to allocate login structure" << std::endl;
        return 1;
    }
    std::cout << "login créé" << std::endl;

    DBSETLUSER(login, "sa");
    DBSETLPWD(login, "159can87*");
    //   DBSETLHOST(login, "10.2.0.16");

    if ((dbproc = dbopen(login, "10.2.0.16")) == NULL) {
        std::cerr << "unable to connect" << std::endl;
        return 1;
    }
    std::cout << "Connecté au serveur" << std::endl;

    if (erc = dbuse(dbproc, "testTDS") == FAIL) {
        std::cerr << "unable to use to database" << std::endl;
        return 1;
    }
    std::cout << "Connecté à la base" << std::endl;

    if (erc = dbfcmd(dbproc, "CREATE TABLE 'foo'('id' int, 'name' char)") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;


    if (erc = dbfcmd(dbproc, "CREATE TABLE \"foo\"(\"id\" int, \"name\" char)") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "INSERT INTO \"foo\" VALUES(1, \"aaa\")") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête " << erc << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "INSERT INTO \"foo\" VALUES(2, \"ccc\")") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;

    if (erc = dbfcmd(dbproc, "INSERT INTO \"foo\" VALUES(3, \"bbb\")") == FAIL) {
        std::cerr << "Impossible de créer la requête" << std::endl;
        return 1;
    }
    std::cout << "Requête créée" << std::endl;

    if ((erc = dbsqlexec(dbproc)) == FAIL) {
        std::cerr << "Impossible d'éxecuter la requête" << std::endl;
    }
    std::cout << "Requête éxécutée" << std::endl;
    return 0;
}
int

msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity,
            char *msgtext, char *srvname, char *procname, int line)
{
        enum {changed_database = 5701, changed_language = 5703 };

        if (msgno == changed_database || msgno == changed_language)
                return 0;

        if (msgno > 0) {
            std::cerr << "Msg "<< msgno << ", Level " << severity <<  "State " << msgstate << std::endl;


                /*if (strlen(srvname) > 0)
                        fprintf(stderr, "Server '%s', ", srvname);
                if (strlen(procname) > 0)
                        fprintf(stderr, "Procedure '%s', ", procname);
                if (line > 0)
                        fprintf(stderr, "Line %d", line);

                fprintf(stderr, "\n\t");*/
        }
        std::cout << msgtext << std::endl;

        if (severity > 10) {
            std::cerr <<  "error: severity " << severity << " > 10, exiting" << std::endl;

                return severity;
        }

        return 0;
}

