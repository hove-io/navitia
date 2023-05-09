#! python
"""Bench Jormungandr

Usage:
  bench.py (-h | --help)
  bench.py --version
  bench.py bench [-i FILE | --input=FILE] [-a ARGS | --extra-args=ARGS] [-c CONCURRENT | --concurrent=CONCURRENT] [--scenario=SCENARIO]...
  bench.py replot [<file> <file>]
  bench.py plot-latest N
  bench.py sample <min_lon> <max_lon> <min_lat> <max_lat> <size> [-a ARGS | --extra-args=ARGS] [-s SEED | --seed=SEED]

Options:
  -h --help                                 Show this screen.
  --version                                 Show version.
  -i FILE, --input=FILE                     Input csv (or stdin if missing)
  -a ARGS, --extra-args=ARGS                Extra args for the request
  -c CONCURRENT, --concurrent=CONCURRENT    Concurrent request number
  --scenario=SCENARIO                       Scenarios [default: new_default  distributed]
  -s SEED, --seed=SEED                      Random seed [default: 42]

Example:
  bench.py bench --input=benchmark_example.csv -a 'first_section_mode[]=car&last_section_mode[]=car'
  cat benchmark_example.csv | bench.py bench  -a 'first_section_mode[]=car&last_section_mode[]=car' --scenario=new_default
  bench.py replot new_default.csv distributed.csv
  bench.py plot-latest 30
  bench.py sample 2.298255 2.411574 48.821590 48.898 1000 -a '&datetime=20200318T100000'
"""
from __future__ import print_function
import requests
import datetime
import numpy
import tqdm
import pygal
import os
import csv
from config import logger
import config
import sortedcontainers
from glob import glob
from concurrent.futures import ThreadPoolExecutor, as_completed
import fileinput
import random

NAVITIA_API_URL = os.getenv('NAVITIA_API_URL', config.NAVITIA_API_URL)
COVERAGE = os.getenv('COVERAGE', config.COVERAGE)
TOKEN = os.getenv('TOKEN', config.TOKEN)
DISTANT_BENCH_OUTPUT = os.getenv('DISTANT_BENCH_OUTPUT', config.DISTANT_BENCH_OUTPUT)
OUTPUT_DIR = os.getenv('OUTPUT_DIR', config.OUTPUT_DIR)


def get_coverage_start_production_date():
    r = requests.get("{}/coverage/{}".format(NAVITIA_API_URL, COVERAGE), headers={'Authorization': TOKEN})
    j = r.json()
    return datetime.datetime.strptime(j['regions'][0]['start_production_date'], '%Y%m%d')


def get_request_datetime(start_prod_date, days_shift, seconds):
    return start_prod_date + datetime.timedelta(days=days_shift, seconds=seconds)


def _call_jormun(url):
    import time

    time1 = time.time()
    r = requests.get(url, headers={'Authorization': TOKEN}, verify=True)
    time2 = time.time()
    elapsed_time = (time2 - time1) * 1000.0
    return r, elapsed_time


def plot_per_request(data):
    line_chart = pygal.Line(show_x_labels=False)
    line_chart.title = 'Jormun Bench (in ms)'
    for label, array in data:
        line_chart.add(label, array)
        line_chart.x_labels = map(str, range(0, len(array)))
    line_chart.render_to_file('output_per_request.svg')


def plot_normalized_box(data):
    box = pygal.Box()
    box.title = 'Jormun Bench Box'

    for label, array in data:
        box.add(label, array)

    box.render_to_file('output_box.svg')


def call_jormun(server, coverage, path, parameters, scenario_name, extra_args):
    import os.path

    # remove excessive /
    normpath = os.path.normpath('/'.join(['coverage', coverage, path]))

    url = "{}/{}?{}&_override_scenario={}&{}".format(server, normpath, parameters, scenario_name, extra_args)
    ret, t = _call_jormun(url)
    return url, t, ret.status_code


def call_jormun_scenario(i, server, coverage, path, parameters, scenario, extra_args):
    return i, call_jormun(server, coverage, path, parameters, scenario, extra_args)


class Scenario(object):
    def __init__(self, name, output_dir):
        self.times = sortedcontainers.SortedDict()
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        self.output = os.path.join(output_dir, "{}.csv".format(name))
        with open(self.output, 'w') as f:
            print("No,url,elapsed time,status_code", file=f)


def submit_work(pool, reqs, extra_args, scenario):
    for i, (path, params) in enumerate(reqs):
        args = [i, NAVITIA_API_URL, COVERAGE, path, params, extra_args, scenario]
        yield pool.submit(call_jormun_scenario, *args)


