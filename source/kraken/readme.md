# Kraken
Kraken is the public transport trip planner and much more, it can provide schedules, timetables and expose public
transport data via
[PTReferential](https://github.com/CanalTP/navitia/blob/dev/documentation/rfc/ptref_grammar.md).
It also provide:
  - (bad) street network routing that will be replaced by [asgard](https://github.com/CanalTP/asgard)
  - autocomplete that will be replaced by [mimir](https://github.com/CanalTP/mimirsbrunn)


## Configuration
Kraken can be configured by multiple way:
  - configuration file
  - command line arguments
  - environment variables

### Configuration file
Kraken will try to load a configuration file named the same way than the binary with a `.ini` prefix in the
current directory, so by default it will search for a `kraken.ini` file. Alternatively the file can be passed as
the first positional argument.

Example of configuration file, the values are the defaults:
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
# enable loading of realtime data that add stop_times
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
# ulimit that define the maximum size of a core file<Paste>
core_file_size_limit = 0
# log level, mostly use when configurating kraken by cli or envvar
log_level =
# log format, mostly use when configurating kraken by cli or envvar
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
# realtime contributors to listen, repeat the line to add multiple contributor
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
The parameters are based on the configuration file, they have the following form: `--section.option value`
By example if we want to define the database we have to do ` --GENERAL.database database.nav.lz4`.

### Environment variables
The environment variables are based on the configuration file, they have the following form:
`KRAKEN_SECTION_OPTION`
By example if we want to define the number of thread we have to set ` GENERAL_nb_threads=1`.


## Startup

At startup kraken do the following actions:
1. load configuration
2. open connection to rabbitmq
3. load data.nav.lz4
4. load disruptions from chaos's database
5. load realtime from kirin
6. start background thread that handle disruptions, realtime and data reloading
7. bind zmq sockets: at this point the configured tcp port start acceptint connection
8. start worker threads to handle resquests

## Data loading

## Realtime integration

## Request handling
