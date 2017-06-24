/* File: init2.c */


/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 *
 * UnAngband (c) 2001-2009 Andrew Doull. Modifications to the Angband 2.9.6
 * source code are released under the Gnu Public License. See www.fsf.org
 * for current GPL license details. Addition permission granted to
 * incorporate modifications in all Angband variants as defined in the
 * Angband variants FAQ. See rec.games.roguelike.angband for FAQ.
 */

#include "angband.h"
#include "option.h"
#include "init.h"

/*
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 *
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 *
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 *
 * The "init1.c" file is used only to parse the ascii template files,
 * to create the binary image files.  If you include the binary image
 * files instead of the ascii template files, then you can undefine
 * "ALLOW_TEMPLATES", saving about 20K by removing "init1.c".  Note
 * that the binary image files are extremely system dependant.
 */



/*
 * Find the default paths to all of our important sub-directories.
 *
 * The purpose of each sub-directory is described in "variable.c".
 *
 * All of the sub-directories should, by default, be located inside
 * the main "lib" directory, whose location is very system dependant.
 *
 * This function takes a writable buffer, initially containing the
 * "path" to the "lib" directory, for example, "/pkg/lib/angband/",
 * or a system dependant string, for example, ":lib:".  The buffer
 * must be large enough to contain at least 32 more characters.
 *
 * Various command line options may allow some of the important
 * directories to be changed to user-specified directories, most
 * importantly, the "info" and "user" and "save" directories,
 * but this is done after this function, see "main.c".
 *
 * In general, the initial path should end in the appropriate "PATH_SEP"
 * string.  All of the "sub-directory" paths (created below or supplied
 * by the user) will NOT end in the "PATH_SEP" string, see the special
 * "path_build()" function in "util.c" for more information.
 *
 * Mega-Hack -- support fat raw files under NEXTSTEP, using special
 * "suffixed" directories for the "ANGBAND_DIR_DATA" directory, but
 * requiring the directories to be created by hand by the user.
 *
 * Hack -- first we free all the strings, since this is known
 * to succeed even if the strings have not been allocated yet,
 * as long as the variables start out as "NULL".  This allows
 * this function to be called multiple times, for example, to
 * try several base "path" values until a good one is found.
 */
void init_file_paths(char *path)
{
	char *tail;

#ifdef PRIVATE_USER_PATH
	char dirpath[1024];
	char buf[1024];
#endif

	/*** Free everything ***/

	/* Free the main path */
	string_free(ANGBAND_DIR);

	/* Free the sub-paths */
	string_free(ANGBAND_DIR_APEX);
	string_free(ANGBAND_DIR_BONE);
	string_free(ANGBAND_DIR_DATA);
	string_free(ANGBAND_DIR_EDIT);
	string_free(ANGBAND_DIR_FILE);
	string_free(ANGBAND_DIR_HELP);
	string_free(ANGBAND_DIR_INFO);
	string_free(ANGBAND_DIR_SAVE);
	string_free(ANGBAND_DIR_PREF);
	string_free(ANGBAND_DIR_USER);
	string_free(ANGBAND_DIR_XTRA);


	/*** Prepare the "path" ***/

	/* Hack -- save the main directory */
	ANGBAND_DIR = string_make(path);

	/* Prepare to append to the Base Path */
	tail = path + strlen(path);


#ifdef VM


	/*** Use "flat" paths with VM/ESA ***/

	/* Use "blank" path names */
	ANGBAND_DIR_APEX = string_make("");
	ANGBAND_DIR_BONE = string_make("");
	ANGBAND_DIR_DATA = string_make("");
	ANGBAND_DIR_EDIT = string_make("");
	ANGBAND_DIR_FILE = string_make("");
	ANGBAND_DIR_HELP = string_make("");
	ANGBAND_DIR_INFO = string_make("");
	ANGBAND_DIR_SAVE = string_make("");
	ANGBAND_DIR_PREF = string_make("");
	ANGBAND_DIR_USER = string_make("");
	ANGBAND_DIR_XTRA = string_make("");


#else /* VM */


	/*** Build the sub-directory names ***/

	/* Build a path name */
	strcpy(tail, "edit");
	ANGBAND_DIR_EDIT = string_make(path);

	/* Build a path name */
	strcpy(tail, "file");
	ANGBAND_DIR_FILE = string_make(path);

	/* Build a path name */
	strcpy(tail, "help");
	ANGBAND_DIR_HELP = string_make(path);

	/* Build a path name */
	strcpy(tail, "info");
	ANGBAND_DIR_INFO = string_make(path);

	/* Build a path name */
	strcpy(tail, "pref");
	ANGBAND_DIR_PREF = string_make(path);

#ifdef PRIVATE_USER_PATH
	/* Use absolute path -- ~/ doesn't work reliably on OS X */
	path_parse(dirpath, sizeof(dirpath), PRIVATE_USER_PATH);

    /* Build the path to the user specific sub-directory */
	path_build(buf, sizeof(buf), dirpath, VERSION_NAME);

    /* Build a relative path name */
    ANGBAND_DIR_USER = string_make(buf);

#else

    /* Build a path name */
    strcpy(tail, "user");
    ANGBAND_DIR_USER = string_make(path);

#endif /* PRIVATE_USER_PATH */

#ifdef PRIVATE_USER_PATHS

    /* Build the path to the user specific sub-directory */
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER , "scores");

    /* Build a relative path name */
    ANGBAND_DIR_APEX = string_make(buf);

    /* Build the path to the user specific sub-directory */
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER , "bone");

    /* Build a relative path name */
    ANGBAND_DIR_BONE = string_make(buf);

    /* Build the path to the user specific sub-directory */
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER , "data");

    /* Build a relative path name */
    ANGBAND_DIR_DATA = string_make(buf);

    /* Build the path to the user specific sub-directory */
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER , "save");

    /* Build a relative path name */
    ANGBAND_DIR_SAVE = string_make(buf);

#else /* PRIVATE_USER_PATHS */

	/* Build a path name */
	strcpy(tail, "apex");
	ANGBAND_DIR_APEX = string_make(path);

	/* Build a path name */
	strcpy(tail, "bone");
	ANGBAND_DIR_BONE = string_make(path);

	/* Build a path name */
	strcpy(tail, "data");
	ANGBAND_DIR_DATA = string_make(path);

	/* Build a path name */
	strcpy(tail, "save");
	ANGBAND_DIR_SAVE = string_make(path);

#endif /* PRIVATE_USER_PATHS */

	/* Build a path name */
	strcpy(tail, "xtra");
	ANGBAND_DIR_XTRA = string_make(path);

#endif /* VM */