def make_requests(input_file_name):
    with fileinput.input(input_file_name or '-') as f:
        return [(req['path'], req['parameters']) for req in csv.DictReader(f)]


def bench(args):
    extra_args = args['--extra-args'] or ''
    concurrent = int(args['--concurrent'] or 1)

    # if no input file is given, it will read from stdin
    reqs = make_requests(args['--input'])

    scenarios = {s: Scenario(s, OUTPUT_DIR) for s in args['--scenario']}

    for name, scenario in scenarios.items():
        logger.info('Launching bench for {}'.format(name))
        with ThreadPoolExecutor(concurrent) as pool, open(scenario.output, 'a') as output:
            futures = (f for f in submit_work(pool, reqs, name, extra_args))
            for i in tqdm.tqdm(as_completed(futures), total=len(reqs)):
                idx, res = i.result()
                url, time, status_code = res
                if status_code >= 300:
                    # Numpy and pygal will regard this record as a invalid one
                    time = 0
                print("{},{},{},{}".format(idx, url, time, status_code), file=output)
                scenario.times[idx] = time

    plot_per_request([(name, s.times.values()) for name, s in scenarios.items()])
    plot_normalized_box([(name, s.times.values()) for name, s in scenarios.items()])


def get_times(csv_path):
    times = []
    try:
        import csv

        with open(csv_path, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                times.append(float(row['elapsed time']))
        logger.info('Finish parsing: {}'.format(csv_path))
    except Exception as e:
        import sys

        logger.error('Error occurred when parsing csv: ' + str(e))
    return times


def replot(args):
    files = args.get('<file>') or ['new_default.csv', 'distributed.csv']
    file1 = files[0]
    file2 = files[1]
    time_arry1 = get_times(file1)
    time_arry2 = get_times(file2)
    plot_per_request([(file1, time_arry1), (file2, time_arry2)])
    plot_normalized_box([(file1, time_arry1), (file2, time_arry2)])


def get_benched_coverage_from_output():
    return glob(DISTANT_BENCH_OUTPUT + '/output/*/')


def get_latest_bench_output(coverage, n):
    bench_outputs = glob(coverage + '/*/')
    bench_outputs.sort(reverse=True)
    return bench_outputs[:n]


def plot_latest(args):
    n = int(args.get('N'))
    coverages = get_benched_coverage_from_output()
    if not os.path.exists(os.path.join(DISTANT_BENCH_OUTPUT, 'rendering')):
        os.makedirs(os.path.join(DISTANT_BENCH_OUTPUT, 'rendering'))

    for cov in coverages:
        latest_bench_outputs = get_latest_bench_output(cov, n)
        box = pygal.Box(box_mode="tukey")
        coverage_name = cov.split('/')[-2]
        box.title = coverage_name
        for output in latest_bench_outputs[::-1]:
            time_array1 = get_times(os.path.join(output, "{}.csv".format('distributed')))
            time_array2 = get_times(os.path.join(output, "{}.csv".format('new_default')))
            if len(time_array1) == len(time_array2):
                box.add(output.split('/')[-2], numpy.array(time_array1) / numpy.array(time_array2))
        box.render_to_file(os.path.join(DISTANT_BENCH_OUTPUT, 'rendering', '{}.svg'.format(coverage_name)))


def sample(args):
    min_lon = float(args['<min_lon>'])
    max_lon = float(args['<max_lon>'])
    min_lat = float(args['<min_lat>'])
    max_lat = float(args['<max_lat>'])
    n = int(args['<size>'])

    extra_args = args['--extra-args'] or ''

    header = ['path', 'parameters']
    print(','.join(header))

    path = '/journeys'

    for _ in range(n):
        from_coord = "{};{}".format(random.uniform(min_lon, max_lon), random.uniform(min_lat, max_lat))
        to_coord = "{};{}".format(random.uniform(min_lon, max_lon), random.uniform(min_lat, max_lat))

        params = 'from={}&to={}&{}'.format(from_coord, to_coord, extra_args)
        print(','.join([path, params]))


def parse_args():
    from docopt import docopt

    return docopt(__doc__, version='Jormungandr Bench V0.0.1')


def main():
    args = parse_args()
    logger.info('Running Benchmark on {}/{}'.format(NAVITIA_API_URL, COVERAGE))
    if args.get('bench'):
        bench(args)
    if args.get('replot'):
        replot(args)
    if args.get('plot-latest'):
        plot_latest(args)
    if args.get('sample'):
        sample(args)


if __name__ == '__main__':
    main()
