import datetime

NAVITIA_API_URL = 'http://127.0.0.1:5000/v1'
COVERAGE = 'transilien'
TOKEN = ''
DISTANT_BENCH_OUTPUT = 'Navitia-Multimodal-Bench-Output'
OUTPUT_DIR = "{}/output/{}".format(DISTANT_BENCH_OUTPUT, COVERAGE)

# setup logger
import logging

logger = logging.getLogger('bench.py')
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)
