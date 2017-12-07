
# -*- coding: utf-8 -*-
import os
os.environ['LC_ALL'] = 'en_US'
os.environ['GIT_PYTHON_TRACE'] = '1' #can be 0 (no trace), 1 (git commands) or full (git commands + git output)

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
        self.directory = "."
        self.release_type = release_type
        self.repo = Repo(self.directory)
        self.git = self.repo.git

        #we fetch latest version from remote
        self.remote_name = remote_name

        print "fetching from {}...".format(remote_name)
        self.repo.remote(remote_name).fetch("--tags")

        #and we update dev and release branches
        print "rebasing dev and release..."

        #TODO quit on error
        self.git.rebase(remote_name + "/dev", "dev")
        try:
            self.git.checkout("release")
        except Exception as e:
            print "Cannot checkout 'release':{}, creating from distant branch".format(str(e))
            self.git.checkout("-b", "release", remote_name + "/release")
        self.git.rebase(remote_name + "/release", "release")

        print "checking that release was merged into dev..."
        unmerged = self.git.branch("--no-merged", "dev")
        is_release_unmerged = re.search("  release(\n|$)", unmerged)
        if is_release_unmerged:
            print is_release_unmerged.group(0)
            print "ABORTING: {rem}/release branch was not merged in {rem}/dev".format(rem=remote_name)
            print "  This is required before releasing. You may use (be careful):"
            print "    git checkout dev; git submodule update"
            print "    git merge release"
            exit(1)

        print "current branch: {}".format(self.repo.active_branch)

        self.version = None
        self.str_version = ""
        self.latest_tag = ""

        # if API rate limit exceeded use, get 'personal access token' on github then provide:
        # self.auth = ('user', 'pass')
        self.auth = None

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
        self.git.submodule('update')

        print "current branch {}".format(self.repo.active_branch)

    def closed_pr_generator(self):
        # lazy get all closed PR ordered by last updated
        closed_pr = []
        page = 1
        while True:
            query = "https://api.github.com/repos/CanalTP/navitia/" \
                    "pulls?state=closed&base=dev&sort=updated&direction=desc&page={page}"\
                    .format(latest_tag=self.latest_tag, page=page)
            print "query github api: " + query
            github_response = requests.get(query, auth=self.auth)

            if github_response.status_code != 200:
                message = github_response.json()['message']
                print u'  * Impossible to retrieve PR\n  * ' + message
                return

            closed_pr = github_response.json()
            if not closed_pr:
                print "Reached end of PR list"
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
                    branches = self.git.branch('-r', '--contains', pr_head_sha) + '\n'
                except:
                    print "ERROR while searching for commit in release branch: " \
                          "Following PR added to changelog, remove it if needed.\n"

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
                        print lines[-1]
                        nb_successive_merged_pr = 0
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
        self.git.submodule('update')
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

        #and we merge back the release branch to dev (at least for the tag in release)
        self.git.merge("release", "dev", '--no-ff')

        print "publishing the release"

        print "Check the release, you will probably want to merge release in dev:"
        print "  git checkout dev; git submodule update"
        print "  git merge release"
        print "And when you're happy do:"
        print "  git push {} release dev --tags".format(self.remote_name)
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
            print "now time to do your actual hotfix! (cherry-pick commits)"
            print "PLEASE check that \"release\" COMPILES and TESTS!"
            print "Note: you'll have to merge/tag/push manually after your fix:"
            print "  git checkout release"
            print "  git merge --no-ff {tmp_branch}".format(tmp_branch=tmp_name)
            print "  git tag -a {} #then add message on Version and mention concerned PRs \n".format(get_tag_name(self.version))

            print "  git checkout dev"
            print "  git merge release"
            print "  git push {} release dev --tags".format(self.remote_name)

            #TODO2 try to script that (put 2 hotfix param, like hotfix init and hotfix publish ?)
            exit(0)

        self.publish_release(tmp_name)


if __name__ == '__main__':

    if len(argv) < 2:
        print "mandatory argument: {major|minor|hotfix}"
        print "possible additional argument: remote (default is canalTP)"
        exit(5)

    #the git lib used bug when not in english

    logging.basicConfig(level=logging.INFO)

    r_type = argv[1]
    remote = argv[2] if len(argv) >= 3 else "CanalTP"

    manager = ReleaseManager(release_type=r_type, remote_name=remote)
    manager.release_the_kraken()
