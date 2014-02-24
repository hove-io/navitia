import os
import hashlib

saving_directory = os.path.dirname(os.path.realpath(__file__)) + "/fixtures"

os.environ['JORMUNGANDR_CONFIG_FILE'] = '../tests/integration_tests_settings.py'

def make_filename(request):
    """Hash the request to make the name"""
    filename = hashlib.md5(str(request).encode()).hexdigest()
    return os.path.join(saving_directory, filename)
