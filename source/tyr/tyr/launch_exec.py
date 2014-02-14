"""
Function to launch a bin
"""
import subprocess
import os
import select


def launch_exec(exec_name, args, logger):
    """ Launch an exec with args, log the outputs """
    logger.info("Launching " + exec_name + " " + " ".join(args))
    args.insert(0, exec_name)
    fdr, fdw = os.pipe()
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

    return proc.returncode
