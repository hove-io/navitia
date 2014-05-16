# -*- coding: utf-8 -*-
from git import *
from datetime import datetime
import subprocess
import re
from sys import exit, argv
from shutil import copyfile
from os import remove, stat
import codecs


class ReleaseManager:

    def __init__(self, release_type, remote_name="canalTP"):
        self.directory = "~/bak/kraken"
        self.release_type = release_type
        self.repo = Repo(self.directory)
        self.git = repo.git

        #we fetch latest version from remote
        self.remote_name = remote_name

        print "fetching from {}".format(remote_name)
        self.git.fetch(remote_name, "--tags")

        #and we update dev and release branches
        print "rebase dev and release"

        #TODO quit on error
        self.git.rebase("canalTP/dev", "dev")
        self.git.rebase("canalTP/release", "release")

        self.version = None

    def get_new_version_number(self):
        latest_version = None
        last_tag = self.git.describe('--tags', abbrev=0)

        version = re.search('.*(\d+\.\d+\.\d+).*', last_tag)
        if version:
            latest_version = version.group(1)

        if not latest_version:
            print "no latest version found"
            exit(1)

        version_n = latest_version.split('.')
        print "latest version is {}".format(version_n)

        self.version = [int(i) for i in version_n]

        if self.release_type == "major":
            self.version[0] += 1
            self.version[1] = version_n[2] = 0
        elif self.release_type == "minor":
            self.version[1] += 1
            self.version[2] = 0
        elif self.release_type == "hotfix":
            self.version[2] += 1
        else:
            exit(5)

        str_version = "{maj}.{min}.{hf}".format(maj=version_n[0], min=version_n[1], hf=version_n[2])

        return str_version

    def get_parent_branch(self):
        parent=""
        if self.release_type == "major" or self.release_type == "minor":
            parent = "dev"
        else:
            parent = "release"

        parent_head = self.git.checkout(b=parent)

        return parent_head

    def get_merged_pullrequest(self):
        pass

    def create_changelog(self):
        write_lines = [
            u'navitia2 (%s) unstable; urgency=low\n' % version,
            u'\n',
        ]

        if self.release_type != "hotfix":
            pullrequests = get_merged_pullrequest()
            write_lines.append(pullrequests)
        else:
            write_lines.append(u' * \n')

        author_name = self.git.config('user.name')
        author_mail = self.git.config('user.mail')
        write_lines.extend([
            u'\n',
            u' -- {name} <{mail}>  {now} +0100\n'.format(name=author_name, mail=author_mail,
                                                         now=datetime.now().strftime("%a, %d %b %Y %H:%m:%S")),
            u'\n',
        ])

    def update_changelog(self):
        changelog = create_changelog()

        f_changelog = None
        changelog_filename = self.directory + "/" + "debian/changelog"
        back_filename = changelog_filename + "~"
        try:
            f_changelog = codecs.open(changelog_filename, 'r', 'utf-8')
        except IOError:
            print "Unable to open debian/changelog"
            exit(1)
        f_changelogback = codecs.open(back_filename, "w", "utf-8")

        for line in changelog:
            f_changelogback.write(line)

        for line in f_changelog:
            f_changelogback.write(line)
        f_changelog.close()
        f_changelogback.close()
        last_modified = stat(back_filename)
        (stdout, stderr) = subprocess.Popen(["vim", back_filename, "--nofork"],
                                            stderr=subprocess.PIPE).communicate()
        after = stat(back_filename)
        if last_modified == after:
            print "No changes made, we stop"
            remove(back_filename)
            exit(2)

        copyfile(back_filename, changelog_filename)

        self.git.add(changelog_filename)

    def update_version(self):

        cmake = open(self.directory + "/" + "source/CMakeLists.txt", "r")
        cmake_lines = []
        for line in cmake:
            line = re.sub("^SET\(KRAKEN_VERSION_MAJOR (\d+)\)$",
                          lambda matchobj: "SET(KRAKEN_VERSION_MAJOR %i)" % self.version[0], line)
            line = re.sub("^SET\(KRAKEN_VERSION_MINOR (\d+)\)$",
                          lambda matchobj: "SET(KRAKEN_VERSION_MINOR %i)" % self.version[1], line)
            line = re.sub("^SET\(KRAKEN_VERSION_PATCH (\d+)\)$",
                          lambda matchobj: "SET(KRAKEN_VERSION_PATCH %i)" % self.version[2], line)
            cmake_lines.append(line)
        cmake.close()

        cmake = open(self.directory + "/" + "source/CMakeLists.txt", "w")
        cmake.writelines(cmake_lines)
        cmake.close()
        self.git.add(self.directory + "/" + "source/CMakeLists.txt")

    def publish_release(self):
        print "publishing the release"
        pass

    def release_the_kraken(self):

        new_version = self.get_new_version_number(release_type)

        tmp_name = "release_%s" % new_version

        parent_head = self.get_parent_branch(release_type)

        self.git.checkout(b=tmp_name)

        self.update_changelog()
        self.update_version()

        git.commit(m="Version %s"%version)

        if self.release_type == "hotfix":
            print "now time to do your actual hotfix!"
            print "Note: you'll have to merge/taf/push manually after your fix:"
            print "git checkout release"
            print "git merge --no-ff {tmp_branch}".format(tmp_branch=tmp_name)
            print "git tag TODO!" #TODO! explicit command

            print "git merge --no-ff {tmp_branch} dev".format(tmp_branch=tmp_name)
            print "git push {} release dev --tags".format(self.remote_name)

            #TODO test the commands above :)
            #TODO2 try to script that (put 2 hotfix param, like hotfix init and hotfix publish ?)
            exit(0)

        self.publish_release()


if __name__ == '__main__':

    if len(argv) < 2:
        print "mandatory argument: {major|minor|hotfix} remote"
        exit(5)

    r_type = argv[1]
    remote = argv[2] if len(argv) >= 2 else "origin" #"canalTP"

    manager = ReleaseManager(release_type=r_type, remote_name=remote)
    manager.release_the_kraken()
