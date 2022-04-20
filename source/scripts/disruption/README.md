# Disruptor

Create and send **disruption** inside navitia core. It can be usefull to debug Kraken.<br>
RabbitMq is used as a broker to exchange disruption messages.

## Set up

Disruptor depends on [chaos-proto](https://github.com/hove-io/chaos-proto) sources for disruption messages.

```
# Install protobuf compiler
sudo apt install protobuf-compiler

# compile chaos proto class with protoc
cd navitia_source_dir/source/chaos-proto
protoc chaos.proto gtfs-realtime.proto --python_out=.

# Install RabbitMQ, if it is not done yet
sudo apt install rabbitmq-server
```

Update your **kraken.ini** with broker and rt_topics

```
#############################
# BROKER
#############################
[BROKER]
host = localhost
port = 5672
username = guest
password = guest
exchange = navitia
vhost = /

rt_topics = shortterm.coverage_name
```

Compile Kraken and run it. You are ready to send disruption.<br>
Don't forget to run the script with the **Jormungandr virtualenv** for dependencies.

## Run

Disruptor has several options

### Run help

```
# PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -h

usage: disruption.py [-h] [-b BROKER_CONNECTION] [-e EXCHANGE] [-t TOPIC]
                     [-p PT_OBJECT] [-i IMPACT_TYPE] [-f FILE]
                     [--empty_disruption] [-s SLEEP] [-l]

optional arguments:
  -h, --help            show this help message and exit

  -b BROKER_CONNECTION, --broker_connection BROKER_CONNECTION
                        AMQ broker connection string (mandatory). Example: -b
                        pyamqp://guest:guest@localhost:5672

  -e EXCHANGE, --exchange EXCHANGE
                        AMQ exhange (mandatory). Example: -e navitia

  -t TOPIC, --topic TOPIC
                        AMQ topic (mandatory). Example: -t shortterm.coverage

  -p PT_OBJECT, --pt_object PT_OBJECT
                        The public transport object that you want to impact
                        (mandatory except with --empty_disruption parameter).
                        Allows one shot disruption.
                        Example : -p "Line('line:DUA:800853022')"

                        List of possibility:
                        Line('line_uri'),
                        LineSection('line_uri','stop_area_uri', 'stop_area_uri'),
                        RailSection('line_uri', 'stop_area_uri', 'stop_area_uri'),
                        StopArea('stop_area_uri'),
                        StopPoint('stop_point_uri'),
                        Route('route_uri'),
                        Network('network_uri')

  -i IMPACT_TYPE, --impact_type IMPACT_TYPE
                        Impact type for disruption :
                        NO_SERVICE,
                        REDUCED_SERVICE,
                        SIGNIFICANT_DELAYS,
                        DETOUR,
                        ADDITIONAL_SERVICE,
                        MODIFIED_SERVICE,
                        OTHER_EFFECT,
                        UNKNOWN_EFFECT.

                        By default = NO_SERVICE

  -f FILE, --file FILE  File to import an impacts list from json.
                        It is a simple json of PT object.
                        Here an example of what the file can contain:

                        {
                            "impacts": [
                                {
                                    "pt_object": "Line("line_uri"),
                                    "impact_type": "NO_SERVICE"
                                },
                                {
                                    "pt_object": LineSection("line_uri", "stop_area_uri", "stop_area_uri"),
                                    "impact_type": "NO_SERVICE"
                                },
                                {
                                    "pt_object": RailSection("line_uri", "stop_area_uri", "stop_area_uri"),
                                    "impact_type": "NO_SERVICE"
                                },
                                {
                                    "pt_object": StopArea("stop_area_uri"),
                                    "impact_type": "NO_SERVICE"
                                },
                                {
                                    "pt_object": StopPoint("stop_point_uri"),
                                    "impact_type": "NO_SERVICE"
                                },
                                {
                                    "pt_object": Route("route_uri"),
                                    "impact_type": "NO_SERVICE"
                                },
                                {
                                    "pt_object": Network(network_uri),
                                    "impact_type": "NO_SERVICE"
                                }
                            ]
                        }

  --empty_disruption    Create only empty disruption without impact. Allows
                        one shot disruption

  -d DISRUPTION_DURATION, --disruption_duration DISRUPTION_DURATION
                        Duration of disruption (minutes).
                        default=10min

  -s SLEEP, --sleep SLEEP
                        Sleep time between 2 disruptions in file mode (secondes).

                        default=10s

  -l, --loop_forever    loop forever on the impact list in file mode
```

### Examples

Run inside the **Jormungandr Virtualenv**

Simple **empty disruption**

```
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" --empty_disruption
```

Simple disruption **with pt object** impacted

```
# Line - impact No SERVICE
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -p "Line('line_uri')"

# Line section - impact NO SERVICE
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -p "LineSection('line_uri', 'stop_area_uri', 'stop_area_uri')"

# Rail section - impact NO SERVICE
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -p "RailSection('line_uri', 'stop_area_uri', 'stop_area_uri')"

# Route - impact REDUCED SERVICE
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -p "Route('route_uri')" -i "REDUCED_SERVICE"
```

Disruptions **with file**

Example of file (`impact.json <https://github.com/hove-io/navitia/blob/dev/source/scripts/disruption/impacts.json>`).<br>
You have to add real ID object. Values on the example file are indicative.

```
{
    "impacts": [
        {
            "pt_object": "LineSection('line:DUA:810801041', 'stop_area:DUA:SA:8775810', 'stop_area:DUA:SA:8738221')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "LineSection('line:DUA:810987658', 'stop_area:DUA:SA:8775657', 'stop_area:DUA:SA:7890861')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "LineSection('line:DUA:810806753', 'stop_area:DUA:SA:8876553', 'stop_area:DUA:SA:6252567')",
            "impact_type": "NO_SERVICE"
        },
            "pt_object": "RailSection('line:DUA:810806758', 'stop_area:DUA:SA:8876543', 'stop_area:DUA:SA:6252521')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "Line('line:DUA:800853022')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "Line('line:DUA:800999022')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "Route('route:DUA:80054622')",
            "impact_type": "REDUCED_SERVICE"
        },
        {
            "pt_object": "Route('route:DUA:800736022')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "StopPoint('stop_point:DUA:SP:93:1317')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "StopArea('stop_area:DUA:SA:8876553')",
            "impact_type": "NO_SERVICE"
        },
        {
            "pt_object": "Network('network:DUA855')",
            "impact_type": "NO_SERVICE"
        }
    ]
}
```

```
# play file with sleep = 5 secs between disruptions
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -f path_to_file -s 5

# play file with sleep = 5 secs between disruptions and loop forever
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -f path_to_file -s 5 -l
```

### Disruption in playground

You can find out your disruption inside the jormun response.<br>
Example:

```
# Add disruption on line 1 (line:DUA:100110001)
PYTHONPATH="/navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -p "Line('line:DUA:100110001')" -i "REDUCED_SERVICE"
```

The jormungandr response after the data reload

```
http://localhost:8080/v1/coverage/idfm/lines/line%3ADUA%3A100110001/lines?
```

![disruption_response](disruption_response.png)

## How to debug

In first step, when you have any troubles with the disruptions loading, please use the rabbitMQ management plugin (https://www.rabbitmq.com/management.html). It checks if the script send properly the disruption message.<br>
With a local installation, the default address is http://localhost:15672 (default user/password: guest/guest). You can easily monitor the corresponding queue.

### Verify your config file again

kraken.ini needs the same RabbitMQ parameters as the disruptor script.<br>
Exemple:

**kraken.ini**

```
[GENERAL]

# coverage
#database = /path/to/covarage/data.nav.lz4

# ipc socket
zmq_socket = ipc:///tmp/default_kraken

# number of threads
nb_threads = 1

# name of the instance
instance_name=default

##############################
# BROKER
#############################
[BROKER]
host = localhost
port = 5672
username = guest
password = guest
exchange = navitia
vhost = /

rt_topics = shortterm.coveragename
```

The script needs the following parameters

```
-b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.coveragename"
```

### Verify if disruptions are well received within Kraken

Just look inside the *Kraken logs* if it as received the message properly.<br>
Logs looks like that:

```
[20-05-19 15:59:24,868] maintenance_worker.cpp:179 [DEBUG] - realtime info received from shortterm.transilien
[20-05-19 16:00:02,696] maintenance_worker.cpp:193 [INFO ] - data copied in 00:00:37.827953
[20-05-19 16:00:02,696] maintenance_worker.cpp:199 [DEBUG] - add/update of disruption 6213dc54-b0aa-4cf9-a8f6-cc8f770c9e4c
[20-05-19 16:00:02,696] apply_disruption.cpp:755 [DEBUG] - Deleting disruption: 6213dc54-b0aa-4cf9-a8f6-cc8f770c9e4c
[20-05-19 16:00:02,696] apply_disruption.cpp:766 [DEBUG] - disruption 6213dc54-b0aa-4cf9-a8f6-cc8f770c9e4c deleted
[20-05-19 16:00:02,696] make_disruption_from_chaos.cpp:344 [DEBUG] - Adding disruption: 6213dc54-b0aa-4cf9-a8f6-cc8f770c9e4c
[20-05-19 16:00:02,696] make_disruption_from_chaos.cpp:373 [DEBUG] - 6213dc54-b0aa-4cf9-a8f6-cc8f770c9e4c disruption added
[20-05-19 16:00:02,696] apply_disruption.cpp:772 [DEBUG] - applying disruption: 6213dc54-b0aa-4cf9-a8f6-cc8f770c9e4c
[20-05-19 16:00:02,717] maintenance_worker.cpp:213 [INFO ] - rebuilding relations
[20-05-19 16:00:03,255] maintenance_worker.cpp:219 [INFO ] - cleaning weak impacts
[20-05-19 16:00:03,266] maintenance_worker.cpp:221 [INFO ] - rebuilding data raptor
[20-05-19 16:00:03,266] data.cpp:208 [DEBUG] - Start to build data Raptor
[20-05-19 16:00:16,600] data.cpp:210 [DEBUG] - Finished to build data Raptor
[20-05-19 16:00:16,600] proximity_list.cpp:75 [INFO ] - Building Proximitylist's NN index with 20007 items
[20-05-19 16:00:16,606] proximity_list.cpp:75 [INFO ] - Building Proximitylist's NN index with 41923 items
[20-05-19 16:00:16,616] georef.cpp:397 [INFO ] - Building Proximity list for walking graph
[20-05-19 16:00:16,647] proximity_list.cpp:75 [INFO ] - Building Proximitylist's NN index with 875079 items
[20-05-19 16:00:16,897] georef.cpp:400 [INFO ] - Building Proximity list for bike graph
[20-05-19 16:00:16,931] proximity_list.cpp:75 [INFO ] - Building Proximitylist's NN index with 781857 items
[20-05-19 16:00:17,146] georef.cpp:403 [INFO ] - Building Proximity list for car graph
[20-05-19 16:00:17,178] proximity_list.cpp:75 [INFO ] - Building Proximitylist's NN index with 739281 items
[20-05-19 16:00:17,381] georef.cpp:406 [INFO ] - Building Proximity list for POIs
[20-05-19 16:00:17,382] proximity_list.cpp:75 [INFO ] - Building Proximitylist's NN index with 45080 items
[20-05-19 16:00:22,736] georef.cpp:612 [DEBUG] - Number of stop point projected on the georef network : 41866 (on 41923)
[20-05-19 16:00:22,736] georef.cpp:615 [DEBUG] - Number of stop point projected on the walking georef network : 41866 (on 41923)
[20-05-19 16:00:22,736] georef.cpp:617 [DEBUG] - Number of stop point projected on the biking georef network : 41866 (on 41923)
[20-05-19 16:00:22,736] georef.cpp:619 [DEBUG] - Number of stop point projected on the car georef network : 41866 (on 41923)
[20-05-19 16:00:22,736] georef.cpp:622 [DEBUG] - Number of stop point rejected (X=0 or Y=0)1
[20-05-19 16:00:22,736] georef.cpp:628 [DEBUG] - Number of stop point rejected (other issues)56
[20-05-19 16:00:22,757] next_stop_time.cpp:493 [INFO ] - Cache miss : 0 / 0
[20-05-19 16:00:24,955] maintenance_worker.cpp:229 [INFO ] - data updated 1 disruption applied in 00:01:00.087307
```

### IDs have to be the same

When you load a disruption with Disruptor, like **-p Line('line:DUA:800853022')**, you need to be sure of your ID.<br>
Kraken loads a data.nav.lz4' that have to contain the same ID. Otherwise, the disruption will not be loaded.<br>
You can use **navitia API** like lines/stop_points/networks/... to find if the concerned ID exists.
