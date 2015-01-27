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

"""
Function to launch a bin
"""
import subprocess
import os
import select
import re

class LogLine(object):
    def __init__(self, line):
        if line.startswith('DEBUG') or line.startswith('TRACE') or line.startswith('NOTICE') or not line:
            self.level = 10
        elif line.startswith('INFO'):
            self.level = 20
        elif line.startswith('WARN'):
            self.level = 30
        else:
            self.level = 40

        pos = line.find(' - ')
        if 0 < pos < 10:
            self.msg = line[pos+3:]
        else:
            self.msg = line

def parse_log(buff):
    logs = []
    line, sep, buff = buff.partition('\n')
    while sep and buff:
        logs.append(LogLine(line))
        line, sep, buff = buff.partition('\n')
    if not sep:
        buff = line#we put back the last unterminated line in the buffer
    return (logs, buff)

def launch_exec(exec_name, args, logger):
    """ Launch an exec with args, log the outputs """
    log = 'Launching ' + exec_name + ' ' + ' '.join(args)
    #we hide the password in logs
    logger.info(re.sub('password=\w+', 'password=xxxxxxxxx', log))

    args.insert(0, exec_name)
    fdr, fdw = os.pipe()
    try:
        proc = subprocess.Popen(args, stderr=fdw,
                         stdout=fdw, close_fds=True)
        poller = select.poll()
        poller.register(fdr)
        line = ''
        while True:
            if poller.poll(1000):
                line += os.read(fdr, 1000)
                logs, line = parse_log(line)
                for l in logs:
                    logger.log(l.level, l.msg)
            if proc.poll() is not None:
                break

    finally:
        os.close(fdr)
        os.close(fdw)


    return proc.returncode
