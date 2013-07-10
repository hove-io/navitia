"""
Class with the main loop
"""
#coding=utf-8
import glob
from pyed.binarisation import osm2ed, gtfs2ed, ed2nav
from datetime import datetime
from pyed.launch_exec import launch_exec
from pyed.config import ConfigException
import logging

class Watching():
    """ when launch with run(), it looks in the source directory if there are
        any new file (osm or gtfs) to compute.
        If there was any file it will launch ed2nav after 
    """

    def __init__(self, conf):
        """ Init watching according to conf """
        self.directory = conf.get("instance", "source_directory")
        self.conf = conf
        self.pyed_logger = logging.getLogger('pyed')
        self.backup_directory = None

    def make_backupdirectory(self):
        """ If there is no backup directory it creates one in 
            {instance_directory}/backup/{name}
            The name of the backup directory is the time when it's created
            formatted as %Y%m%d-%H%M%S 
        """
        if not self.backup_directory:
            now = datetime.now()
            self.backup_directory = self.conf.get("instance", "directory")
            self.backup_directory += "/backup/" + now.strftime("%Y%m%d-%H%M%S")
            return launch_exec("mkdir", [self.backup_directory],
                               self.pyed_logger)

    def run(self):
        """ Every minutes it looks in the source directory if there is any file
            After it has computed the files it will launch ed2nav
        """
        while True:
            osm_files = glob.glob(self.directory+"/source/*.pbf")
            gtfs_files = glob.glob(self.directory+"/source/*.zip")
            worked_on_files = []
            while len(osm_files) > 0 or len(gtfs_files) :
                try:
                    if self.make_backupdirectory() !=0:
                        self.pyed_logger.error("""Unable to make backup
                                                  directory""")
                except ConfigException:
                    self.pyed_logger.error("""Unable to make backup directory,
                                              conf error""")
                if len(osm_files) > 0:
                    osmfile = osm_files.pop()
                    self.pyed_logger.info("Osm file found " + osmfile )
                    res = osm2ed(osmfile, self.conf, self.backup_directory)
                    if res == 0:
                        self.pyed_logger.info("Osm file add to ed " + osmfile )
                        worked_on_files.append(osmfile)
                    else:
                        self.pyed_logger.error("""Osm file's binarisation
                                                  failed %s"""% osmfile)
                elif len(gtfs_files) > 0:
                    gtfsfile = gtfs_files.pop()
                    self.pyed_logger.info("Gtfs file found " + gtfsfile )
                    res = gtfs2ed(gtfsfile, self.conf, self.backup_directory)
                    if res == 0:
                        self.pyed_logger.info("""Gtfs file added 
                                                 to ed %s""" % gtfsfile )
                        worked_on_files.append(gtfsfile)
                    else:
                        self.pyed_logger.error("""Gtfs file's binarisation
                                                  failed %s""" % gtfsfile)
                else:
                    self.pyed_logger.debug("No gtfs nor osm file found")
                osm_files = glob.glob(self.directory+"/*.pbf")
                gtfs_files = glob.glob(self.directory+"/*.zip")

            if len(worked_on_files) > 0:
                self.pyed_logger.info("Launching ed2nav")
                res = ed2nav(self.directory+"/data/data.nav.lz4", self.conf)
                joined_files_str = ", ".join(worked_on_files)
                if res == 0:
                    self.pyed_logger.info("""Ed2nav has finished for the files
                                            : %s"""%joined_files_str)
                else:
                    self.pyed_logger.error("""Ed2nav failed for the files 
                                             : %s"""%joined_files_str)
                worked_on_files = []
            else:
                self.pyed_logger.debug("We haven't made any binarisation")
            self.backup_directory = None
