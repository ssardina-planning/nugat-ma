# MULTI-AGENT NuGaT Game Solver (for NuSMV 2.6.0+)#

This is the NuGAT 0.6.5+ version of game solver NuGAT built on top of NuSMV model checker, compatible with NuSMV 2.6.0 and able to handle multi-agents (i.e., more than 2 players)

Develped as part of Lorenzo Dibenedetto Master's thesis at RMIT University, supervised by A/Prof. Sebastian Sardina and Dr. Nitin Yadav.

## BACKGROUND ##

NuGAT (https://es.fbk.eu/technologies/nugat-game-solver) was developed at FBK-ES
group. The last version was 0.5.0 which worked with NuSMV 2.5.0, but not newer
versions. 

NuSMV had an important upgrade to 2.5.4.

Then, in June 2015, NuGAT 0.5.0 was ported to 0.5.4 to work with NuSMV 2.5.4 by Nitin Yadav and Sebastian Sardina: [NuGaT 0.5.4 repo](https://bitbucket.org/nitinyadav/nugat-0.5.4)

After that, NuSMV had a major upgrade to version 2.6.0 that changed the whole  architecture: [NuSMV 2.6.0 release news](http://nusmv.fbk.eu/announce-NuSMV-2.6.0.txt)

Then, NuGAT was upgraded, in October 2015, to version 0.6.0 to work with this new NuSMV 2.6.0 as part of Lorenzo Dibenedetto Master's thesis, supervised by 
Sebastian Sardina and Nitin Yadav: [NuGaT 0.6.0 repo](https://bitbucket.org/nugaton/nugat-0.6.0)

Finally, NuGAT was extended in December 2015 to incoporate multi-agent ATL-like verificatoin.


### LOG OF MAJOR CHANGES ###

These are the instructions to make NuGAT work under Linux.

* October 2015 - Version 0.6.0 based on 0.5.4 to work with NuSMV 2.6.0
* June 2015 - Version 0.5.4 based on 0.5.0 to work with NuSMV 2.5.4
* March 2015 - Version 0.5.0 (for NuSMV 2.5.0)
* October 2012 - First install for NuSMV 2.5.0

## INSTALLATION INSTRUCTIONS ##

1. Make sure you have compiled and installed NuSMV 2.6.0 and then set
the following environmet variables:

        export NUSMV_BUILD_DIR=/opt/NuSMV-2.6.0/
        export NUSMV_SOURCE_DIR=~/src/NuSMV/NuSMV-2.6.0/NuSMV/
        
    Remember you can install NuSMV in your own directory:

        cmake .. -DCMAKE_INSTALL_PREFIX=~/opt/nusmv-2.6.0/
        make
        make install


2. Some of the packages needed:

        sudo apt-get install libc6-dev g++ pkg-config automake pkgconf icu-config
        sudo apt-get install flex bison 
        sudo apt-get install libreadline5 libreadline6
        sudo apt-get install libexpat1 libexpat1-dev libxml2-dev liblzma-dev libicu-dev
        sudo apt-get install ruby-libxml 
        sudo apt-get install libncurses5:amd64 libncurses5:i386 libncurses5-dev:amd64 libncurses5-dev:i386

Note: 

- pkgconf is a program which helps to configure compiler and linker flags for development frameworks. It is similar to pkg-config from freedesktop.org. libpkgconf is a library which provides access to most of pkgconf's functionality, to allow other tooling such as compilers and IDEs to discover and use frameworks configured by pkgconf.
 
- icu-config simplifies the task of building and linking against ICU as compared to manually configuring user makefiles or equivalent. Because icu-config is an executable script, it also solves the problem of locating the ICU libraries and headers, by allowing the system PATH to locate it.


3. BUILD WITH CMAKE (newer):

        mkdir build
        cd build
        rm -rf *
        cmake ..        # this generate the Makefiles
        cmake --build . # this generate NuGAT executable

    This should finish a ./NuGaT executable:
        
        [ssardina@Thinkpad-X1 build]$ ./NuGaT 
        *** This is NuGaT 0.6.0 (compiled on Sat Dec 12 00:19:13 UTC 2015)
        *** Enabled addons are: game 
        *** For more information on NuGaT see <http://es.fbk.eu/tools/nugat>
        *** or email to <nugat-users@list.fbk.eu>.
        *** Please report bugs to <nugat@list.fbk.eu>.
        *** Copyright (c) 2010, Fondazione Bruno Kessler

        *** This version of NuGAT-0.6.5 is linked to NuSMV 2.6.0.
        *** For more information on NuSMV see <http://nusmv.fbk.eu>
        *** or email to <nusmv-users@list.fbk.eu>.
        *** Copyright (C) 2010-2014, Fondazione Bruno Kessler

        *** This version of NuGAT-0.6.5 is linked to the CUDD library version 2.4.1
        *** Copyright (c) 1995-2004, Regents of the University of Colorado

        *** This version of NuGAT-0.6.5 is linked to the MiniSat SAT solver. 
        *** See http://minisat.se/MiniSat.html
        *** Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
        *** Copyright (c) 2007-2010, Niklas Sorensson

4. Install binary NuGAT wherever you would like to be in your system.




## USAGE ##

It is assumed that the reader is familiar with concepts of two player games.

Games are implemented as a special mode of NuGaT which is entered when
a source file containing a game is given either as input file when
calling NuGaT or when using the "read_model" or the "read_rat_file"
commands in the NuGaT shell. The mode is left when the "reset" command
is used in the NuGaT shell. While in game mode the NuGaT shell command
help provides an overview of the available commands. Calling a NuGaT
shell command with argument "-h" prints a brief usage message.

NuGaT 0.6.5+ uses extended features to accommodate multi-agents; each
agent is a number. Basically, what it used to be PLAYER_N now it is just N.

The usual keywoerds are still available for players 1 and 2, but there is also
now the ATL "ATLREACHTARGET (n1,n2,n3) spec" stating that team (n1,n2,n3) can 
force/reach the spec. 


Try some examples in examples/

### TWO PLAYER AGENT ONES (as in original NuGaT): ###

    The port of the original NuGaT example (gets same results):
    
    ./NuGaT ../examples/NuGAT/simple.smv

    An example to solve a planning program:

    ./NuGaT -dynamic ../examples/NuGAT/test-nugat.smv


### MULTI-AGENT ONES: ###


    ./NuGaT ../examples/atl/simple-atl-01.smv     # just one query
    ./NuGaT ../examples/atl/simple-atl-02.smv     # multiple queries


    [ssardina@Thinkpad-X1 build]$ ./NuGaT ../examples/atl/simple-atl-02.smv 
    *** This is NuGaT 0.6.5 (compiled on Sun Nov 26 10:59:39 2017)
    *** Enabled addons are: game 
    *** For more information on NuGaT see <http://es.fbk.eu/tools/nugat>
    *** or email to <nugat-users@list.fbk.eu>.
    *** Please report bugs to <Please report bugs to <nusmv-users@fbk.eu>>.
    *** Copyright (c) 2010, Fondazione Bruno Kessler

    *** This version of NuGAT-0.6.5 is linked to NuSMV 2.6.0.
    *** For more information on NuSMV see <http://nusmv.fbk.eu>
    *** or email to <nusmv-users@list.fbk.eu>.
    *** Copyright (C) 2010-2014, Fondazione Bruno Kessler

    *** This version of NuGAT-0.6.5 is linked to the CUDD library version 2.4.1
    *** Copyright (c) 1995-2004, Regents of the University of Colorado

    *** This version of NuGAT-0.6.5 is linked to the MiniSat SAT solver. 
    *** See http://minisat.se/MiniSat.html
    *** Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
    *** Copyright (c) 2007-2010, Niklas Sorensson

    *** WARNING: Game addon does not support properties COI size sorting.  ***
    *** WARNING: Properties COI size sorting will be disabled.             ***
    --   AtlReachTarget 1  x2  : no strategy exists

    --   AtlReachTarget 3  x2  : no strategy exists

    --   AtlReachTarget 4  x2  : no strategy exists

    --   AtlReachTarget 1 3  x2  : no strategy exists

    --   AtlReachTarget 1 4  x2  : no strategy exists

    --   AtlReachTarget 3 4  x2  : no strategy exists

    --   AtlReachTarget 1 3 4  x2  : the strategy has been found



There are more examples in examples/ subdir

or interactively:

    [ssardina@Thinkpad-X1 build]$ ./NuGaT -int
    *** This is NuGaT 0.6.5 (compiled on Sun Nov 26 10:59:39 2017)
    *** Enabled addons are: game 
    *** For more information on NuGaT see <http://es.fbk.eu/tools/nugat>
    *** or email to <nugat-users@list.fbk.eu>.
    *** Please report bugs to <Please report bugs to <nusmv-users@fbk.eu>>.
    *** Copyright (c) 2010, Fondazione Bruno Kessler

    *** This version of NuGAT-0.6.5 is linked to NuSMV 2.6.0.
    *** For more information on NuSMV see <http://nusmv.fbk.eu>
    *** or email to <nusmv-users@list.fbk.eu>.
    *** Copyright (C) 2010-2014, Fondazione Bruno Kessler

    *** This version of NuGAT-0.6.5 is linked to the CUDD library version 2.4.1
    *** Copyright (c) 1995-2004, Regents of the University of Colorado

    *** This version of NuGAT-0.6.5 is linked to the MiniSat SAT solver. 
    *** See http://minisat.se/MiniSat.html
    *** Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
    *** Copyright (c) 2007-2010, Niklas Sorensson

    NuSMV > read_model -i ../examples/atl/simple-atl-02.smv 
    Entering game mode...
    Done entering game mode.
    Note that now game commands apply.
    NuSMV > go
    *** WARNING: Game addon does not support properties COI size sorting.  ***
    *** WARNING: Properties COI size sorting will be disabled.             ***
    NuSMV > build_boolean_model 
    NuSMV > check_property 
    --   AtlReachTarget 1  x2  : no strategy exists

    --   AtlReachTarget 3  x2  : no strategy exists

    --   AtlReachTarget 4  x2  : no strategy exists

    --   AtlReachTarget 1 3  x2  : no strategy exists

    --   AtlReachTarget 1 4  x2  : no strategy exists

    --   AtlReachTarget 3 4  x2  : no strategy exists

    --   AtlReachTarget 1 3 4  x2  : the strategy has been found

    NuGaT > quit
    Exiting game mode...
    Done exiting game mode.
    Note that now the commands from before entering game mode apply.
    %


## COPYRIGHT ##

NuGAT version is licensed under the GNU Lesser General Public License (LGPL in short) as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version. File LGPL-2.1 contains a copy of the License.
For license information on Lily see its documentation.


## CONTACT ##

This extension of NuGAT to multi-agent was carried out by:

* Sebastian Sardina - ssardina@gmail.com 
* Lorenzo Dibenedetto - lorenzodibenedetto90@gmail.com
* Nitin Yadav - nitin.yadav@rmit.edu.au


