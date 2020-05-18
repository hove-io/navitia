# Log Analyzer

The log analyzer produces a Gantt chart of the different computations performed during a "journey" call to Navitia.
It does so by parsing the logs of Jormungandr and Kraken.


# How to use

- Configure Jormungandr and Kraken logs to record the `info` level (on by default)
The logs are on by default for Jormungandr. For Kraken, you can add these lines to the `kraken.ini` configuration file :

```
[LOG]
log4cplus.rootLogger = INFO, ALL_MSGS
log4cplus.appender.ALL_MSGS=log4cplus::FileAppender
log4cplus.appender.ALL_MSGS.File=kraken.log
log4cplus.appender.ALL_MSGS.layout=log4cplus::PatternLayout
log4cplus.appender.ALL_MSGS.layout.ConversionPattern=[%D{%y-%m-%d %H:%M:%S,%q}] %b:%L [%-5p] - %m %n

```

- prepare the needed python environment with `virtualenvwrapper` (see https://virtualenvwrapper.readthedocs.io/en/latest/ for installation instructions).
  You can also install the python librairies listed in `requirements.txt` in your own environment

```
cd log_analyzer/
mkvirtualenv -p python3 -r requirements.txt log_analyzer
```

- then load the python environment, and launch the tool

```
workon log_analyzer
python3 log_analyzer.py --kraken_log_file path/to/kraken.log --jormun_log_file path/to/jormun.log --output_dir my/output/dir/

```

This will create in `my/output/dir/` an html file containing the gantt chart for each journey request found in the logs of Jormungandr and Kraken.


## How does it work

For each request to Navitia, a `request_id` is generated in `jormungandr/interfaces/v1/Journeys.py:get:generate_request_id`.
This identifier is kept around all computations :
 - it is used in Jormungandr each time the `timed_logger` of `jormungandr/scenarios/helper_classes/helper_utils.py`
 - it is sent to Kraken via the `request_id` field of the protobuf, and used in `kraken/kraken_zmq.h:do_work`

The `log_analyzer` identify the log lines corresponding to computations timing, parses them, group them by request, and then produces the chart.
