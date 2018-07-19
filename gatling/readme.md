To test the load and performance of the different Navitia scenarios, the framework Gatling is used

How to run
===========

Gatling can be downloaded from the gatling.io page and is run with an executable script.
Here's an example of command to run the Navitia tests:
```
<PATHTO>/bin/gatling.sh -df <PATHTO>/navitia/gatling/data -sf <PATHTO>/navitia/gatling/simulations
```
This command will show a list of simulations, based on the file found in the folder "simulations". In the command above, it is defined with the argument -sf (note that there can be default simulations found in the downloaded gatling folder).
A file with the arguments of the requests to run is provided with the argument -df.

Once executed, select the simulation number and the next steps can be skipped in our case:'simulation id' and 'description' (can be used to specify the commit id tested)

At the end of the simulation, a stats html page is generated.

The navitia simulations require to have running coverage (Jormungandr and Kraken) locally.
Note: to reproduce a behaviour similar to the prod platform, it is necessary to launch several processes of Jormun using uwsgi.

How to add tests
================

The requests done for the simulations are '/journeys'. You can find in the data file (argument -df in the command above) the data needed for the requests. To add a test, add a line in the data file filling each argument as described in the first line of the file.