#ifdef NeXT

	/* Allow "fat binary" usage with NeXT */
	if (TRUE)
	{
		cptr next = NULL;

# if defined(m68k)
		next = "m68k";
# endif

# if defined(i386)
		next = "i386";
# endif

# if defined(sparc)
		next = "sparc";
# endif

# if defined(hppa)
		next = "hppa";
# endif

		/* Use special directory */
		if (next)
		{
			/* Forget the old path name */
			string_free(ANGBAND_DIR_DATA);

			/* Build a new path name */
			sprintf(tail, "data-%s", next);
			ANGBAND_DIR_DATA = string_make(path);
		}
	}

#endif /* NeXT */

}

#ifdef PRIVATE_USER_PATH

/*
 * Create an ".angband/" directory in the users home directory.
 *
 * ToDo: Add error handling.
 * ToDo: Only create the directories when actually writing files.
 */
void create_user_dirs(void)
{
	char dirpath[1024];
	char subdirpath[1024];


	/* Get an absolute path from the filename */
	path_parse(dirpath, sizeof(dirpath), PRIVATE_USER_PATH);

	/* Create the ~/.angband/ directory */
	mkdir(dirpath, 0700);

	/* Build the path to the variant-specific sub-directory */
	path_build(subdirpath, sizeof(subdirpath), dirpath, VERSION_NAME);

	/* Create the directory */
	mkdir(subdirpath, 0700);

#ifdef PRIVATE_USER_PATHS
	/* Build the path to the scores sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "scores");

	/* Create the directory */
	mkdir(dirpath, 0700);

	/* Build the path to the savefile sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "bone");

	/* Create the directory */
	mkdir(dirpath, 0700);

	/* Build the path to the savefile sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "data");

	/* Create the directory */
	mkdir(dirpath, 0700);

	/* Build the path to the savefile sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "save");

	/* Create the directory */
	mkdir(dirpath, 0700);
#endif /* PRIVATE_USER_PATHS */
}

#endif /* PRIVATE_USER_PATH */


#ifdef ALLOW_TEMPLATES


/*
 * Hack -- help give useful error messages
 */
int error_idx;
int error_line;


/*
 * Standard error message text
 */
static cptr err_str[PARSE_ERROR_MAX] =
{
	NULL,
	"parse error",
	"obsolete file",
	"missing record header",
	"non-sequential records",
	"invalid flag specification",
	"undefined directive",
	"out of memory",
	"value out of bounds",
	"too few arguments",
	"too many arguments",
	"too many allocation entries",
	"invalid spell frequency",
	"invalid number of items (0-99)",
	"too many entries",
};


#endif /* ALLOW_TEMPLATES */


/*
 * File headers
 */
header z_head;
header v_head;
header d_head;
header method_head;
header effect_head;
header region_head;
header f_head;
header k_head;
header a_head;
header n_head;
header e_head;
header x_head;
header r_head;
header p_head;
header c_head;
header s_head;
header w_head;
header y_head;
header t_head;
header u_head;
header h_head;
header b_head;
header g_head;
header q_head;



/*** Initialize from binary image files ***/


/*
 * Initialize a "*_info" array, by parsing a binary "image" file
 */
static errr init_info_raw(int fd, header *head)
{
	header test;


	/* Read and verify the header */
	if (fd_read(fd, (char*)(&test), sizeof(header)) ||
	    (test.v_major != head->v_major) ||
	    (test.v_minor != head->v_minor) ||
	    (test.v_patch != head->v_patch) ||
	    (test.v_extra != head->v_extra) ||
	    (test.info_num != head->info_num) ||
	    (test.info_len != head->info_len) ||
	    (test.head_size != head->head_size) ||
	    (test.info_size != head->info_size))
	{
		/* Error */
		return (-1);
	}


	/* Accept the header */
	COPY(head, &test, header);


	/* Allocate the "*_info" array */
	head->info_ptr = C_ZNEW(head->info_size, char);

	/* Read the "*_info" array */
	fd_read(fd, (char*)head->info_ptr, head->info_size);

	if (head->name_size)
	{
		/* Allocate the "*_name" array */
		head->name_ptr = C_ZNEW(head->name_size, char);

		/* Read the "*_name" array */
		fd_read(fd, head->name_ptr, head->name_size);
	}

	if (head->text_size)
	{
		/* Allocate the "*_text" array */
		head->text_ptr = C_ZNEW(head->text_size, char);

		/* Read the "*_text" array */
		fd_read(fd, head->text_ptr, head->text_size);
	}

	/* Success */
	return (0);
}


/*
 * Initialize the header of an *_info.raw file.
 */
static void init_header(header *head, int num, int len)
{
	/* Save the "version" */
	head->v_major = VERSION_MAJOR;
	head->v_minor = VERSION_MINOR;
	head->v_patch = VERSION_PATCH;
	head->v_extra = VERSION_EXTRA;

	/* Save the "record" information */
	head->info_num = num;
	head->info_len = len;

	/* Save the size of "*_head" and "*_info" */
	head->head_size = sizeof(header);
	head->info_size = head->info_num * head->info_len;

	/* Clear evaluation and emission functions */
	head->eval_info_power = NULL;
	head->emit_info_txt_index = NULL;
	head->emit_info_txt_always = NULL;
}


#ifdef ALLOW_TEMPLATES

/*
 * Display a parser error message.
 */
static void display_parse_error(cptr filename, errr err, cptr buf)
{
	cptr oops;

	/* Error string */
	oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");

	/* Oops */
	msg_format("Error at line %d of '%s.txt'.", error_line, filename);
	msg_format("Record %d contains a '%s' error.", error_idx, oops);
	msg_format("Parsing '%s'.", buf);
	message_flush();

	/* Quit */
	quit_fmt("Error in '%s.txt' file.", filename);
}

#endif /* ALLOW_TEMPLATES */


/*
 * Initialize a "*_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_info(cptr filename, header *head)
{
	int fd;

	errr err = 1;

#ifdef ALLOW_TEMPLATES
	FILE *fp;
#endif

#ifdef ALLOW_TEMPLATES_OUTPUT
	FILE *fpout;
#endif

	/* General buffer */
	char buf[1024];


