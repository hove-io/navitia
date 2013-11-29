********
Navitia
********

Used libs
=========
#. Boost

#. Protocol buffers

#. Fastlz

TODO presentation of the python libs

CMake use
=========
With CMake you can build in a repository different than the source directory.
By convention, you can have one build repository for each kind of build : 

    #. Debug directory 

           ``mkdir debug; cd debug; cmake -D BUILD_TYPE=Debug ../source``

    #. Release directory 

           ``mkdir release; cd release; cmake -D BUILD_TYPE=Release ../source``
