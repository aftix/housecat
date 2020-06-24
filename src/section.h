#ifndef H_SECTION_H
#define H_SECTION_H

#include "err.h"
#include "post.h"

// Forward declarations
struct h_atom_section;

typedef struct h_section
{
	char* title;
	char* slug;

	h_post** posts;
	int numposts;

	h_post** drafts;
	int numdrafts;

	struct h_section** subs;
	int numsubs;

	char* path;
	int depth;

	char* rpath;
	struct h_atom_section* atom_metadata;
	char* atom;
} h_section;

//Initiate a section from directory
h_err* h_section_init_from_dir(h_section* section, char* path);

//Add a post
h_err* h_section_add_post(h_section* section, h_post* post);

//Add a draft post
h_err* h_section_add_draft(h_section* section, h_post* post);

//Add a sub section
h_err* h_section_add_sub(h_section* section, h_section* sub);

h_section* h_section_create();
void h_section_free(h_section* section);

#endif