#ifdef ALLOW_TEMPLATES

	/*** Load the binary image file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_DATA, format("%s.raw", filename));

	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);

	/* Process existing "raw" file */
	if (fd >= 0)
	{
#ifdef CHECK_MODIFICATION_TIME

		err = check_modification_date(fd, format("%s.txt", filename));

#endif /* CHECK_MODIFICATION_TIME */

		/* Attempt to parse the "raw" file */
		if (!err)
			err = init_info_raw(fd, head);

		/* Close it */
		fd_close(fd);
	}

	/* Do we have to parse the *.txt file? */
	if (err)
	{
		/*** Make the fake arrays ***/

		/* Allocate the "*_info" array */
		head->info_ptr = C_ZNEW(head->info_size, char);

		/* MegaHack -- make "fake" arrays */
		if (z_info)
		{
			head->name_ptr = C_ZNEW(z_info->fake_name_size, char);
			head->text_ptr = C_ZNEW(z_info->fake_text_size, char);
		}


		/*** Load the ascii template file ***/

		/* Build the filename */
		path_build(buf, 1024, ANGBAND_DIR_EDIT, format("%s.txt", filename));

		/* Open the file */
		fp = my_fopen(buf, "r");

		/* Parse it */
		if (!fp) quit(format("Cannot open '%s.txt' file.", filename));

		/* Parse and output the files */
		err = init_info_txt(fp, buf, head, head->parse_info_txt);

		/* Close it */
		my_fclose(fp);

		/* Errors */
		if (err) display_parse_error(filename, err, buf);

		/* Post processing the data */
		if (head->eval_info_power) eval_info(head->eval_info_power, head);

#ifdef ALLOW_TEMPLATES_OUTPUT

		/*** Output a 'evaluated' ascii template file ***/
		if ((head->emit_info_txt_index) || (head->emit_info_txt_always))
		{
			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_EDIT, format("%s.txt", filename));

			/* Open the file */
			fp = my_fopen(buf, "r");

			/* Parse it */
			if (!fp) quit(format("Cannot open '%s.txt' file for re-parsing.", filename));

			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, format("%s.txt", filename));

			/* Open the file */
			fpout = my_fopen(buf, "w");

			/* Parse it */
			if (!fpout) quit(format("Cannot open '%s.txt' file for output.", filename));

			/* Parse and output the files */
			err = emit_info_txt(fpout, fp, buf, head, head->emit_info_txt_index, head->emit_info_txt_always);

			/* Close both files */
			my_fclose(fpout);
			my_fclose(fp);
		}

#endif


		/*** Dump the binary image file ***/

		/* File type is "DATA" */
		FILE_TYPE(FILE_TYPE_DATA);

		/* Build the filename */
		path_build(buf, 1024, ANGBAND_DIR_DATA, format("%s.raw", filename));


		/* Attempt to open the file */
		fd = fd_open(buf, O_RDONLY);

		/* Failure */
		if (fd < 0)
		{
			int mode = 0644;

			/* Grab permissions */
			safe_setuid_grab();

			/* Create a new file */
			fd = fd_make(buf, mode);

			/* Drop permissions */
			safe_setuid_drop();

			/* Failure */
			if (fd < 0)
			{
				char why[1024];

				/* Message */
				sprintf(why, "Cannot create the '%s' file!", buf);

				/* Crash and burn */
				quit(why);
			}
		}

		/* Close it */
		fd_close(fd);

		/* Grab permissions */
		safe_setuid_grab();

		/* Attempt to create the raw file */
		fd = fd_open(buf, O_WRONLY);

		/* Drop permissions */
		safe_setuid_drop();

		/* Dump to the file */
		if (fd >= 0)
		{
			/* Dump it */
			fd_write(fd, (cptr)head, head->head_size);

			/* Dump the "*_info" array */
			fd_write(fd, (char*)head->info_ptr, head->info_size);

			/* Dump the "*_name" array */
			fd_write(fd, head->name_ptr, head->name_size);

			/* Dump the "*_text" array */
			fd_write(fd, head->text_ptr, head->text_size);

			/* Close */
			fd_close(fd);
		}


		/*** Kill the fake arrays ***/

		/* Free the "*_info" array */
		FREE(head->info_ptr);

		/* MegaHack -- Free the "fake" arrays */
		if (z_info)
		{
			FREE(head->name_ptr);
			FREE(head->text_ptr);
		}

#endif /* ALLOW_TEMPLATES */


		/*** Load the binary image file ***/

		/* Build the filename */
		path_build(buf, 1024, ANGBAND_DIR_DATA, format("%s.raw", filename));

		/* Attempt to open the "raw" file */
		fd = fd_open(buf, O_RDONLY);

		/* Process existing "raw" file */
		if (fd < 0) quit(format("Cannot load '%s.raw' file.", filename));

		/* Attempt to parse the "raw" file */
		err = init_info_raw(fd, head);

		/* Close it */
		fd_close(fd);

		/* Error */
		if (err) quit(format("Cannot parse '%s.raw' file.", filename));

#ifdef ALLOW_TEMPLATES
	}
#endif /* ALLOW_TEMPLATES */

	/* Success */
	return (0);
}


/*
 * Free the allocated memory for the info-, name-, and text- arrays.
 */
static errr free_info(header *head)
{
	if (head->info_size)
		FREE(head->info_ptr);

	if (head->name_size)
		FREE(head->name_ptr);

	if (head->text_size)
		FREE(head->text_ptr);

	/* Success */
	return (0);
}


/*
 * Initialize the "z_info" array
 */
static errr init_z_info(void)
{
	errr err;

	/* Init the header */
	init_header(&z_head, 1, sizeof(maxima));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	z_head.parse_info_txt = parse_z_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("limits", &z_head);

	/* Set the global variables */
	z_info = (maxima*)z_head.info_ptr;

	return (err);
}


/*
 * Initialize the "d_info" array
 */
