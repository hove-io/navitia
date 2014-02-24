"""
Function to launch a bin
"""
import subprocess
import os
import select
import re


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
        while True:
            if poller.poll(1000):
                line = os.read(fdr, 1000)
                logger.info(line)
            if proc.poll() is not None:
                break

    finally:
        os.close(fdr)
        os.close(fdw)


    return proc.returncode
