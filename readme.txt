              _   _        _             _                  _ 
             | | | |_ _   /_\  _ _  __ _| |__  __ _ _ _  __| |
             | |_| | ' \ / _ \| ' \/ _` | '_ \/ _` | ' \/ _` |
              \___/|_||_/_/ \_\_||_\__, |_.__/\__,_|_||_\__,_|
                                    |___/                      
           The Unnamed Angband                  Version 0.6.3
        Copyright (c) 2001-2009 by Andrew Doull and others, except where noted.
        UnAngband 0.5.3 by Andrew Doull, Mogami, and Andrew Sidwell
               Other versions of UnAngband by Andrew Doull.
    Based on:   Moria:   Copyright (c) 1985 Robert Alan Koeneke
                Umoria:   Copyright (c) 1989 James E. Wilson
                Angband 2.0 - 2.4 - 2.6.2 by:
                Alex Cutler, Andy Astrand, Sean Marsh,
                Geoff Hill, Charles Teague, Charles Swiger
                Angband 2.7.0 - 2.8.5 by Ben Harrison
                Angband 2.9.0 - 3.0.x by Robert Ruehlmann
                Oangband 0.6.0 Copyright 1998-2003 Leon Marrick, Bahman Rabii
                EYAngband 0.5.2 By Eytan Zweig
                Sangband 0.9.9 and Demoband by Leon Marrick

     Send comments, bug reports and patches to unangband@yahoogroups.com
         Visit the UnAngband website at http://unangband.blogspot.com/
     Read the Angband Usenet group at news:rec.games.roguelike.angband


            To access the help system, use the '?' key in-game.

=== General Info ===

2004-January-7 - UnAngband 0.5.5: "New Years Resolution".

This is the README file for Unnamed Angband 0.5.5 for 31st March 2003.

For a more recent list of changes, read changes.txt.

Unnamed Angband adds over 100 class/style combinations to Angband,
130 new spells, 60 new spell books and spell related objects, over
500 new monsters, 196 features, 230 room descriptions and slightly
modifies combat and monster spell casting.

There is a context sensitive help feature you are invited to
contribute to.

Classes added include bards, archers, thieves and druids. Bards use
a new type of spell book (Song books), and continue to sing their
spells after initially casting them, letting them simultaneously
perform other actions. Archers are the missile weapon equivelent
of warriors. Thieves are the only class to improve in stealth from
level to level. Druids can cast both mage and priest spells. The
c_info.txt file can be used to create new classes and reconfigure
existing ones. There are a number of internally hard-coded limits
on classes that probably prevent more than 26 total class types.
All class specific abilities are now defined in info files.

More significantly, all classes are now able to specialise in various
spell books, fighting styles or objects, depending on which class
they are a member of. The class gains additional benefits of various
types when using their specialisation. This is detailled in the
w_info.txt file which can also be modified and expanded. There is
a maximum of 32 different styles, for each class, some of which
are not useful for a particular class. Various hard coded
limits probably prevent more than 26 possible styles plus spellbook
substyles per class.

There is a (hack) pretty name function that keeps sub-class names
interesting for the player.

There are 130 new spells of various types, including spells that 
create temporary objects, modify existing objects, modify terrain
and so forth. A number of the new spells are duplicates of existing
spells for other classes. A character can learn up to 64 total spells.

In order to support the new spells, there are 60 new spell books
and spell book related objects. Spellbooks are used by bards for their
spells, but may also be used by priest and mage-type spell casters
although there are many illegible spells in each book for those
casters. Instruments may be used by the bard to improve those spells
he sings for long durations. Temporary spell objects are wielded by
the spell caster as enchanted objects.

Over 500 monsters have been added, with the intention of ensuring
variety in the dungeon, and to make feature-full levels interesting.

Features added to the dungeon include new traps, water, ice, lava,
mud and chasms. These are detailled in f_info.txt and
defined using additional fields and flags documented in the same
file. Monsters should be able to traverse various terrain types
depending on additional flags in r_info.txt. Lakes and rivers
will be generated through some dungeon levels.

Room descriptions have been added. These are displayed when a 
player lites up or enters a room. Room descriptions can also add
extra monsters, objects and features. See d_info.txt for details.

Monsters can now cast spells out of line of sight of the player,
including summon spells. You will be warned in some circumstances
when this is happening. Confused monsters will attack and breath
against other monsters.

Combat has been modified slightly so that melee attacks against
monsters are reduced by the monster AC, much in the same way as
melee attacks against the player are reduced. To compensate,
monster hit points are scaled down by the same factor (Internally),
depending on their armour class. It has the added benefit of making
spell attacks against heavily armoured monsters more powerful.

If a monster is killed and the player has blows remaining, the player
only uses the appropriate fraction of the full energy use for the
round.

