# Kraken
Kraken is a public transport trip planner and much more, it provides schedules, timetables and expose public
transport data via
[PTReferential](https://github.com/hove-io/navitia/blob/dev/documentation/rfc/ptref_grammar.md).<br>It also provides:
  - (bad) street network routing that will be replaced by [asgard](https://github.com/hove-io/asgard)
  - autocomplete that will be replaced by [mimir](https://github.com/hove-io/mimirsbrunn)


## Configuration
Kraken can be configured in multiple ways:
  - configuration file
  - command line arguments
  - environment variables

### Configuration file
Kraken will try to load a configuration file named the same way as the binary with a `.ini` prefix in the
current directory, so by default it will search for a `kraken.ini` file. Alternatively the file path can be passed in as
the first positional argument.

Example of configuration file, defaults values are:
```
[GENERAL]
# Path to data file
database = data.nav.lz4
# zmq socket to bind, see: http://api.zeromq.org/3-2:zmq-connect (required)
zmq_socket =
# name of the coverage (required)
instance_name =
# number of thread created to serve resquests
nb_threads = 1

# enable loading of realtime data
is_realtime_enabled = false
# enable loading of realtime data that add stop_times (only if is_realtime_enabled is activated)
is_realtime_add_enabled = false
# enable loading of realtime data that add trips (only if is_realtime_add_enabled is activated)
is_realtime_add_trip_enabled = false
# timeout in ms for loading realtime data from kirin
kirin_timeout = 60000
# timeout in ms before retrying to load realtime data
kirin_retry_timeout = 300000
# request running a least this number of milliseconds are logged
slow_request_duration = 1000
# display all contributors in feed publishers
display_contributors = True
# number of cache raptor to keep at most. improve performances by increasing memory usage
raptor_cache_size = 10
# binding for metrics http server, format: IP:PORT
metrics_binding =
# ulimit that defines the maximum size of a core file<Paste>
core_file_size_limit = 0
# log level, mostly used when configurating kraken by cli or envvar
log_level =
# log format, mostly used when configurating kraken by cli or envvar
log_format = [%D{%y-%m-%d %H:%M:%S,%q}] [%p] [%x] - %m %b:%L  %n


# configuration of logs, passed directly to log4cplus
# see: https://log4cplus.sourceforge.io/docs/html/classlog4cplus_1_1PropertyConfigurator.html#a21e8e6b1440cc7a8a47b8fd14c54b239
[LOG]
log4cplus.rootLogger= DEBUG, ALL_MSGS
log4cplus.appender.ALL_MSGS=log4cplus::ConsoleAppender
log4cplus.appender.ALL_MSGS.layout=log4cplus::PatternLayout
log4cplus.appender.ALL_MSGS.layout.ConversionPattern=[%D{%y-%m-%d %H:%M:%S,%q}] [%p] [%x] - %m %b:%L  %n

# configuration of rabbitmq
[BROKER]
host = localhost
port = 5672
username = guest
password = guest
vhost = /
exchange = navitia
# realtime contributors to listen, repeat the line to add multiple contributors
rt_topics =

# should we delete the queue at shutdown, useful only for tests
queue_auto_delete = false
# timeout for waiting messages from rabbitmq
timeout = 100
# time to wait between each poll of rabbitmq in seconds (should not be updated)
sleeptime = 1
#rabbitmq's queue name to be bound, only used for tests
queue =


[CHAOS]
# connection string to the chaos database, format: host=localhost user=navitia password=navitia dbname=chaos
database =
```
### CLI arguments
Parameters are based on the configuration file with the following form: `--section.option value`
By example if we want to define the database we have to do `--GENERAL.database database.nav.lz4`.

### Environment variables
The environment variables are based on the configuration file, they have the following form:
`KRAKEN_SECTION_OPTION`
By example if we want to define the number of thread we have to set `GENERAL_nb_threads=1`.


## Startup

At startup kraken do the following actions:
1. load configuration
2. open connection to rabbitmq
3. Data loading
4. start background thread that handles disruptions, realtime and data reloading
5. bind zmq sockets: at this point the configured tcp port starts accepting connection
6. start worker threads to handle requests

## Data loading

Data loading is triggered by two events:
  - at startup
  - by an order received from rabbitmq

kraken starts by reading the `nav.lz4` file configured, it then applies disruptions by loading them from the chaos
database.
Kraken will do the following actions:

1. load data.nav.lz4
2. load disruptions from chaos's database
3. load realtime from kirin:
    1. kraken creates an anonymous queue in rabbitmq
    2. kraken sends a request to kirin via rabbitmq with the queue name as parameter
    3. kraken waits for a message in the queue
    4. kirin builds a "ntfs-rt" and sends it to the queue previously created
    5. kraken receives the message and apply the realtime data
4. build raptor
5. build relations
6. build proximitylist

When a data loading occurs after startup the process is the same but is done on another dataset, this means that
memory usage will double during reload.
There is no service interruption as we have two datasets in memory, there is no locking done to prevent blocking
requests. Swap of dataset is done by an atomic swap of pointer.

## Realtime integration

In this chapter, 'realtime' means any modification of the static data, hence disruptions from Chaos or
realtime_update from Kirin.

These data are received from rabbitmq, kraken will handle multiple messages in batch (up to 5000).
The process is very similar to a data reload, it is done in background on another dataset to prevent impacts
on requests.

The following actions are done:
1. clone `Data` to have a writable dataset, this is quite slow
2. update `Data` with the realtime data
3. build relations
4. build autocomplete
5. build raptor
6. build proximitylist
7. rebuild raptor cache from the previous Data
8. switch `Data`

Rebuilding raptor's cache is not strictly required, but it reduces the slowdown of the first few requests on the new
dataset.

## Request handling
The main thread executes the [`ZMQ LoadBalancer`](https://github.com/hove-io/utils/blob/master/zmq.h) that
dispatches requests to available worker threads, it only forwards the requests to a worker and responds to the client
once the worker have finished. There is no serialization done here, only (unneeded) copy of bytes.
The waiting queue is not handled by the load balancer, it won't accept a request if there is no worker available, the
queue is managed by zmq.
Communication between threads is done with [zmq inproc sockets](http://api.zeromq.org/2-1:zmq-inproc).

Workers threads start by registering themselves to the load balancer and start waiting for requests. Each thread
can only handle one request concurrently.

When a request is received the following actions occur:
1. read request and deserialize it
2. if enabled create a deadline object to terminate requests
3. acquire a readonly Data object that will be used for this request
4. check if the deadline has expired
5. initialize worker if needed: first request with a new dataset
    1. creation of raptor planner
    2. creation of streetnetwork planner
6. initialisation of `pb_creator` used to build the response
7. execute handler of this API
8. build response from `pb_creator`
9. respond to LB

In case of error the thread will respond with the error message, termination by deadline is handled like an
error.

### Raptor cache
The raptor cache is a structure shared between every raptor planner that contains an optimized
representation of stoptimes for a specific period of time. Every request that computes stoptimes (journeys,
departures, *_schedules) will use the raptor cache attached the starting day of the request (raptor cache only contains 48 hours of data to handle pass-midnight journeys)
hours of data.
These stoptimes are sorted and contiguous in memory to maximize cpu caching.

By default we can create at most 10 raptor caches, these are kept by a simple LRU that will discard "unused"
values. Creation of raptor cache is quite costly, it will take a few hundred milliseconds on somethings like
fr-idf.

The raptor cache is quite special in kraken's design as it is the only data structure writable by multiple
threads.

