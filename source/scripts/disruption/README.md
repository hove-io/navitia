# Disruptor

Create and send **disruption** inside navitia core. It can be usefull to debug Kraken.<br>
RabbitMq is used as a broker to exchange disruption messages.

## Set up

Disruptor depends on **chaos-proto** sources for disrurption messages.

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

# Route - impact REDUCED SERVICE
PYTHONPATH="navitia_source_dir/source/chaos-proto" python2.7 disruptor.py -b pyamqp://guest:guest@localhost:5672 -e "navitia" -t "shortterm.transilien" -p "Route('route_uri')" -i "REDUCED_SERVICE"
```

Disruptions **with file**

Example of file (`impact.json <https://github.com/canaltp/navitia/blob/dev/source/scripts/disruption/impacts.json>`).<br>
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
