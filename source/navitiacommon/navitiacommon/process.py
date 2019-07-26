# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

import logging
import re
import subprocess
import sys

"""
Function to launch a bin
"""

logger = logging.getLogger(__name__)  # type: logging.Logger


def run(exec_name, args, sync=True, new_logger=None):
    """
    Run a process with its args.

    :param exec_name: the binary name to run the new subprocess
    :param args: the parameters for the subprocess
    :param sync: true if synchronous, otherwise false
    :param new_logger: the optionnal logger to log into
    :return: the error code of the subprocess
    """
    # we hide the password in logs
    args_str = re.sub('password=\w+', 'password=xxxxxxxxx', ' '.join(args))
    logger.info('Running binary: ' + exec_name + ' ' + args_str)

    args.insert(0, exec_name)
    proc = subprocess.Popen(args, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    for line in proc.stdout:
        if new_logger is not None:
            new_logger.info(line)
        else:
            sys.stdout.write(line)

    if sync is True:
        proc.wait()

    return proc.returncode
