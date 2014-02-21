import os
import hashlib

saving_directory = "fixtures"

def make_filename(request):
    """Hash the request to make the name"""
    filename = hashlib.md5(str(request).encode()).hexdigest()
    return os.path.join(saving_directory, filename)
