#include "../build.h"
#include "../util.h"
#include "../file.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

h_err* h_build_imgs(const char* rootdir)
{
	char* imgsdir = h_util_path_join(rootdir, H_FILE_IMGS);
	char* outimgsdir = h_util_path_join(
		rootdir,
		H_FILE_OUTPUT "/" H_FILE_OUT_META "/" H_FILE_OUT_IMGS
	);

	//Check status of rootdir/imgs, returning if it doesn't exist
	{
		int err = h_util_file_err(imgsdir);
		if (err == ENOENT)
			return NULL;
		if (err && err != EEXIST)
			return h_err_from_errno(err, imgsdir);
	}

	//Create rootdir/public/_/plugins if it doesn't exist
	if (mkdir(outimgsdir, 0777) == -1 && errno != EEXIST)
		return h_err_from_errno(errno, outimgsdir);

	//Make sure dirs are okay
	DIR* d1 = opendir(imgsdir);
	if (d1 == NULL)
		return h_err_from_errno(errno, imgsdir);
	closedir(d1);
	if (mkdir(outimgsdir, 0777) == -1 && errno != EEXIST)
		return h_err_from_errno(errno, outimgsdir);

	h_util_cp_dir(imgsdir, outimgsdir);
	free(imgsdir);
	free(outimgsdir);

	return NULL;
}
