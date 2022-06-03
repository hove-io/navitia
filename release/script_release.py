# -*- coding: utf-8 -*-
# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, division
import os

os.environ['LC_ALL'] = 'en_US'
os.environ['GIT_PYTHON_TRACE'] = '1'  # can be 0 (no trace), 1 (git commands) or full (git commands + git output)

from git import *
from datetime import datetime
import subprocess
import re
from sys import exit, argv
from shutil import copyfile
from os import remove, stat
import codecs
import requests
import logging


def get_tag_name(version):
    return "v{maj}.{min}.{hf}".format(maj=version[0], min=version[1], hf=version[2])


class ReleaseManager:
    def __init__(self, release_type, remote_name="canalTP"):
        self.directory = ".."
        self.changelog_filename = self.directory + "/debian/changelog"
        self.data_version_filename = self.directory + "/source/type/data.cpp"
        self.release_type = release_type
        self.repo = Repo(self.directory)
        self.git = self.repo.git

        # we fetch latest version from remote
        self.remote_name = remote_name

        print("fetching from {}...".format(remote_name))
        self.repo.remote(remote_name).fetch("--tags")

        # and we update dev and release branches
        print("rebasing dev and release...")

        # TODO quit on error
        self.git.rebase(remote_name + "/dev", "dev")
        self.dev_data_version = self.get_data_version()
        remote_release = remote_name + "/release"
        try:
            self.git.checkout("-B release ", remote_release)
        except Exception as e:
            print("Cannot checkout 'release':{}, creating from distant branch".format(str(e)))
            self.git.checkout("-b", "release", remote_release)

        print("checking that release was merged into dev...")
        unmerged = self.git.branch("--no-merged", "dev", '--no-color')
        is_release_unmerged = re.search(" release(\n|$)", unmerged)
        if is_release_unmerged:
            print(is_release_unmerged.group(0))
            print("ABORTING: {rem}/release branch was not merged in {rem}/dev".format(rem=remote_name))
            print("This is required before releasing. You may use (be careful):")
            print("git checkout dev; git submodule update --recursive")
            print("git merge release")

            exit(1)

        print("current branch: {}".format(self.repo.active_branch))

        self.version = None
        self.str_version = ""
        self.latest_tag = ""

        # if API rate limit exceeded use, get 'personal access token' on github then provide:
        # self.auth = ('user', 'pass')
        self.auth = None

    def get_data_version(self):
        f_data_version = codecs.open(self.data_version_filename, 'r', 'utf-8')
        version = None
        for line in f_data_version:
            res = re.search('^ *const .*data_version *= *([0-9]+) *;.*$', line)
            if res:
                version = res.group(1)
                break

        if version is None:
            print("ABORTING: data_version could not be retrieved from {f}".format(f=self.data_version_filename))
            exit(1)

        print("Current data_version is " + version)

        try:
            return int(version)
        except ValueError:
            print("ABORTING: data_version {d} is not an Integer".format(d=version))
            exit(1)

    def get_new_version_number(self):
        last_tag = self.git.describe('--tags', abbrev=0)

        if not last_tag:
            print("no latest version found")
            exit(1)

        version_n = last_tag[1:].split('.')
        print("latest version is {}".format(version_n))

        self.version = [int(i) for i in version_n]

        self.latest_tag = get_tag_name(self.version)
        print("last tag is " + self.latest_tag)

        if self.release_type == "regular":
            if self.version[0] > self.dev_data_version:
                print(
                    "ABORTING: data_version {d} is < to latest tag {t}".format(
                        d=self.dev_data_version, t=self.latest_tag
                    )
                )
                exit(1)
            elif self.version[0] < self.dev_data_version:  # major version
                self.version[0] = self.dev_data_version
                self.version[1] = self.version[2] = 0
            else:  # versions equal: minor version
                self.version[0] = self.dev_data_version
                self.version[1] += 1
                self.version[2] = 0

        elif self.release_type == "major":
            self.version[0] += 1
            self.version[1] = self.version[2] = 0

        elif self.release_type == "minor":
            self.version[1] += 1
            self.version[2] = 0

        elif self.release_type == "hotfix":
            self.version[2] += 1

        else:
            exit(5)

        if self.version[0] > self.dev_data_version:
            print(
                "ABORTING: data_version {d} is < to tag {t} to be published".format(
                    d=self.dev_data_version, t=self.latest_tag
                )
            )
            exit(1)

        self.str_version = "{maj}.{min}.{hf}".format(
            maj=self.version[0], min=self.version[1], hf=self.version[2]
        )

        print("New version is {}".format(self.str_version))
        return self.str_version

    def checkout_parent_branch(self):
        parent = ""
        if self.release_type == "hotfix":
            parent = "release"
        else:
            parent = "dev"

        self.git.checkout(parent)
        self.git.submodule('update', '--recursive')

        print("current branch {}".format(self.repo.active_branch))

    def closed_pr_generator(self):
        # lazy get all closed PR ordered by last updated
        closed_pr = []
        page = 1
        while True:
            query = (
                "https://api.github.com/repos/hove-io/navitia/"
                "pulls?state=closed&base=dev&sort=updated&direction=desc&page={page}".format(page=page)
            )
            print("query github api: " + query)
            github_response = requests.get(query, auth=self.auth)

            if github_response.status_code != 200:
                message = github_response.json()['message']
                print(u'  * Impossible to retrieve PR\n  * ' + message)
                return

            closed_pr = github_response.json()
            if not closed_pr:
                print("Reached end of PR list")
                return

            for pr in closed_pr:
                yield pr

            page += 1

    def get_merged_pullrequest(self):
        lines = []
        nb_successive_merged_pr = 0
        for pr in self.closed_pr_generator():
            title = pr['title']
            url = pr['html_url']
            pr_head_sha = pr['head']['sha']
            # test if PR was merged (not simply closed)
            # and if distant/release contains HEAD of PR
            # (stops after 10 successive merged PR)
            if pr['merged_at']:
                branches = []
                try:
                    branches = self.git.branch('-r', '--contains', pr_head_sha, '--no-color') + '\n'
                except:
                    print(
                        "ERROR while searching for commit in release branch: "
                        "Following PR added to changelog, remove it if needed.\n"
                    )

                # adding separators before and after to match only branch name
                release_branch_name = '  ' + self.remote_name + '/release\n'
                if release_branch_name in branches:
                    nb_successive_merged_pr += 1
                    if nb_successive_merged_pr >= 10:
                        break
                else:
                    # doing the label search as late as possible to save api calls
                    has_excluded_label = False
                    label_query = pr['_links']['issue']['href'] + '/labels'
                    labels = requests.get(label_query, auth=self.auth).json()
                    if any(label['name'] in ("hotfix", "not_in_changelog") for label in labels):
                        has_excluded_label = True

                    if not has_excluded_label:
                        lines.append(u'  * {title}  <{url}>\n'.format(title=title, url=url))
                        print(lines[-1])
                        nb_successive_merged_pr = 0
        return lines

    def create_changelog(self):
        write_lines = [u'navitia2 (%s) unstable; urgency=low\n' % self.str_version, u'\n']

        if self.release_type != "hotfix":
            pullrequests = self.get_merged_pullrequest()
            write_lines.extend(pullrequests)
        else:
            write_lines.append(u'  * \n')

        author_name = self.git.config('user.name')
        author_mail = self.git.config('user.email')
        write_lines.extend(
            [
                u'\n',
                u' -- {name} <{mail}>  {now} +0100\n'.format(
                    name=author_name, mail=author_mail, now=datetime.now().strftime("%a, %d %b %Y %H:%m:%S")
                ),
                u'\n',
            ]
        )

        return write_lines

    def update_changelog(self):
        print("updating changelog")
        changelog = self.create_changelog()

        f_changelog = None
        back_filename = self.changelog_filename + "~"
        try:
            f_changelog = codecs.open(self.changelog_filename, 'r', 'utf-8')
        except IOError:
            print("Unable to open file: " + self.changelog_filename)
            exit(1)
        f_changelogback = codecs.open(back_filename, "w", "utf-8")

        for line in changelog:
            f_changelogback.write(line)

        for line in f_changelog:
            f_changelogback.write(line)
        f_changelog.close()
        f_changelogback.close()
        last_modified = stat(back_filename)
        (stdout, stderr) = subprocess.Popen(
            ["vim", back_filename, "--nofork"], stderr=subprocess.PIPE
        ).communicate()
        after = stat(back_filename)
        if last_modified == after:
            print("No changes made, we stop")
            remove(back_filename)
            exit(2)

        copyfile(back_filename, self.changelog_filename)
        self.git.add(os.path.abspath(self.changelog_filename))

    def get_modified_changelog(self):
        # the changelog might have been modified by the user, so we have to read it again
        f_changelog = codecs.open(self.changelog_filename, 'r', 'utf-8')

        lines = []
        nb_version = 0
        for line in f_changelog:
            # each version are separated by a line like
            # navitia2 (0.94.1) unstable; urgency=low
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
        self.git.submodule('update', '--recursive')
        # merge with the release branch
        self.git.merge(temp_branch, "release", '--no-ff')

        print("current branch {}".format(self.repo.active_branch))
        # we tag the release
        tag_message = u'Version {}\n'.format(self.str_version)

        changelog = self.get_modified_changelog()
        for change in changelog:
            tag_message += change

        print("tag: " + tag_message)

        self.repo.create_tag(get_tag_name(self.version), message=tag_message)

        # and we merge back the release branch to dev (at least for the tag in release)
        self.git.merge("release", "dev", '--no-ff')

        print("publishing the release")

        print("Check the release, you will probably want to merge release in dev:")
        print("  git checkout dev; git submodule update --recursive")
        print("  git merge release")
        print("And when you're happy do:")
        print("  git push {} release dev --tags".format(self.remote_name))
        # TODO: when we'll be confident, we will do that automaticaly

    def release_the_kraken(self, new_version):

        tmp_name = "release_%s" % new_version

        self.checkout_parent_branch()

        # we then create a new temporary branch
        print("creating temporary release branch {}".format(tmp_name))
        self.git.checkout(b=tmp_name)
        print("current branch {}".format(self.repo.active_branch))

        self.update_changelog()

        self.git.commit(m="Version %s" % self.str_version)

        if self.release_type == "hotfix":
            print("now time to do your actual hotfix! (cherry-pick commits)")
            print("PLEASE check that \"release\" COMPILES and TESTS!")
            print("Note: you'll have to merge/tag/push manually after your fix:")
            print("  git checkout release")
            print("  git merge --no-ff {tmp_branch}".format(tmp_branch=tmp_name))
            print(
                "  git tag -a {} #then add message on Version and mention concerned PRs".format(
                    get_tag_name(self.version)
                )
            )
            print("  git checkout dev")
            print("  git merge --ff release")
            print("  git push {} release dev --tags".format(self.remote_name))

            # TODO2 try to script that (put 2 hotfix param, like hotfix init and hotfix publish ?)
            exit(0)

        self.publish_release(tmp_name)


def get_release_type():
    if raw_input("Do you need a binarization ? [Y/n] ").lower() == "y":
        return "major"

    if raw_input("Have you changed the API or Data interface ? [Y/n] ").lower() == "y":
        return "major"

    if raw_input("Are the changes backward compatible ? [Y/n] ").lower() == "y":
        return "minor"

    if raw_input("Are you hotfixing ? [Y/n] ").lower() == "y":
        return "hotfix"

    raise RuntimeError("Couldn't find out the release type")


if __name__ == '__main__':

    if len(argv) < 1:
        print("mandatory argument: {regular|major|minor|hotfix}")
        print("possible additional argument: remote (default is hove-io)")
        exit(5)

    logging.basicConfig(level=logging.INFO)

    release_type = get_release_type()
    remote = argv[1] if len(argv) >= 2 else "hove-io"

    manager = ReleaseManager(release_type, remote_name=remote)

    new_version = manager.get_new_version_number()

    print("Release type: {}".format(release_type))
    print("Release version: {}".format(new_version))
    if raw_input("Shall we proceed ? [Y/n] ").lower() != "y":
        exit(6)

    manager.release_the_kraken(new_version)
