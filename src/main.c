#include "err.h"
#include "section.h"
#include "post.h"
#include "build.h"
#include "file.h"
#include "util.h"
#include "conf.h"
#include "strs.h"
#include "rss.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

h_err* build(char* path)
{
	h_err* err;

	if (h_util_file_err(path))
		return h_err_from_errno(errno, path);


	char* inpath = h_util_path_join(path, H_FILE_INPUT);
	char* outpath = h_util_path_join(path, H_FILE_OUTPUT);
	if (inpath == NULL || outpath == NULL)
		return h_err_create(H_ERR_ALLOC, NULL);

	h_section* root = malloc(sizeof(h_section));
	err = h_section_init_from_dir(root, inpath);
	if (err)
		return err;
	err = h_rss_configure(root, NULL);
	if (err)
		return err;

	if (mkdir(inpath, 0777) == -1 && errno != EEXIST)
		return h_err_from_errno(errno, inpath);

	if (mkdir(outpath, 0777) == -1 && errno != EEXIST)
		return h_err_from_errno(errno, outpath);

	//Get config
	char* conf_path = h_util_path_join(path, H_FILE_CONF);
	char* conf_str = h_util_file_read(conf_path);
	if (conf_str == NULL) return h_err_from_errno(errno, conf_path);
	h_conf* conf = calloc(1, sizeof(*conf));
  conf->use_guid = 1;
	if (conf == NULL) return h_err_from_errno(errno, conf_path);
	free(conf_path);
	err = h_conf_parse(conf_str, strlen(conf_str)+1, (void *)conf, h_conf_build);
	if (err)
		return err;
  // append root to the end of the url so links actually work
  if (conf->url != NULL && conf->root != NULL && conf->root[0] != '\0')
    conf->url = h_util_path_join(conf->url, conf->root);

	//index template
	char* index_path = h_util_path_join(path, H_FILE_THEME_HTML "/index.html");
	char* index_str = h_util_file_read(index_path);
	if (index_str == NULL) return h_err_from_errno(errno, index_path);
	free(index_path);

	//post template
	char* post_path = h_util_path_join(path, H_FILE_THEME_HTML "/post.html");
	char* post_str = h_util_file_read(post_path);
	if (post_str == NULL) return h_err_from_errno(errno, post_path);
	free(post_path);

	//page template
	char* page_path = h_util_path_join(path, H_FILE_THEME_HTML "/page.html");
	char* page_str = h_util_file_read(page_path);
	if (page_str == NULL) return h_err_from_errno(errno, page_path);
	free(page_path);

	//menu template
	char* menu_path = h_util_path_join(path, H_FILE_THEME_HTML "/menu.html");
	char* menu_str = h_util_file_read(menu_path);
	if (menu_str == NULL) return h_err_from_errno(errno, menu_path);
	free(menu_path);

	//menu_section template
	char* menu_section_path = h_util_path_join(path, H_FILE_THEME_HTML "/menu_section.html");
	char* menu_section_str = h_util_file_read(menu_section_path);
	if (menu_section_str == NULL) return h_err_from_errno(errno, menu_section_path);
	free(menu_section_path);

	//menu_logo template
	char* menu_logo_path = h_util_path_join(path, H_FILE_THEME_HTML "/menu_logo.html");
	char* menu_logo_str = h_util_file_read(menu_logo_path);
	if (menu_logo_str == NULL) return h_err_from_errno(errno, menu_logo_path);
	free(menu_logo_path);

	h_build_strs strs =
	{
		index_str,
		post_str,
		page_str,
		menu_str,
		menu_section_str,
		menu_logo_str
	};

	err = h_build(root, outpath, strs, conf);
	if (err)
		return err;


	// Deal with rss
	if (conf->rss)
	{
		err = h_rss_build(root, conf, outpath);
		if (err)
			return err;
	}

	//Deal with meta things

	char* metapath = h_util_path_join(path, H_FILE_OUTPUT "/" H_FILE_OUT_META);
	if (metapath == NULL)
		return h_err_create(H_ERR_ALLOC, NULL);
	if (mkdir(metapath, 0777) == -1 && errno != EEXIST)
		return h_err_from_errno(errno, metapath);
	free(metapath);

	h_build_outfiles outfiles;

	//public/_/script.js
	char* outjspath = h_util_path_join(
		outpath, H_FILE_OUT_META "/" H_FILE_OUT_JS
	);
	outfiles.js = fopen(outjspath, "w");
	if (outfiles.js == NULL)
		return h_err_from_errno(errno, outjspath);
	free(outjspath);

	//public/_/style.css
	char* outcsspath = h_util_path_join(
		outpath, H_FILE_OUT_META "/" H_FILE_OUT_CSS
	);
	outfiles.css = fopen(outcsspath, "w");
	if (outfiles.css == NULL)
		return h_err_from_errno(errno, outcsspath);
	free(outcsspath);

	//Prepare imgs
	err = h_build_imgs(path);
	if (err)
		return err;

	//Prepare basic script.js library
	fputs(H_STRS_JS_LIB, outfiles.js);

	//Prepare theme things
	err = h_build_theme(path, outfiles);
	if (err)
		return err;

	//Prepare plugins
	err = h_build_plugins(path, outfiles, conf);
	if (err)
		return err;

	fclose(outfiles.js);
	fclose(outfiles.css);
	free(inpath);
	free(outpath);
	free(conf);
	free(conf_str);
	free(index_str);
	free(post_str);
	free(menu_str);
	free(menu_section_str);

	return NULL;
}

int main(int argc, char** argv)
{
#define usage() fprintf(stderr, "Usage: %s <directory>\n", argv[0])

	if (argc != 2) {
		usage();
		return 1;
	}

	h_err* err = build(argv[1]);
	if (err)
		h_err_print(err);

	return 0;
}
