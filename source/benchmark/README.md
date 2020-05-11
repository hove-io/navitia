# Navitia Benchmark Tool

Run `pipenv install` to install all required packages.

```
Bench Jormungandr

Usage:
  python bench.py (-h | --help)
  python bench.py --version
  python bench.py bench [-i FILE | --input=FILE] [-a ARGS | --extra-args=ARGS] [-c CONCURRENT | --concurrent=CONCURRENT] [--scenario=SCENARIO]...
  python bench.py replot [<file> <file>]
  python bench.py plot-latest N
  python bench.py sample <min_lon> <max_lon> <min_lat> <max_lat> <size> [-a ARGS | --extra-args=ARGS] [-s SEED | --seed=SEED]

Options:
  -h --help                                 Show this screen.
  --version                                 Show version.
  -i FILE, --input=FILE                     Input csv (or stdin if missing)
  -a ARGS, --extra-args=ARGS                Extra args for the request
  -c CONCURRENT, --concurrent=CONCURRENT    Concurrent request number
  --scenario=SCENARIO                       Scenarios [default: new_default  distributed]
  -s SEED, --seed=SEED                      Random seed [default: 42]

Example:
  python bench.py bench --input=benchmark_example.csv -a 'first_section_mode[]=car&last_section_mode[]=car'
  cat benchmark_example.csv | python bench.py bench  -a 'first_section_mode[]=car&last_section_mode[]=car' --scenario= new_default
  python bench.py replot new_default.csv distributed.csv
  python bench.py plot-latest 30
  python bench.py sample 2.298255 2.411574 48.821590 48.898 1000 -a '&datetime=20200318T100000'
```

To launch the benchmark, you will need to generate a file which contains the requests to navitia. You can find an example in this folder.

## Summary

To use this tool, you should make sure that you have the right configuration in `config.py` or passing the configuration by environnment variables. At the end of benchmark, a `output_box.svg`, a box plot of time consummed by requests,  a `output_per_request.svg`, `distributed.csv` and `new_default.csv` will be generated so that you can check request by request.

## Benchmark /journeys

This benchmark tool is developped to compare `/journeys` performance between `new_default` scenario and `distributed` scenario at the fisrt place. So it will run the request on both scenarios by default.
The `bench` command takes both a file and `stdin` as its input. If you want to generate random journeys requests, you can run the `sample` command as shown in the help message. It will pick randomly two coordinates in the given bounding box then output it on `stdout`. The randomly generated requests can be saved by `python bench.py sample 2.298255 2.411574 48.821590 48.898 1000 -a '&datetime=20200318T100000' > requests.csv` then you can feed it to `bench` by `cat requests.csv | python bench.py bench -c 1`.


## Benchmark other endpoints
You can also use this tool to test other endpoints. In order to test only once, you need to specify on which scenario you want to run the benchmark.
