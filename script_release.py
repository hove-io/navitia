# -*- coding: utf-8 -*-
from git import *
from datetime import datetime
import subprocess
import re
from sys import exit, argv
from shutil import copyfile
from os import remove, stat
import codecs


AUTHOR_NAME = "Vincent Lara"
AUTHOR_MAIL = "vincent.lara@canaltp.fr"

repo = Repo(".")
git = repo.git

tags = repo.tags

latest_version = None
last_tag = git.describe('--tags', abbrev=0)

version = re.search('.*(\d+\.\d+\.\d+).*', last_tag)
if version:
    latest_version = version.group(1)

if not latest_version:
    print "no latest version found"
    exit(1)

version_n = latest_version.split('.')
print "latest version is %s" % version_n

version_n = [int(i) for i in version_n]

if len(argv) != 2:
    print "mandatory argument: {major|minor|hotfix}"
    exit(5)

if argv[1] == "major":
    version_n[0] += 1
    version_n[1] = version_n[2] = 0
elif argv[1] == "minor":
    version_n[1] += 1
    version_n[2] = 0
elif argv[1] == "hotfix":
    version_n[2] += 1
else:
    exit(5)
version = "%i.%i.%i" % (version_n[0], version_n[1], version_n[2])

tmp_name = "release_%s" % version
tmp_head = git.checkout(b=tmp_name)

write_lines = [
    u'navitia2 (%s) unstable; urgency=low\n' % version,
    u'\n',
]
if argv[1] != "hotfix":
    parents_sha = [c.hexsha for c in last_tag.commit.parents]
    parents_sha.append(last_tag.commit.hexsha)
    for c in repo.iter_commits("dev"):
        if c.hexsha in parents_sha:
            break
        write_lines.append(u' * %s\n'%(unicode(c.message.partition("\n")[0])))
    if len(write_lines):
        write_lines = write_lines[0:50]
        write_lines.append(" * Attention il manque des commits, y en avait trop j'ai pas tout mis")
else:
    write_lines.append(u' * \n')
write_lines.extend([
    u'\n',
    u' -- %s <%s>  %s +0100\n' %(AUTHOR_NAME, AUTHOR_MAIL,
                             datetime.now().strftime("%a, %d %b %Y %H:%m:%S")),
    u'\n',
])
f_changelog = None
changelog_filename = "debian/changelog"
back_filename = changelog_filename + "~"
try:
    f_changelog = codecs.open(changelog_filename, 'r', 'utf-8')
except IOError:
    print "Unable to open debian/changelog"
    exit(1)
f_changelogback = codecs.open(back_filename, "w", "utf-8")

for line in write_lines:
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

cmake = open("source/CMakeLists.txt", "r")
cmake_lines = []
for line in cmake:
    line = re.sub("^SET\(KRAKEN_VERSION_MAJOR (\d+)\)$",
                  lambda matchobj : "SET(KRAKEN_VERSION_MAJOR %i)"%version_n[0], line)
    line = re.sub("^SET\(KRAKEN_VERSION_MINOR (\d+)\)$",
                  lambda matchobj : "SET(KRAKEN_VERSION_MINOR %i)"%version_n[1], line)
    line = re.sub("^SET\(KRAKEN_VERSION_PATCH (\d+)\)$",
                  lambda matchobj : "SET(KRAKEN_VERSION_PATCH %i)"%version_n[2], line)
    cmake_lines.append(line)
cmake.close()
cmake = open("source/CMakeLists.txt", "w")
cmake.writelines(cmake_lines)
cmake.close()

git.add(changelog_filename)
git.add("source/CMakeLists.txt")
git.commit(m="Version %s"%version)
