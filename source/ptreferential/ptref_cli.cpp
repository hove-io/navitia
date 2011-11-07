#include <iostream>
#include "ptreferential.h"
#include "data.h"
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>



using namespace navitia::type;

int main(int argc, char** argv){
    std::cout << "    _   _____ _    ___ _______ ___ \n"
                 "   / | / /   | |  / (_)_  __(_)   |\n"
                 "  /  |/ / /| | | / / / / / / / /| |\n"
                 " / /|  / ___ | |/ / / / / / / ___ |\n"
                 "/_/ |_/_/  |_|___/_/ /_/ /_/_/  |_| v0.0.0\n\n"
                 "    ____  ______          ____                     __  _       __\n"
                 "   / __ \\/_  __/_______  / __/__  ________  ____  / /_(_)___ _/ /\n"
                 "  / /_/ / / / / ___/ _ \\/ /_/ _ \\/ ___/ _ \\/ __ \\/ __/ / __ `/ /\n"
                 " / ____/ / / / /  /  __/ __/  __/ /  /  __/ / / / /_/ / /_/ / /\n"
                 "/_/     /_/ /_/   \\___/_/  \\___/_/   \\___/_/ /_/\\__/_/\\__,_/_/\n\n";
    std::cout << "Chargement des données..." << std::flush;
    Data d;
    d.load_flz("idf_flz.nav");
    std::cout << " effectué" << std::endl << std::endl;

    std::cout
            << "Statistiques :" << std::endl
            << "    Nombre de StopAreas : " << d.pt_data.stop_areas.size() << std::endl
            << "    Nombre de StopPoints : " << d.pt_data.stop_points.size() << std::endl
            << "    Nombre de lignes : " << d.pt_data.lines.size() << std::endl
            << "    Nombre d'horaires : " << d.pt_data.stop_times.size() << std::endl << std::endl;


    if(argc == 1) {
        static char *line_read = (char *)NULL;
        for(;;){
            if (line_read)
              {
                free (line_read);
                line_read = (char *)NULL;
              }

            /* Get a line from the user. */
            line_read = readline ("NAViTiA> ");

            /* If the line has any text in it,
               save it on the history. */
            if (line_read && *line_read)
            {
                if( strcmp(line_read, "exit") == 0 || strcmp(line_read, "quit") == 0)
                {
                    std::cout << "\n Bye! See you soon!" << std::endl;
                    exit(0);
                }
                add_history (line_read);
                pbnavitia::PTReferential result = navitia::ptref::query(line_read, d.pt_data);
                std::cout << "octets généré en protocol buffer: " << result.ByteSize() << std::endl << std::endl;
                std::cout << navitia::ptref::pb2txt(&result);
            }

        }
    }
    else if (argc == 2){
        pbnavitia::PTReferential result = navitia::ptref::query(argv[1], d.pt_data);
        std::cout << "octets généré en protocol buffer: " << result.ByteSize() << std::endl;
        std::cout << navitia::ptref::pb2txt(&result);
    }
    else {
        std::cout << "Il faut exactement zéro ou un paramètre" << std::endl;
    }
    return 0;
}
