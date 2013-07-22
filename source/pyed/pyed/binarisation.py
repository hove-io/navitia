"""
Functions to launch the binaratisations
"""
from pyed.launch_exec import launch_exec
from pyed.config import ConfigException
import logging
import subprocess

def make_connection_string(config):
    """ Make the a connection string connection from the config """
    connection_string = 'host='+config.get("database", "host")
    connection_string += " user="+config.get("database", "user")
    connection_string += " dbname="+config.get("database", "name")
    connection_string += " password="+config.get("database", "password")
    return connection_string

def gtfs2ed(gtfs_filename, config, backup_directory):
    """ Unzip gtfs file, remove the file, launch gtfs2ed """
    pyed_logger = logging.getLogger('pyed')
    gtfs_logger = logging.getLogger('gtfs2ed')
    res = launch_exec("unzip", [gtfs_filename, "-d", backup_directory],
                       pyed_logger)
    if res != 0:
        return 1
    res = launch_exec("rm", [gtfs_filename], pyed_logger)
    if res != 0:
        return 2
    try :
        connection_string = make_connection_string(config)
    except ConfigException:
        pyed_logger.error("gtfs2ed : Unable to make the connection string")
        return 3
    res = launch_exec(config.get("instance", "directory")+"/gtfs2ed",
                ["-i", backup_directory, "--connection-string",
                 connection_string],
                gtfs_logger, pyed_logger)
    if res != 0:
        return 4
    return 0


def osm2ed(osm_filename, config, backup_directory):
    """ Move osm file to backup directory, launch osm2ed """
    pyed_logger = logging.getLogger('pyed')
    osm_logger = logging.getLogger('osm2ed')
    res = launch_exec("mv", [osm_filename, backup_directory], pyed_logger)
    if res != 0:
        return 1
    connection_string = ""
    try:
        connection_string = make_connection_string(config)
    except ConfigException:
        pyed_logger.error("osm2ed : Unable to make the connection string")
        return 2
    mved_file = backup_directory
    mved_file += osm_filename.split("/")[-1]
    res = launch_exec(config.get("instance", "directory")+"/osm2ed",
                ["-i", mved_file, "--connection-string", connection_string],
                osm_logger, pyed_logger)
    if res != 0:
        return 3
    return 0


def ed2nav(filename, config):
    """ Launch osm2ed, compute the md5 sum of it, and save the md5 """
    pyed_logger = logging.getLogger('pyed')
    ed2nav_logger = logging.getLogger('ed2nav')
    target_directory = config.get("instance" , "target_directory")
    filename = target_directory +"/data.nav.lz4"
    try:
        connection_string = make_connection_string(config)
    except ConfigException:
        pyed_logger.error("osm2ed : Unable to make the connection string")
        return 1
    res = launch_exec(config.get("instance", "directory")+"/ed2nav",
                ["-o", filename, "--connection-string", connection_string],
                ed2nav_logger, pyed_logger)
    if res != 0:
        return 2
    md5 = subprocess.Popen(('/usr/bin/md5sum', filename),
                            stdout=subprocess.PIPE)
    cut = subprocess.Popen(['cut', '-d', ' ', '-f1'], stdin=md5.stdout,
                                  stdout=subprocess.PIPE)
    pyed_logger.info("Lauching md5 of "+ filename)
    if md5.wait() == 0:
        pyed_logger.info("md5 of "+filename+ " computed")
        pyed_logger.info("cutting md5 of "+filename)
        cut_log = cut.communicate()
        if cut.wait()==0:
            pyed_logger.info("md5 of "+filename+" cutted")
            pyed_logger.info("writing md5 to "+filename+".md5")
            md5file = open(filename+".md5", 'w')
            md5file.write(cut_log[0])
            md5file.close()
            pyed_logger.info("md5 of "+filename+" written in "+filename+".md5")
        else:
            pyed_logger.error("cutting the md5 of "+ filename+ "failed ")
            if len(cut_log[0]):
                pyed_logger.info(cut_log[0])
            if len(cut_log[1]):
                pyed_logger.error(cut_log[1])
    else:
        pyed_logger.error("md5 of "+ filename+ "failed ")
    return 0
