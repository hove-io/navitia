"""
Function to launch a bin
"""
import subprocess


def launch_exec(exec_name, args, execwriter, pyed_writer=None):
    """ Launch an exec with args, log the outputs """
    execwriter.info("Launching "+ exec_name + " " + " ".join(args))
    if pyed_writer == None:
        pyed_writer = execwriter
    else:
        pyed_writer.info("Launching "+ exec_name + " " + " ".join(args))
    args.insert(0, exec_name)
    proc = subprocess.Popen(args, stderr=subprocess.PIPE,
                     stdout=subprocess.PIPE)
    log = proc.communicate()
    if len(log[0]) > 0:
        execwriter.info(log[0])
    if len(log[1]) > 0:
        execwriter.error(log[1])
    return proc.wait()
