# -*- coding: utf-8 -*-
from git import *
from datetime import datetime
import subprocess
import re
from sys import exit, argv
from shutil import copyfile
import os
from os import remove, stat
import codecs
import requests


def get_tag_name(version):
        return "v{maj}.{min}.{hf}".format(maj=version[0], min=version[1], hf=version[2])


class ReleaseManager:

    def __init__(self, release_type, remote_name="canalTP"):
        self.directory = "."
        self.release_type = release_type
        self.repo = Repo(self.directory)
        self.git = self.repo.git

        #we fetch latest version from remote
        self.remote_name = remote_name

        print "fetching from {}".format(remote_name)
        self.repo.remote(remote_name).fetch("--tags")

        #and we update dev and release branches
        print "rebase dev and release"

        #TODO quit on error
        self.git.rebase(remote_name + "/dev", "dev")
        self.git.rebase(remote_name + "/release", "release")

        print "current branch {}".format(self.repo.active_branch)

        self.version = None
        self.str_version = ""
        self.latest_tag = ""

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

        self.latest_tag = get_tag_name(self.version)
        print "last tag is " + self.latest_tag

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

        self.str_version = "{maj}.{min}.{hf}".format(maj=self.version[0],
                                                     min=self.version[1],
                                                     hf=self.version[2])

        print "current version is {}".format(self.str_version)
        return self.str_version

    def checkout_parent_branch(self):
        parent=""
        if self.release_type == "major" or self.release_type == "minor":
            parent = "dev"
        else:
            parent = "release"

        self.git.checkout(parent)

        print "current branch {}".format(self.repo.active_branch)

    def get_merged_pullrequest(self):

        #we get all closed PR since latest tag
        #WARNING: it is not the list of merged PR since github does not have that
        query = "https://api.github.com/repos/CanalTP/navitia/" \
                "pulls?state=closed&head={latest_tag}".format(latest_tag=self.latest_tag)

        print "query github api: " + query

        github_response = requests.get(query)

        if github_response.status_code != 200:
            message = github_response.json()['message']
            return u'  * Impossible de récupérer les PR\n  * '+ message

        closed_pr = github_response.json()

        lines = []
        for pr in closed_pr:
            title = pr['title']
            url = pr['html_url']
            lines.append(u'  * {title}  <{url}>\n'.format(title=title, url=url))

        return lines

    def create_changelog(self):
        write_lines = [
            u'navitia2 (%s) unstable; urgency=low\n' % self.str_version,
            u'\n',
        ]

        if self.release_type != "hotfix":
            pullrequests = self.get_merged_pullrequest()
            write_lines.extend(pullrequests)
        else:
            write_lines.append(u'  * \n')

        author_name = self.git.config('user.name')
        author_mail = self.git.config('user.email')
        write_lines.extend([
            u'\n',
            u' -- {name} <{mail}>  {now} +0100\n'.format(name=author_name, mail=author_mail,
                                                         now=datetime.now().strftime("%a, %d %b %Y %H:%m:%S")),
            u'\n',
        ])

        return write_lines

    def update_changelog(self):
        print "updating changelog"
        changelog = self.create_changelog()

        f_changelog = None
        changelog_filename = "debian/changelog"
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

    def get_modified_changelog(self):
        #the changelog might have been modified by the user, so we have to read it again
        changelog_filename = "debian/changelog"

        f_changelog = codecs.open(changelog_filename, 'r', 'utf-8')

        lines = []
        nb_version = 0
        for line in f_changelog:
            #each version are separated by a line like
            #navitia2 (0.94.1) unstable; urgency=low
            if line.startswith("navitia2 "):
                nb_version += 1
                continue
            if nb_version >= 2:
                break  # we can stop
            if nb_version == 0:
                continue

            lines.append(line + u'\n')

        f_changelog.close()
        return lines

    def publish_release(self, temp_branch):
        self.git.checkout("release")
        #merge with the release branch
        self.git.merge(temp_branch, "release", '--no-ff')

        print "current branch {}".format(self.repo.active_branch)
        #we tag the release
        tag_message = u'Version {}\n'.format(self.str_version)

        changelog = self.get_modified_changelog()
        for change in changelog:
            tag_message += change

        print "tag: " + tag_message

        self.repo.create_tag(get_tag_name(self.version), message=tag_message)

        #and we merge back the temporary branch to dev
        self.git.merge(temp_branch, "dev", '--no-ff')

        print "publishing the release"

        print "check the release and when you're happy do:"
        print "git push {} release dev --tags".format(self.remote_name)
        #TODO: when we'll be confident, we will do that automaticaly

    def release_the_kraken(self):
        new_version = self.get_new_version_number()

        tmp_name = "release_%s" % new_version

        self.checkout_parent_branch()

        #we then create a new temporary branch
        print "creating temporary release branch {}".format(tmp_name)
        self.git.checkout(b=tmp_name)
        print "current branch {}".format(self.repo.active_branch)

        self.update_changelog()

        self.git.commit(m="Version %s" % self.str_version)

        if self.release_type == "hotfix":
            print "now time to do your actual hotfix!"
            print "Note: you'll have to merge/tag/push manually after your fix:"
            print "git checkout release"
            print "git merge --no-ff {tmp_branch}".format(tmp_branch=tmp_name)
            print "git tag -a {}".format(get_tag_name(self.version))

            print "git checkout dev"
            print "git merge --no-ff release".format(tmp_branch=tmp_name)
            print "git push {} release dev --tags".format(self.remote_name)

            #TODO2 try to script that (put 2 hotfix param, like hotfix init and hotfix publish ?)
            exit(0)

        self.publish_release(tmp_name)


if __name__ == '__main__':

    if len(argv) < 2:
        print "mandatory argument: {major|minor|hotfix}"
        print "possible additional argument: remote (default is canalTP)"
        exit(5)

    #the git lib used bug when not in english
    os.environ['LC_ALL'] = 'en_US'

    r_type = argv[1]
    remote = argv[2] if len(argv) >= 3 else "CanalTP"

    manager = ReleaseManager(release_type=r_type, remote_name=remote)
    manager.release_the_kraken()
