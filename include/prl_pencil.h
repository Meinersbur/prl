#ifndef PRL_PENCIL_H
#define PRL_PENCIL_H

#ifndef PENCIL_H
enum npr_mem_tags {
	PENCIL_NPR_READWRITABLE = 0,
	PENCIL_NPR_NOWRITE = 1,
	PENCIL_NPR_NOREAD = 2,
	PENCIL_NPR_NOACCESS = PENCIL_NPR_NOWRITE | PENCIL_NPR_NOREAD,
};

// Only use on global, allocate-once memory OR prl-allocated memory that is released using prl_free or prl_mem_free; otherwise, this leaks memory as the tag has to be stored somewhere and there is no direct way to free it (only calling prl_free)
void __pencil_npr_mem_tag(void *location, enum npr_mem_tags mode);
#endif

#endif /* PRL_PENCIL_H */
