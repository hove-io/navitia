************************
How to build a package
************************

Dependencies
==============
* Gpg
* debuild
* dpkg-buildpackage
* git-buildpackage

Build instructions
====================

1. Be sure you have runned the following command at the root of your tree :
 ``git submodule update --init --recursive``

2. Commit or stash all your changes
3. Tag the new release version
    ``git-dch -R -N version --ignore-branch``
    ``commit the changes made to debian/changelog``
4. Build the new version
     ``git-buildpackage --git-tag --git-ignore-branch``

5. Files are available in the parent directory.