static errr init_d_info(void)
{
	errr err;

	/* Init the header */
	init_header(&d_head, z_info->d_max, sizeof(desc_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	d_head.parse_info_txt = parse_d_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	d_head.emit_info_txt_always = emit_d_info_always;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("room", &d_head);

	/* Set the global variables */
	d_info = (desc_type*)d_head.info_ptr;
	d_name = d_head.name_ptr;
	d_text = d_head.text_ptr;

	return (err);
}


/*
 * Initialize the "blow_info" array
 */
static errr init_method_info(void)
{
	errr err;

	/* Init the header */
	init_header(&method_head, z_info->method_max, sizeof(method_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	method_head.parse_info_txt = parse_method_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emit function*/
	method_head.emit_info_txt_index = emit_method_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("blows", &method_head);

	/* Set the global variables */
	method_info = (method_type*)method_head.info_ptr;
	method_name = method_head.name_ptr;
	method_text = method_head.text_ptr;

	return (err);
}


/*
 * Initialize the "effect_info" array
 */
static errr init_effect_info(void)
{
	errr err;

	/* Init the header */
	init_header(&effect_head, z_info->effect_max, sizeof(effect_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	effect_head.parse_info_txt = parse_effect_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emit function*/
	effect_head.emit_info_txt_index = emit_effect_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("effect", &effect_head);

	/* Set the global variables */
	effect_info = (effect_type*)effect_head.info_ptr;
	effect_name = effect_head.name_ptr;
	effect_text = effect_head.text_ptr;

	return (err);
}


/*
 * Initialize the "region_info" array
 */
static errr init_region_info(void)
{
	errr err;

	/* Init the header */
	init_header(&region_head, z_info->region_info_max, sizeof(region_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	region_head.parse_info_txt = parse_region_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("region", &region_head);

	/* Set the global variables */
	region_info = (region_info_type*)region_head.info_ptr;
	region_name = region_head.name_ptr;
	region_text = region_head.text_ptr;

	return (err);
}


/*
 * Initialize the "f_info" array
 */
static errr init_f_info(void)
{
	errr err;

	/* Init the header */
	init_header(&f_head, z_info->f_max, sizeof(feature_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	f_head.parse_info_txt = parse_f_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emit function*/
	f_head.emit_info_txt_index = emit_f_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("terrain", &f_head);

	/* Set the global variables */
	f_info = (feature_type*)f_head.info_ptr;
	f_name = f_head.name_ptr;
	f_text = f_head.text_ptr;

	return (err);
}



/*
 * Initialize the "k_info" array
 */
static errr init_k_info(void)
{
	errr err;

	/* Init the header */
	init_header(&k_head, z_info->k_max, sizeof(object_kind));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	k_head.parse_info_txt = parse_k_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emit function*/
	k_head.emit_info_txt_index = emit_k_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("object", &k_head);

	/* Set the global variables */
	k_info = (object_kind*)k_head.info_ptr;
	k_name = k_head.name_ptr;
	k_text = k_head.text_ptr;

	return (err);
}



/*
 * Initialize the "a_info" array
 */
static errr init_a_info(void)
{
	errr err;

	/* Init the header */
	init_header(&a_head, z_info->a_max, sizeof(artifact_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	a_head.parse_info_txt = parse_a_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emit function*/
	a_head.emit_info_txt_index = emit_a_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("artifact", &a_head);

	/* Set the global variables */
	a_info = (artifact_type*)a_head.info_ptr;
	a_name = a_head.name_ptr;
	a_text = a_head.text_ptr;

	return (err);
}



/*
 * Initialize the "n_info" structure
 */
static errr init_n_info(void)
{
  errr err;

  /* Init the header */
  init_header(&n_head, 1, sizeof(names_type));

#ifdef ALLOW_TEMPLATES

  /* Save a pointer to the parsing function */
  n_head.parse_info_txt = parse_n_info;

#endif /* ALLOW_TEMPLATES */

  err = init_info("names", &n_head);

  n_info = (names_type*)n_head.info_ptr;

  return (err);
}


/*
 * Initialize the "e_info" array
 */
static errr init_e_info(void)
{
	errr err;

	/* Init the header */
	init_header(&e_head, z_info->e_max, sizeof(ego_item_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	e_head.parse_info_txt = parse_e_info;

	/* Save a pointer to the evaluate power function*/
	e_head.eval_info_power = eval_e_power;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	e_head.emit_info_txt_index = emit_e_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("ego_item", &e_head);

	/* Set the global variables */
	e_info = (ego_item_type*)e_head.info_ptr;
	e_name = e_head.name_ptr;
	e_text = e_head.text_ptr;

	return (err);
}


/*
 * Initialize the "x_info" array
 */
static errr init_x_info(void)
{
	errr err;

	/* Init the header */
	init_header(&x_head, z_info->x_max, sizeof(flavor_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	x_head.parse_info_txt = parse_x_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	x_head.emit_info_txt_index = emit_x_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("flavor", &x_head);

	/* Set the global variables */
	x_info = (flavor_type*)x_head.info_ptr;
	x_name = x_head.name_ptr;
	x_text = x_head.text_ptr;

	return (err);
}


/*
 * Initialize the "r_info" array
 */
static errr init_r_info(void)
{
	errr err;

	/* Init the header */
	init_header(&r_head, z_info->r_max, sizeof(monster_race));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	r_head.parse_info_txt = parse_r_info;

	/* Save a pointer to the evaluate power function*/
	r_head.eval_info_power = eval_r_power;

#ifdef ALLOW_TEMPLATES_OUTPUT

	r_head.emit_info_txt_index = emit_r_info_index;

#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("monster", &r_head);

	/* Set the global variables */
	r_info = (monster_race*)r_head.info_ptr;
	r_name = r_head.name_ptr;
	r_text = r_head.text_ptr;

	return (err);
}



/*
 * Initialize the "v_info" array
 */
static errr init_v_info(void)
{
	errr err;

	/* Init the header */
	init_header(&v_head, z_info->v_max, sizeof(vault_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	v_head.parse_info_txt = parse_v_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	v_head.emit_info_txt_index = emit_v_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("vault", &v_head);

	/* Set the global variables */
	v_info = (vault_type*)v_head.info_ptr;
	v_name = v_head.name_ptr;
	v_text = v_head.text_ptr;

	return (err);
}


/*
 * Initialize the "p_info" array
 */
static errr init_p_info(void)
{
	errr err;

	/* Init the header */
	init_header(&p_head, z_info->p_max, sizeof(player_race));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	p_head.parse_info_txt = parse_p_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	p_head.emit_info_txt_index = emit_p_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("p_race", &p_head);

	/* Set the global variables */
	p_info = (player_race*)p_head.info_ptr;
	p_name = p_head.name_ptr;
	p_text = p_head.text_ptr;

	return (err);
}


/*
 * Initialize the "c_info" array
 */
static errr init_c_info(void)
{
	errr err;

	/* Init the header */
	init_header(&c_head, z_info->c_max, sizeof(player_class));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	c_head.parse_info_txt = parse_c_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	c_head.emit_info_txt_index = emit_c_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */
#endif /* ALLOW_TEMPLATES */

	err = init_info("p_class", &c_head);

	/* Set the global variables */
	c_info = (player_class*)c_head.info_ptr;
	c_name = c_head.name_ptr;
	c_text = c_head.text_ptr;

	return (err);
}


/*
 * Initialize the "w_info" array
 */
static errr init_w_info(void)
{
	errr err;

	/* Init the header */
	init_header(&w_head, z_info->w_max, sizeof(weapon_style));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	w_head.parse_info_txt = parse_w_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("style", &w_head);

	/* Set the global variables */
	w_info = (weapon_style*)w_head.info_ptr;

	return (err);
}


/*
 * Initialize the "s_info" array
 */
static errr init_s_info(void)
{
	errr err;

	/* Init the header */
	init_header(&s_head, z_info->s_max, sizeof(spell_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	s_head.parse_info_txt = parse_s_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	s_head.emit_info_txt_index = emit_s_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("spell", &s_head);

	/* Set the global variables */
	s_info = (spell_type*)s_head.info_ptr;
	s_name = s_head.name_ptr;
	s_text = s_head.text_ptr;

	return (err);
}


/*
 * Initialize the "y_info" array
 */
static errr init_y_info(void)
{
	errr err;

	/* Init the header */
	init_header(&y_head, z_info->y_max, sizeof(rune_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	y_head.parse_info_txt = parse_y_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("rune", &y_head);

	/* Set the global variables */
	y_info = (rune_type*)y_head.info_ptr;
	y_name = y_head.name_ptr;
	y_text = y_head.text_ptr;

	return (err);
}


/*
 * Initialize the "h_info" array
 */
static errr init_h_info(void)
{
	errr err;

	/* Init the header */
	init_header(&h_head, z_info->h_max, sizeof(hist_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	h_head.parse_info_txt = parse_h_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("p_hist", &h_head);

	/* Set the global variables */
	h_info = (hist_type*)h_head.info_ptr;
	h_text = h_head.text_ptr;

	return (err);
}



/*
 * Initialize the "t_info" array
 */
static errr init_t_info(void)
{
	errr err;

	/* Init the header */
	init_header(&t_head, z_info->t_max, sizeof(town_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	t_head.parse_info_txt = parse_t_info;

#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	t_head.emit_info_txt_index = emit_t_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("dungeon", &t_head);

	/* Set the global variables */
	t_info = (town_type*)t_head.info_ptr;
	t_name = t_head.name_ptr;
	t_text = t_head.text_ptr;

	return (err);
}


/*
 * Initialize the "u_info" array
 */
static errr init_u_info(void)
{
	errr err;

	/* Init the header */
	init_header(&u_head, z_info->u_max, sizeof(store_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	u_head.parse_info_txt = parse_u_info;
	
#ifdef ALLOW_TEMPLATES_OUTPUT

	/* Save a pointer to the emission function*/
	u_head.emit_info_txt_index = emit_u_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */

#endif /* ALLOW_TEMPLATES */

	err = init_info("store", &u_head);

	/* Set the global variables */
	u_info = (store_type*)u_head.info_ptr;
	u_name = u_head.name_ptr;
	u_text = u_head.text_ptr;

	return (err);
}


/*
 * Initialize the "b_info" array
 */
static errr init_b_info(void)
{
	errr err;

	/* Init the header */
	init_header(&b_head, (u16b)(MAX_STORES * z_info->b_max), sizeof(owner_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	b_head.parse_info_txt = parse_b_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("shop_own", &b_head);

	/* Set the global variables */
	b_info = (owner_type*)b_head.info_ptr;
	b_name = b_head.name_ptr;
	b_text = b_head.text_ptr;

	return (err);
}



/*
 * Initialize the "g_info" array
 */
static errr init_g_info(void)
{
	errr err;

	/* Init the header */
	init_header(&g_head, (u16b)(z_info->g_max * z_info->g_max), sizeof(byte));


#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	g_head.parse_info_txt = parse_g_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("cost_adj", &g_head);

	/* Set the global variables */
	g_info = (byte_hack*)g_head.info_ptr;
	g_name = g_head.name_ptr;
	g_text = g_head.text_ptr;

	return (err);
}


/*
 * Initialize the "q_info" array
 */
static errr init_q_info(void)
{
	errr err;

	/* Init the header */
	init_header(&q_head, z_info->q_max, sizeof(quest_type));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	q_head.parse_info_txt = parse_q_info;

#endif /* ALLOW_TEMPLATES */

	err = init_info("quest", &q_head);

	/* Set the global variables */
	q_info = (quest_type*)q_head.info_ptr;
	q_name = q_head.name_ptr;
	q_text = q_head.text_ptr;

	return (err);
}

/*** Initialize others ***/

errr init_slays(void)
{
	int err = 0;

	int i, j;

	/* No monster power to start with */
	tot_mon_power = 0;

	/* Compute total monster power */
	for (i = 0; i < z_info->r_max; i++)
	{
		/* Point at the "info" */
		monster_race *r_ptr = &r_info[i];

		/* Total power */
		tot_mon_power += r_ptr->power;
	}

	/* Allocate the "slay values" array - now used to compute magic item power */
	slays = C_ZNEW(SLAY_MAX, s32b);

	/* Clear values */
	for (i = 0; i < SLAY_MAX; i++) slays[i] = 0L;

	/* Precompute values for single flag slays for magic items */
	for (i = 0, j = 0x00000001L; (i < 32) && (j < 0x00200000L); i++, j <<=1)
	{
		/* Compute slay power for single flags */
		magic_slay_power[i] = slay_power(j);
	}

	return (err);
}


/*
 * Initialize some other arrays
 */
static errr init_other(void)
{
	int i, n;

	/*** Prepare the various "bizarre" arrays ***/

	/* Initialize the "macro" package */
	(void)macro_init();

	/* Initialize the "quark" package */
	(void)quarks_init();

	/* Initialize the "message" package */
	(void)messages_init();

	/*** Prepare grid arrays ***/

	/* Array of grids */
	view_g = C_ZNEW(VIEW_MAX, u16b);

	/* Array of grids */
	fire_g = C_ZNEW(VIEW_MAX, u16b);

	/* Array of grids */
	temp_g = C_ZNEW(TEMP_MAX, u16b);

	/* Hack -- use some memory twice */
	temp_y = ((byte*)(temp_g)) + 0;
	temp_x = ((byte*)(temp_g)) + TEMP_MAX;

	/* Array of grids */
	dyna_g = C_ZNEW(DYNA_MAX, u16b);

	/*** Set the default modify_grids ***/
	modify_grid_adjacent_hook = modify_grid_adjacent_view;
	modify_grid_boring_hook = modify_grid_boring_view;
	modify_grid_unseen_hook = modify_grid_unseen_view;
	modify_grid_interesting_hook = modify_grid_interesting_view;

	/*** Prepare dungeon arrays ***/

	/* Padded into array */
	cave_info = C_ZNEW(DUNGEON_HGT, byte_256);

	/* Padded into array */
	play_info = C_ZNEW(DUNGEON_HGT, byte_256);

	/* Region piece array */
	cave_region_piece = C_ZNEW(DUNGEON_HGT, s16b_wid);

	/* Feature array */
	cave_feat = C_ZNEW(DUNGEON_HGT, s16b_wid);

	/* Entity arrays */
	cave_o_idx = C_ZNEW(DUNGEON_HGT, s16b_wid);
	cave_m_idx = C_ZNEW(DUNGEON_HGT, s16b_wid);

#ifdef MONSTER_FLOW

	/* Flow arrays */
	cave_cost = C_ZNEW(DUNGEON_HGT, byte_wid);
	cave_when = C_ZNEW(DUNGEON_HGT, byte_wid);

#endif /* MONSTER_FLOW */

	/*** Prepare "vinfo" array ***/

	/* Used by "update_view()" */
	(void)vinfo_init();


	/*** Prepare entity arrays ***/

	/* Objects */
	o_list = C_ZNEW(z_info->o_max, object_type);

	/* Monsters */
	m_list = C_ZNEW(z_info->m_max, monster_type);

	/* Region pieces */
	region_piece_list = C_ZNEW(z_info->region_piece_max, region_piece_type);

	/* Regions */
	region_list = C_ZNEW(z_info->region_max, region_type);


	/*** Prepare lore array ***/

	/* Lore */
	l_list = C_ZNEW(z_info->r_max, monster_lore);

	/* Lore */
	a_list = C_ZNEW(z_info->a_max, object_info);

	/* Lore */
	e_list = C_ZNEW(z_info->e_max, object_lore);

	/* Lore */
	x_list = C_ZNEW(z_info->x_max, object_info);

	/*** Prepare quest array ***/

	/* Quests */
	q_list = C_ZNEW(MAX_Q_IDX, quest_type);


	/*** Prepare the inventory ***/

	/* Allocate it */
	inventory = C_ZNEW(INVEN_TOTAL + 1, object_type);


	/*** Prepare the bags ***/

	/* Initialize empty bags */
	for (i = 0; i < SV_BAG_MAX_BAGS; i++)
		for (n = 0; n < INVEN_BAG_TOTAL; n++)
			bag_contents[i][n] = 0;

	/* Initialize bag kind cache */
	for (i = 0; i < SV_BAG_MAX_BAGS; i++)
		for (n = 0; n < INVEN_BAG_TOTAL; n++)
			bag_kinds_cache[i][n] = lookup_kind(bag_holds[i][n][0], bag_holds[i][n][1]);

	/*** Prepare the dungeons ***/

	/* Maximum number of stores */
	max_store_count = 0;

	/* Initialize maximum depth and count stores */
	for (i = 0; i < z_info->t_max; i++)
	{
		t_info[i].attained_depth = min_depth(i);

		for (n = 0; n < MAX_STORES; n++)
		{
			if (t_info[i].store[n]) max_store_count++;
		}
	}


	/*** Prepare the stores ***/

	/* Set zero stores initially. Stores get initialized at load time or when first entering a store. */
	total_store_count = 0;

	/*** Allocate space for the maximum number of stores */
	store = C_ZNEW(max_store_count, store_type_ptr);

	/*** Prepare the options ***/

	option_set_defaults();

	/* Initialize the window flags */
	for (n = 0; n < ANGBAND_TERM_MAX; n++)
	{
		/* Assume no flags */
		op_ptr->window_flag[n] = 0L;
	}


	/*** Pre-allocate space for the "format()" buffer ***/

	/* Hack -- Just call the "format()" function */
	(void)format("%s (%s).", "the Gold Dragon", MAINTAINER);

	/* Success */
	return (0);
}



/*
 * Initialize some other arrays
 */
static errr init_alloc(void)
{
	int i, j;

	object_kind *k_ptr;

	feature_type *f_ptr;

	monster_race *r_ptr;

	ego_item_type *e_ptr;

	alloc_entry *table;

	s16b num[MAX_DEPTH];

	s16b aux[MAX_DEPTH];


	/*** Analyze object allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(num, MAX_DEPTH, s16b);

	/* Size of "alloc_kind_table" */
	alloc_kind_size = 0;

	/* Scan the objects */
	for (i = 1; i < z_info->k_max; i++)
	{
		k_ptr = &k_info[i];

		/* Scan allocation pairs */
		for (j = 0; j < 4; j++)
		{
			/* Count the "legal" entries */
			if (k_ptr->chance[j])
			{
				/* Count the entries */
				alloc_kind_size++;

				/* Group by level */
				num[k_ptr->locale[j]]++;
			}
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town objects!");


	/*** Initialize object allocation info ***/

	/* Allocate the alloc_kind_table */
	alloc_kind_table = C_ZNEW(alloc_kind_size, alloc_entry);

	/* Get the table entry */
	table = alloc_kind_table;

	/* Scan the objects */
	for (i = 1; i < z_info->k_max; i++)
	{
		k_ptr = &k_info[i];

		/* Scan allocation pairs */
		for (j = 0; j < 4; j++)
		{
			/* Count the "legal" entries */
			if (k_ptr->chance[j])
			{
				int p, x, y, z;

				/* Extract the base level */
				x = k_ptr->locale[j];

				/* Extract the base probability */
				p = (100 / k_ptr->chance[j]);

				/* Skip entries preceding our locale */
				y = (x > 0) ? num[x-1] : 0;

				/* Skip previous entries at this locale */
				z = y + aux[x];

				/* Load the entry */
				table[z].index = i;
				table[z].level = x;
				table[z].prob1 = p;
				table[z].prob2 = p;
				table[z].prob3 = p;

				/* Another entry complete for this locale */
				aux[x]++;
			}
		}
	}


	/*** Analyze feature allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(&aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(&num, MAX_DEPTH, s16b);

	/* Size of "alloc_feat_table" */
	alloc_feat_size = 0;

	/* Scan the features */
	for (i = 1; i < z_info->f_max; i++)
	{
		/* Get the i'th race */
		f_ptr = &f_info[i];

		/* Legal features */
		if (f_ptr->rarity)
		{
			/* Count the entries */
			alloc_feat_size++;

			/* Group by level */
			num[f_ptr->level]++;
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia - not really necessary */
	if (!num[0]) quit("No town features!");

	/*** Initialize feature allocation info ***/

	/* Allocate the alloc_feat_table */
	alloc_feat_table = C_ZNEW(alloc_feat_size, alloc_entry);

	/* Get the table entry */
	table = alloc_feat_table;

	/* Scan the features */
	for (i = 1; i < z_info->f_max; i++)
	{
		/* Get the i'th feature */
		f_ptr = &f_info[i];

		/* Count valid pairs */
		if (f_ptr->rarity)
		{
			int p, x, y, z;

			/* Extract the base level */
			x = f_ptr->level;

			/* Extract the base probability */
			p = (100 / f_ptr->rarity);

			/* Skip entries preceding our locale */
			y = (x > 0) ? num[x-1] : 0;

			/* Skip previous entries at this locale */
			z = y + aux[x];

			/* Load the entry */
			table[z].index = i;
			table[z].level = x;
			table[z].prob1 = p;
			table[z].prob2 = p;
			table[z].prob3 = p;

			/* Another entry complete for this locale */
			aux[x]++;
		}
	}

	/*** Analyze monster allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(num, MAX_DEPTH, s16b);

	/* Size of "alloc_race_table" */
	alloc_race_size = 0;

	/* Scan the monsters (not the ghost) */
	for (i = 1; i < z_info->r_max - 1; i++)
	{
		/* Get the i'th race */
		r_ptr = &r_info[i];

		/* Legal monsters */
		if (r_ptr->rarity)
		{
			/* Count the entries */
			alloc_race_size++;

			/* Group by level */
			num[r_ptr->level]++;
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town monsters!");


	/*** Initialize monster allocation info ***/

	/* Allocate the alloc_race_table */
	alloc_race_table = C_ZNEW(alloc_race_size, alloc_entry);

	/* Get the table entry */
	table = alloc_race_table;

	/* Scan the monsters (not the ghost) */
	for (i = 1; i < z_info->r_max - 1; i++)
	{
		/* Get the i'th race */
		r_ptr = &r_info[i];

		/* Count valid pairs */
		if (r_ptr->rarity)
		{
			int p, x, y, z;

			/* Extract the base level */
			x = r_ptr->level;

			/* Extract the base probability */
			p = (100 / r_ptr->rarity);

			/* Skip entries preceding our locale */
			y = (x > 0) ? num[x-1] : 0;

			/* Skip previous entries at this locale */
			z = y + aux[x];

			/* Load the entry */
			table[z].index = i;
			table[z].level = x;
			table[z].prob1 = p;
			table[z].prob2 = p;
			table[z].prob3 = p;

			/* Another entry complete for this locale */
			aux[x]++;
		}
	}

	/*** Analyze ego_item allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(num, MAX_DEPTH, s16b);

	/* Size of "alloc_ego_table" */
	alloc_ego_size = 0;

	/* Scan the ego items */
	for (i = 1; i < z_info->e_max; i++)
	{
		/* Get the i'th ego item */
		e_ptr = &e_info[i];

		/* Legal items */
		if (e_ptr->rarity)
		{
			/* Count the entries */
			alloc_ego_size++;

			/* Group by level */
			num[e_ptr->level]++;
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/*** Initialize ego-item allocation info ***/

	/* Allocate the alloc_ego_table */
	alloc_ego_table = C_ZNEW(alloc_ego_size, alloc_entry);

	/* Get the table entry */
	table = alloc_ego_table;

	/* Scan the ego-items */
	for (i = 1; i < z_info->e_max; i++)
	{
		/* Get the i'th ego item */
		e_ptr = &e_info[i];

		/* Count valid pairs */
		if (e_ptr->rarity)
		{
			int p, x, y, z;

			/* Extract the base level */
			x = e_ptr->level;

			/* Extract the base probability */
			p = (100 / e_ptr->rarity);

			/* Skip entries preceding our locale */
			y = (x > 0) ? num[x-1] : 0;

			/* Skip previous entries at this locale */
			z = y + aux[x];

			/* Load the entry */
			table[z].index = i;
			table[z].level = x;
			table[z].prob1 = p;
			table[z].prob2 = p;
			table[z].prob3 = p;

			/* Another entry complete for this locale */
			aux[x]++;
		}
	}

	/* Success */
	return (0);
}


/*
 * Hack -- take notes on line 23
 */
static void note(cptr str)
{
	Term_erase(0, 23, 255);
	Term_putstr(20, 23, -1, TERM_WHITE, str);
	Term_fresh();
}



/*
 * Hack -- Explain a broken "lib" folder and quit (see below).
 */
static void init_angband_aux(cptr why)
{
	quit_fmt("%s\n\n%s", why,
	 "The 'lib' directory is probably missing or broken.\n"
	 "Perhaps the archive was not extracted correctly.\n"
	 "See the 'readme.txt' file for more information.");
}


/*
 * Hack -- main Angband initialization entry point
 *
 * Verify some files, display the "news.txt" file, create
 * the high score file, initialize all internal arrays, and
 * load the basic "user pref files".
 *
 * Be very careful to keep track of the order in which things
 * are initialized, in particular, the only thing *known* to
 * be available when this function is called is the "z-term.c"
 * package, and that may not be fully initialized until the
 * end of this function, when the default "user pref files"
 * are loaded and "Term_xtra(TERM_XTRA_REACT,0)" is called.
 *
 * Note that this function attempts to verify the "news" file,
 * and the game aborts (cleanly) on failure, since without the
 * "news" file, it is likely that the "lib" folder has not been
 * correctly located.  Otherwise, the news file is displayed for
 * the user.
 *
 * Note that this function attempts to verify (or create) the
 * "high score" file, and the game aborts (cleanly) on failure,
 * since one of the most common "extraction" failures involves
 * failing to extract all sub-directories (even empty ones), such
 * as by failing to use the "-d" option of "pkunzip", or failing
 * to use the "save empty directories" option with "Compact Pro".
 * This error will often be caught by the "high score" creation
 * code below, since the "lib/apex" directory, being empty in the
 * standard distributions, is most likely to be "lost", making it
 * impossible to create the high score file.
 *
 * Note that various things are initialized by this function,
 * including everything that was once done by "init_some_arrays".
 *
 * This initialization involves the parsing of special files
 * in the "lib/data" and sometimes the "lib/edit" directories.
 *
 * Note that the "template" files are initialized first, since they
 * often contain errors.  This means that macros and message recall
 * and things like that are not available until after they are done.
 *
 * We load the default "user pref files" here in case any "color"
 * changes are needed before character creation.
 *
 * Note that the "graf-xxx.prf" file must be loaded separately,
 * if needed, in the first (?) pass through "TERM_XTRA_REACT".
 */
void init_angband(void)
{
	int fd;

	int mode = 0644;

	FILE *fp;

	char buf[1024];


	/*** Verify the "news" file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_FILE, "news.txt");

	/* Attempt to open the file */
	fd = fd_open(buf, O_RDONLY);

	/* Failure */
	if (fd < 0)
	{
		char why[1024];

		/* Message */
		sprintf(why, "Cannot access the '%s' file!", buf);

		/* Crash and burn */
		init_angband_aux(why);
	}

	/* Close it */
	fd_close(fd);


	/*** Display the "news" file ***/

	/* Clear screen */
	Term_clear();

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_FILE, "news.txt");

	/* Open the News file */
	fp = my_fopen(buf, "r");

	/* Dump */
	if (fp)
	{
		int i = 0;

		/* Dump the file to the screen */
		while (0 == my_fgets(fp, buf, sizeof(buf)))
		{
			/* Display and advance */
			Term_putstr(0, i++, -1, TERM_WHITE, buf);
		}

		/* Close */
		my_fclose(fp);
	}

	/* Flush it */
	Term_fresh();


	/*** Verify (or create) the "high score" file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_APEX, "scores.raw");

	/* Attempt to open the high score file */
	fd = fd_open(buf, O_RDONLY);

	/* Failure */
	if (fd < 0)
	{
		/* File type is "DATA" */
		FILE_TYPE(FILE_TYPE_DATA);

		/* Grab permissions */
		safe_setuid_grab();

		/* Create a new high score file */
		fd = fd_make(buf, mode);

		/* Drop permissions */
		safe_setuid_drop();

		/* Failure */
		if (fd < 0)
		{
			char why[1024];

			/* Message */
			sprintf(why, "Cannot create the '%s' file!", buf);

			/* Crash and burn */
			init_angband_aux(why);
		}
	}

	/* Close it */
	fd_close(fd);


	/*** Initialize some arrays ***/

	/* Initialize size info */
	note("[Initializing array sizes...]");
	if (init_z_info()) quit("Cannot initialize sizes");

	/* Initialize effect info */
	note("[Initializing arrays... (effects)]");
	if (init_effect_info()) quit("Cannot initialize effects");

	/* Initialize blow info */
	note("[Initializing arrays... (blows)]");
	if (init_method_info()) quit("Cannot initialize blows");

	/* Initialize region info */
	note("[Initializing arrays... (regions)]");
	if (init_region_info()) quit("Cannot initialize regions");

	/* Initialize feature info */
	note("[Initializing arrays... (features)]");
	if (init_f_info()) quit("Cannot initialize features");

	/* Initialize room description info */
	note("[Initializing arrays... (room descriptions)]");
	if (init_d_info()) quit("Cannot initialize room descriptions");

	/* Initialize monster info */
	note("[Initializing arrays... (monsters)]");
	if (init_r_info()) quit("Cannot initialize monsters");

	/* Initialize slay info */
	note("[Initializing arrays... (slays)]");
	if (init_slays()) quit("Cannot initialize slays");

	/* Initialize object info */
	note("[Initializing arrays... (objects)]");
	if (init_k_info()) quit("Cannot initialize objects");

	/* Initialize artifact info */
	note("[Initializing arrays... (artifacts)]");
	if (init_a_info()) quit("Cannot initialize artifacts");

	/* Initialize name info */
	note("[Initializing arrays... (names)]");
	if (init_n_info()) quit("Cannot initialize names");

	/* Initialize ego-item info */
	note("[Initializing arrays... (ego-items)]");
	if (init_e_info()) quit("Cannot initialize ego-items");

	/* Initialize flavor info */
	note("[Initializing arrays... (flavors)]");
	if (init_x_info()) quit("Cannot initialize flavors");

	/* Initialize feature info */
	note("[Initializing arrays... (vaults)]");
	if (init_v_info()) quit("Cannot initialize vaults");

	/* Initialize history info */
	note("[Initializing arrays... (histories)]");
	if (init_h_info()) quit("Cannot initialize histories");

	/* Initialize class info */
	note("[Initializing arrays... (classes)]");
	if (init_c_info()) quit("Cannot initialize classes");

	/* Initialize race info */
	note("[Initializing arrays... (races)]");
	if (init_p_info()) quit("Cannot initialize races");

	/* Initialize weapon style info */
	note("[Initializing arrays... (weapon styles)]");
	if (init_w_info()) quit("Cannot initialize weapon styles");

	/* Initialize spell info */
	note("[Initializing arrays... (spells)]");
	if (init_s_info()) quit("Cannot initialize spells");

	/* Initialize rune info */
	note("[Initializing arrays... (runes)]");
	if (init_y_info()) quit("Cannot initialize runes");

	/* Initialize town info */
	note("[Initializing arrays... (dungeons)]");
	if (init_t_info()) quit("Cannot initialize dungeons");

	/* Initialize store info */
	note("[Initializing arrays... (stores)]");
	if (init_u_info()) quit("Cannot initialize stores");

	/* Initialize owner info */
	note("[Initializing arrays... (owners)]");
	if (init_b_info()) quit("Cannot initialize owners");

	/* Initialize price info */
	note("[Initializing arrays... (prices)]");
	if (init_g_info()) quit("Cannot initialize prices");

	/* Initialize quest info */
	note("[Initializing arrays... (quests)]");
	if (init_q_info()) quit("Cannot initialize quests");

	/* Initialize some other arrays */
	note("[Initializing arrays... (other)]");
	if (init_other()) quit("Cannot initialize other stuff");

	/* Initialize some other arrays */
	note("[Initializing arrays... (alloc)]");
	if (init_alloc()) quit("Cannot initialize alloc stuff");

	/*** Load default user pref files ***/

	/* Initialize feature info */
	note("[Loading basic user pref file...]");

	/* Process that file */
	(void)process_pref_file("pref.prf");

	/* Done */
	note("    [Initialization complete]");
}

void ang_atexit(void (*arg)(void) ){

	typedef struct exitlist exitlist;
	struct exitlist {
		void (*func)(void) ;
		exitlist *next;
	};
	static exitlist *list;
	exitlist *next;

	if(arg != 0) {
		next = C_ZNEW(1, exitlist);
		next->func = arg;
		next->next = list;
		list = next;
		return;
	}

	while (list) {
		next = list->next;
		list->func();
		FREE(list);
		list = next;
	}
}

void cleanup_angband(void)
{
	int i, j;

	ang_atexit(0);

	/* Free the macros */
	for (i = 0; i < macro__num; ++i)
	{
		string_free(macro__pat[i]);
		string_free(macro__act[i]);
	}

	FREE(macro__pat);
	FREE(macro__act);

	/* Free the keymaps */
	for (i = 0; i < KEYMAP_MODES; ++i)
	{
		for (j = 0; j < (int)N_ELEMENTS(keymap_act[i]); ++j)
		{
			string_free(keymap_act[i][j]);
		}
	}

	/* Free the allocation tables */
	FREE(alloc_ego_table);
	FREE(alloc_race_table);
	FREE(alloc_kind_table);

	if (store)
	{
		/* Free the store inventories */
		for (i = 0; i < total_store_count; i++)
		{
			/* Get the store */
			store_type *st_ptr = store[i];

			/* Free the store inventory */
			FREE(st_ptr->stock);

			/* Free the store */
			FREE(st_ptr);
		}
	}


	/* Free the player inventory */
	FREE(inventory);

	/* Free the quest list */
	FREE(q_list);

	/* Free the lore, monster, and object lists */
	FREE(l_list);
	FREE(m_list);
	FREE(o_list);
	FREE(region_piece_list);
	FREE(region_list);

#ifdef MONSTER_FLOW

	/* Flow arrays */
	FREE(cave_when);
	FREE(cave_cost);

#endif /* MONSTER_FLOW */

	/* Free the cave */
	FREE(cave_o_idx);
	FREE(cave_m_idx);
	FREE(cave_region_piece);

	FREE(cave_feat);
	FREE(cave_info);
	FREE(play_info);

	/* Free the "update_view()" array */
	FREE(view_g);

	/* Free the "update_view()" array */
	FREE(fire_g);

	/* Free the temp array */
	FREE(temp_g);

	/* Free the messages */
	messages_free();

	/* Free the "quarks" */
	quarks_free();

	/* Free the info, name, and text arrays */
	free_info(&g_head);
	free_info(&b_head);
	free_info(&u_head);
	free_info(&t_head);
	free_info(&y_head);
	free_info(&s_head);
	free_info(&w_head);
	free_info(&c_head);
	free_info(&p_head);
	free_info(&h_head);
	free_info(&v_head);
	free_info(&r_head);
	free_info(&x_head);
	free_info(&e_head);
	free_info(&n_head);
	free_info(&a_head);
	free_info(&k_head);
	free_info(&f_head);
	free_info(&d_head);
	free_info(&q_head);
	free_info(&region_head);
	free_info(&method_head);
	free_info(&effect_head);
	free_info(&z_head);

	/* Free the format() buffer */
	vformat_kill();

	/* Free the directories */
	string_free(ANGBAND_DIR);
	string_free(ANGBAND_DIR_APEX);
	string_free(ANGBAND_DIR_BONE);
	string_free(ANGBAND_DIR_DATA);
	string_free(ANGBAND_DIR_EDIT);
	string_free(ANGBAND_DIR_FILE);
	string_free(ANGBAND_DIR_HELP);
	string_free(ANGBAND_DIR_INFO);
	string_free(ANGBAND_DIR_SAVE);
	string_free(ANGBAND_DIR_PREF);
	string_free(ANGBAND_DIR_USER);
	string_free(ANGBAND_DIR_XTRA);
}
