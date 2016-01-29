WIKI
====

This subdirectory is a container for the wiki, which in turn is a
git-sub-module to allow flexibility when creating new projects from
template.

Documentation:
--------------

All documentation of this project is a gitit wiki. Start reading in these
simple steps:

1) Get all sub-modules - documentation is one of them

    git submodule init
    git submodule update

**Note:** The git-module wikidata contains the actual content and may intentionally
differ between git-projects, versions and branches.

2) Install gitit if you don't have it already:

http://gitit.net/Installing

3) Execute the following:

    ./start_wiki.sh

4) Open browser and go to:

[http://localhost:5011](http://localhost:5011)

