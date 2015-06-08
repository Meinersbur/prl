#ifndef PRL_H
#define PRL_H



#include "prl_scop.h"
#include "prl_mem.h"
#include "prl_perf.h"



//TODO: Remove __ppcg_*
static inline int __ppcg_floord(int n, unsigned int d)
{
	if (n < 0)
		return -(((unsigned int) (-n) + d - 1) / d);
	return (unsigned int) n / d;
}

static inline int __ppcg_min(int lhs, int rhs)
{
	if (lhs <= rhs)
		return lhs;
	return rhs;
}

static inline int __ppcg_max(int lhs, int rhs)
{
	if (lhs >= rhs)
		return lhs;
	return rhs;
}




#endif /* PRL_H */