Missile weapons now use the same algorithm as melee weapons. Note
that missile weapons do not have their damage reduced by AC,
so they should remain (approximately) as effective.

You can contact the author (Andrew Doull) at andrewdoull@hotmail.com,
or check his home page at http://promisedpoems.editthispage.com/.


==== Information from the Angband Readme file ====

Angband is a "graphical" dungeon adventure game using textual characters
to represent the walls and floors of a dungeon and the inhabitants therein,
in the vein of "rogue", "hack", "nethack", and "moria".

There are some ascii "on line help" files in the "lib/help" directory.

See the Official Angband Home Page "http://thangorodrim.angband.org/" for
a list (mostly complete) of what has changed in each recent version.

See the various Angband ftp sites (including "export.andrew.cmu.edu" and
"ftp.cis.ksu.edu") for the latest files, patches, and executables.

Contact Robert Ruehlmann < rr9@angband.org > to report any bugs or to make
any suggestions.  Use the newsgroup "rec.games.roguelike.angband" to ask
any general questions about the game, including compilation questions.

This version of Angband will run on OS X (10.3 and later), Windows,
Unix (X11/Curses), Linux (X11/Curses), Acorn, Amiga, various DOS machines,
and many others...

See compile.txt, the Makefiles, h-config.h, and config.h for details on
compiling.
See "Makefile.xxx" and "main-xxx.c" for various supported systems.


=== Quick and dirty compilation instructions ===

For many platforms (including OS X and Windows), a "pre-compiled"
archive is available, which contains everything you need to install and
play Angband.  For other platforms, including most UNIX systems, you must
compile the source code yourself.  Try the following non-trivial steps:

Step 1: Acquire.  Ftp to "export.andrew.cmu.edu:/angband/Source"
                  Try "bin" and "mget angband*.tar.gz" and "y"
Step 2: Extract.  Try "gunzip *.gz" then "tar -xvf *.tar"
Step 3: Prepare.  Try "cd angband*/src", then edit "Makefile"
                  You may also edit "h-config.h" and "config.h"
Step 4: Compile.  Try "make", and then "cd .." if successful
Step 5: Execute.  Try "angband -uTest" to initialize stuff
Step 6: Play....  Read the "online help" via the "?" command.

Of course, if you have a compiler, you can compile a (possibly customized)
executable on almost any system.  You will need the "source archive" (as
above), which contains the standard "src" and "lib" directories, and for
some platforms (including Macintosh and Windows), you will also need an
appropriate "extra archive", which contains some extra platform specific
files, and instructions about how to use them.  Some "extra archives" may
be found at the ftp site (including "/angband/Macintosh/ext-mac.sit.bin"
and "/angband/Windows/ext-win.zip"), but be sure that you get a version
of the "extra archive" designed for Angband 2.9.0.


=== Special instructions for certain platforms ===

This OS X version works on only for OS X versions 10.3 and later.
For legacy OSX/Mac (QuickDraw), use main-crb.c or main-mac.c from Angband
releases 3.0.x and earlier.

Some archive generation programs refuse to handle empty directories,
so special "fake" files with names like "DELETEME.TXT" may have been
placed into certain directories to avoid this problem.  You may safely
delete these files if you so desire.


=== Upgrading from older versions (and/or other platforms) ===

If you have been using an older version of Angband (and/or playing on a
different platform), you can "upgrade" (or "sidegrade") to Angband 2.9.0,
bringing your old savefiles, high score list, and other files with you.

Angband 2.9.0 uses a platform independant file format for the binary files
that store information about games in progress, known as "savefiles", and
is able to translate savefiles from all known versions of Angband.  To use
an "old" savefile, simply copy it into the "lib/save" directory, changing
the name of the savefile (if necessary) to satisfy the requirements of the
platform you are using.  In general, the savefile should be named "UUU.NNN"
or "NNN" where "UUU" is the userid of the player (on "multiuser" systems),
and "NNN" is the name of the "character" in the savefile.  Note that only
"multiuser" platforms use the "UUU.NNN" form, and the "dot" is required.

Angband 2.9.0 uses a platform independant file format for the binary file
used to store the high score list.  This file is named "scores.raw".  To
use an "old" high score list, simply copy it into the "lib/apex" directory.

Angband 2.9.0 uses a set of special ascii "configuration files" which are
kept in the "lib/file" directory.  These files should not be modified (or
imported from older versions) unless you know exactly what you are doing,
but often you can use "old" versions of these files with little trouble.

Angband 2.9.0 uses a set of ascii "user pref files" which are kept in the
"lib/user" directory.  Most of these files can only be used on a small set
of platforms, and may need slight modifications when imported from older
versions.  Note that only some of these files are auto-loaded by the game.


=== Directory "src" ===

The "src" directory contains the complete set of "standard" source files.


=== Directory "lib" ===

The "lib" directory contains all of Angband's special sub-directories.


