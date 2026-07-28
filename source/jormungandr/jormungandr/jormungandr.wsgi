import sys
import os

sys.path.append("/srv/jormungandr")
os.environ['JORMUNGANDR_CONFIG_FILE'] = '/srv/jormungandr/Jormungandr.ini'
from jormungandr import app as application
