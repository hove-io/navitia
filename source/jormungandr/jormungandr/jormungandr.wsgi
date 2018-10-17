import sys
import os

sys.path.append("/srv/jormungandr")
os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'
os.environ['JORMUNGANDR_CONFIG_FILE'] = '/srv/jormungandr/Jormungandr.ini'
from jormungandr import app as application
