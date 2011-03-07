
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>

using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;


void printDebug(){
    Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("foo"));
    LOG4CPLUS_DEBUG(logger, "This is a DEBUG message");
    LOG4CPLUS_INFO(logger, "This is a INFO message");
    LOG4CPLUS_WARN(logger, "This is a WARN message");
    LOG4CPLUS_ERROR(logger, "This is a ERROR message");
    LOG4CPLUS_FATAL(logger, "This is a FATAL message");
}


int main(){
    cout << "Entering main()..." << endl;
    LogLog::getLogLog()->setInternalDebugging(true);
    Logger root = Logger::getRoot();
    try {
        PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("log4cplus.ini"));

        LOG4CPLUS_WARN(root, "Testing....");
        printDebug();
    }catch(...) {
        cout << "Exception..." << endl;
        LOG4CPLUS_FATAL(root, "Exception occured...");
    }

    cout << "Exiting main()..." << endl;
    return 0;
}

