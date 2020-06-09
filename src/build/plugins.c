#include "../build.h"
#include "../util.h"
#include "../file.h"
#include "../strs.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

static h_err* build_plugin(
		const char* dirpath,
		const char* outdir,
		h_build_outfiles outfiles,
		const h_conf* conf)
{
	char* confjsonpath = h_util_path_join(dirpath, H_FILE_PLUGIN_CONF);
	if (confjsonpath == NULL)
		return h_err_create(H_ERR_ALLOC, NULL);

	errno = 0;
	char* confjson = h_util_file_read(confjsonpath);
	if (confjson == NULL && errno)
	{
		h_err* err = h_err_from_errno(errno, confjsonpath);
		free(confjsonpath);
		return err;
	}
	free(confjsonpath);
	if (confjson == NULL)
		return h_err_create(H_ERR_ALLOC, NULL);

	int jsonlen = strlen(confjson);
	int rootlen = strlen(conf->root);
	char* jspath = h_util_path_join(dirpath, H_FILE_PLUGIN_JS);
	if (jspath == NULL)
	{
		free(confjson);
		return h_err_create(H_ERR_ALLOC, NULL);
	}
	char* phppath = h_util_path_join(dirpath, H_FILE_PLUGIN_PHP);
	if (phppath == NULL)
	{
		free(confjson);
		free(jspath);
		return h_err_create(H_ERR_ALLOC, NULL);
	}

	//Copy things to script.js
	if (h_util_file_err(jspath) != ENOENT)
	{
		char* starttemplate = H_STRS_JS_START;

		int len = sizeof(char) * (strlen(starttemplate) + jsonlen + rootlen);
		char* start = malloc(len);
		snprintf(start, len, starttemplate, confjson, conf->root);

		char* end = H_STRS_JS_END;

		h_util_cp_dir_to_file_se(jspath, outfiles.js, start, end);

		free(start);
	}

	//Copy PHP files
	if (h_util_file_err(phppath) != ENOENT)
	{
		const char* starttemplate = H_STRS_PHP_START;

		int len = sizeof(char) * (strlen(starttemplate) + jsonlen) + rootlen;
		char* start = malloc(len);
		snprintf(start, len, starttemplate, confjson, conf->root);

		//Make sure dirs are okay
		DIR* d1 = opendir(phppath);
		if (d1 == NULL)
			return h_err_from_errno(errno, phppath);
		closedir(d1);
		if (mkdir(outdir, 0777) == -1 && errno != EEXIST)
			return h_err_from_errno(errno, outdir);

		h_util_cp_dir_se(phppath, outdir, start, "");

		free(start);
	}

	free(confjson);
	free(phppath);
	free(jspath);

	return NULL;
}

h_err* h_build_plugins(const char* rootdir, h_build_outfiles outfiles, const h_conf* conf)
{
	char* pluginsdir = h_util_path_join(rootdir, H_FILE_PLUGINS);
	if (pluginsdir == NULL)
		return h_err_create(H_ERR_ALLOC, NULL);
	char* outpluginsdir = h_util_path_join(
		rootdir,
		H_FILE_OUTPUT "/" H_FILE_OUT_META "/" H_FILE_OUT_PHP
	);
	if (outpluginsdir == NULL)
	{
		free(pluginsdir);
		return h_err_create(H_ERR_ALLOC, NULL);
	}

	//Check status of rootdir/plugins, returning if it doesn't exist
	{
		int err = h_util_file_err(pluginsdir);
		if (err == ENOENT)
		{
			free(outpluginsdir);
			free(pluginsdir);
			return NULL;
		}
		if (err && err != EEXIST)
		{
			free(outpluginsdir);
			free(pluginsdir);
			return h_err_from_errno(err, pluginsdir);
		}
	}

	//Create rootdir/public/_/plugins if it doesn't exist
	if (mkdir(outpluginsdir, 0777) == -1 && errno != EEXIST)
	{
		free(outpluginsdir);
		free(pluginsdir);
		return h_err_from_errno(errno, outpluginsdir);
	}

	//Loop through plugins, building them
	struct dirent** namelist;
	int n = scandir(pluginsdir, &namelist, NULL, alphasort);
	int i;
	for (i = 0; i < n; ++i)
	{
		struct dirent* ent = namelist[i];
		if (ent->d_name[0] == '.')
		{
			free(ent);
			continue;
		}

		char* dirpath = h_util_path_join(pluginsdir, ent->d_name);
		if (dirpath == NULL)
		{
			free(pluginsdir);
			free(outpluginsdir);
			return h_err_create(H_ERR_ALLOC, NULL);
		}
		char* outdir = h_util_path_join(outpluginsdir, ent->d_name);
		if (outdir == NULL)
		{
			free(dirpath);
			free(pluginsdir);
			free(outpluginsdir);
			return h_err_create(H_ERR_ALLOC, NULL);
		}

		h_err* err;
		err = build_plugin(dirpath, outdir, outfiles, conf);
		if (err)
		{
			free(outdir);
			free(dirpath);
			free(pluginsdir);
			free(outpluginsdir);
			return err;
		}

		free(dirpath);
		free(outdir);
		free(ent);
	}
	free(pluginsdir);
	free(outpluginsdir);
	free(namelist);

	return NULL;
}