=== Directory "lib/apex" ===

The "lib/apex" directory contains the "high score" files.

The "scores.raw" file contains the "high score" table, in a "semi-binary" form,
that is, all the bytes in the file are normal ascii values, but this includes
the special "nul" or "zero" byte, which is used to separate and pad records.
You should probably not attempt to modify this file with a normal text editor.
This file should be (more or less) portable between different platforms.  It
must be present (or creatable) for the game to run correctly.


=== Directory "lib/bone" ===

The "lib/bone" directory is currently unused.


=== Directory "lib/data" ===

The "lib/data" directory contains various special binary data files.

The files 'f_info.raw', 'k_info.raw', 'a_info.raw', 'e_info.raw', 'r_info.raw',
and 'v_info.raw' are binary image files constructed by parsing the ascii
template files in "lib/edit", described below.  These files are required,
but can be created by the game if the "lib/edit" directory contains the
proper files, and if the game was compiled to allow this creation.


=== Directory "lib/edit" ===

The "lib/edit" directory contains various special ascii data files.

The files 'f_info.txt', 'k_info.txt', 'a_info.txt', 'e_info.txt', 'r_info.txt',
and 'v_info.txt' are ascii template files used to construct the binary image
files in "lib/data", described above.  These files describe the "terrain
features", "object kinds", "artifacts", "ego-items", "monster races", and
"dungeon vaults", respectively.

The ascii template files are easier to edit than hard-coded arrays, and also
prevent compilation errors on some machines, and also shrink the size of the
binary executable, and also provide a user-readible spoiler file of sorts.

These files should not be modified unless you know exactly what you are doing.

These files are optional if the game is distributed with pre-created
binary raw files in "lib/data".


=== Directory "lib/file" ===

The "lib/file" directory contains various special ascii data files.

The 'news.txt' file is displayed to the user when the game starts up.  It
contains basic information such as my name and email address, and the names
of some of the people who have been responsible for previous versions of
Angband.  You may edit this file (slightly) to include local "site specific"
information such as who compiled the local executable.  You should refer the
user to a special "online help" file, if necessary, that describes any local
modifications in detail.  The first two lines of this file should be blank,
and only the next 20 lines should contain information.

The 'dead.txt' file is displayed to the user when the player dies.  It
contains a picture of a tombstone which is filled in with interesting
information about the dead player.  You should not edit this file.

The optional file 'wizards.txt' may be used to specify which users may enter
'wizard' mode.  A missing file provides no restrictions, and an empty file
prevents everyone from entering 'wizard' mode.  This file is only used on
multi-user machines, otherwise there are no restrictions.

The optional file 'time.txt' may be used to restrict the "times" at which
the game may be played, by providing specification of which hours of each day
of the week are legal for playing the game.  See 'files.c' for more details.
A missing file provides no restrictions, and an empty file will, by default,
forbid the playing of the game from 8am-5pm on weekdays.  This file is only
used on multi-user machines, and only if CHECK_TIME is defined, otherwise,
there are no restrictions.

The optional file 'load.txt' may be used to restrict the "load" which the game
may impose on the system.  See 'files.c' for more details.  A missing file
provides no restrictions, and an empty file will, by default, restrict the
"current load" to a maximal value of 100*FSCALE.  This file is only used on
multi-user machines, and only if CHECK_LOAD is defined, otherwise, there are
no restrictions.

These files should not be modified unless you know exactly what you are doing.


=== Directory "lib/help" ===

The "lib/help" directory contains the "online help" files.

This directory is used to search for normal "online help" files.


=== Directory "lib/info" ===

The "lib/info" directory contains the "online spoiler" files.

This directory is used to search for any "online help" file that cannot
be found in the "lib/help" directory.

This directory is empty by default.  Many people use this directory for
"online spoiler files", many of which are available.

Note that the default "help.hlp" file allows the "9" key to access a help
file called "spoiler.hlp", and allows the "0" key to access "user.hlp".

These special help files can thus be placed in the user's own "info"
directory to allow the on line help to access his files.


=== Directory "lib/save" ===

The "lib/save" directory contains "savefiles" for the players.

Each savefile is named "NNN" where "NNN" is the name of the character, or,
on some machines, the name of the character, or, on multi-user machines,
"UUU.NNN", where "UUU" is the player uid and "NNN" is the character name.

The savefiles should be portable between systems, assuming that the
appropriate renaming is perfomed.


=== Directory "lib/user" ===

The "lib/user" directory contains the "user pref files", if any.

In general, these files are used to "customize" aspects of the game for
a given site or a given player.

See "src/files.c" for information on the proper "format" of these files.


=== Directory "lib/xtra" ===

The "lib/xtra" directory contains special system files, if any.




=== NO WARRANTY ===

    BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

    IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.


--- Ben Harrison and Robert Ruehlmann ---

