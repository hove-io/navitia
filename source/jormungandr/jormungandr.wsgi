import sys
sys.path.append("/srv/jormungandr")
import jormungandr
import os
os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'
application = jormungandr.application
