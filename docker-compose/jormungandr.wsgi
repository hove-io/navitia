#
## File managed by fabric, don't edit directly
#

# this import might fix a strange thread problem: http://bugs.python.org/issue7980
import _strptime

import sys

sys.path.append('/usr/src/app')

from jormungandr import app as application
