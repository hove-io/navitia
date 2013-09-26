import sys
sys.path.append("/srv/jormungandr")
import jormungandr
import os
os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'
os.environ['JORMUNGANDR_CONFIG_FILE'] = '/srv/jormungandr/Jormungandr.ini'
os.chdir('/srv/jormundandr')
application = jormungandr.application
