#include <dirent.h>
#include <search.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "rt.h"

// Interprets user input resistance
double r_get(char * in) {
	// Attempts to catch values such as:
	// 12.34, 12.34k, 12k34
	double f;	// 12(.34)
	char s;		// k
	double i;	// 34
	int ssf = sscanf(in, "%lf%c%lf", &f, &s, &i);
	if (ssf == 0) return -1;
	if (ssf == 1) return f;
	if (ssf == 3) {
		if (f != floor(f)) return -1; // 12.34k56 ??
		f += i / pow(10,(floor(log10(i))+1));
	}
	if (ssf >= 2) {
		switch (s) {
			case 'n': f*=1e-9; break;
			case 'u': f*=1e-6; break;
			case 'm': f*=1e-3; break;
			case 'R':          break;
			case 'k': f*=1e3; break;
			case 'M': f*=1e6; break;
			case 'G': f*=1e9; break;
			default: return -1;
		}
		return f;
	}
	return -1;

}

// Interprets user input error value
// Currently not in use
double e_get(char * in) {
	// Attempts to catch values such as:
	// 0.03, 0.1%, 1%
	double f;
	char s;
	int ssf = sscanf(in, "%lf%c", &f, &s);
	if (ssf == 0) return -1;
	if (ssf == 1) return f;
	if (ssf == 2) {
		if (s == '%') f /= 100;
		return f;
	}
	return -1;

}

// Handles reading the resistor value list
void get_list(char * name) {
	FILE * f_list;
	char fpath[100];

	snprintf(usd, 100, "%s%s", getenv("HOME"), "/.config/rt/");
	char * paths[] = {cwd, usd, gsd};
	int i;

	snprintf(fpath, 100, "%s", name);
	if (debug) printf("%s\n", &fpath);
	if (f_list = fopen(fpath, "r")) goto gl_parse;

	for (i=0;i<3;i++){
		snprintf(fpath, 100, "%s%s%s", paths[i], "lists/", name);
		if (debug) printf("%s\n", &fpath);
		if (f_list = fopen(fpath, "r")) goto gl_parse;
	}
	
	printf("The specified value list cannot be found.\n\n");

	printf("Available lists, in order of precedence:\n");
	struct dirent * dp;
	
	for (i=0;i<3;i++){
		snprintf(fpath, 100, "%s%s", paths[i], "lists/");
		DIR *d = opendir(fpath);
		if (d) {
			while ((dp = readdir(d)) != NULL) {
				if (dp->d_type == DT_REG) printf("%s%s\n", fpath, dp->d_name);
			}
			closedir(d);
		}
	}

	exit (-1);

gl_parse:
	int vcount = 0;
	int v_vals_size = 32;
	double * v_vals = malloc(v_vals_size * sizeof(double));	
	
	printf("Using the list of available values: %s\n", fpath);

	while (1) {
		if (fscanf(f_list, "%99s,", &fpath) == EOF) break;
		v_vals[vcount] = r_get(fpath);
		vcount++;
		if (vcount == v_vals_size-1) {
			v_vals_size *= 2;
			v_vals = realloc(v_vals, v_vals_size * sizeof(double));
			if (debug) printf("Realloc. v_vals_size=%d\n", v_vals_size);
		}
	}
	if (debug) printf("List file read complete. vcount=%d\n", vcount);
	qsort(v_vals, (vcount+1), sizeof(double), cmpf);

	// Remove entries <= 0
	int g0 = 0;
	while (v_vals[g0] <= 0) g0++;
	if (g0 > 0) {
		if (debug) printf("Removing %d entries <= 0.\n", g0);
		vcount -= g0;
		memmove(v_vals, v_vals + g0, (vcount) * sizeof(double));
		v_vals = realloc(v_vals, (vcount) * sizeof(double));
	}
	//TODO: Remove duplicates

	v_list.vals = v_vals;
	v_list.n = vcount;
	v_list.min = v_vals[0];
	v_list.max = v_vals[vcount-1];
	if (debug) printf("Created list: %d items, %g min, %g max.\n", vcount, v_vals[0], v_vals[vcount-1]);
	int j;
	if (debug) for (j=0;j<vcount;j++) printf("%g\t", v_vals[j]);
	if (debug) printf("\n");

}

// Prints a nicely formatted value of a single resistor from a double 
void rf_print(double in) {
	if (in >= 1e9) {
		printf("%gG", in/1e9);
		return;
	}
	if (in >= 1e6) {
		printf("%gM", in/1e6);
		return;
	}
	if (in >= 1e3) {
		printf("%gk", in/1e3);
		return;
	}
	if (in >= 1) {
		printf("%g", in);
		return;
	}
	if (in >= 1e-3) {
		printf("%gm", in*1e3);
		return;
	}
	if (in >= 1e-6) {
		printf("%gu", in*1e6);
		return;
	}
	printf("%gn", in*1e9);
	return;
}

void r_print(struct res * r, int i) {
	double in = r->i[i];
	rf_print(in);
	return;
}

// Search the v_list for the value "key".
// Supports multiple modes.
int vfind_i(double key, enum find_mode mode) {
	
	if (mode == MIN) return 0;
	if (key > v_list.max) return v_list.n-1;
	if (key < v_list.min) return 0;
	//if (isinf(key)) return 0;

	// Search for GE
	// A simple binary search
	// This used to be a choke point, hence those weird optimizations
	unsigned short int i = v_list.n >> 1;
	unsigned short int step = i >> 1;
	while (step > 1) {
		if (v_list.vals[i] < key) i+= step;
		else i -= step;
		step = step >> 1;
	}
	while (v_list.vals[i] > key) i--;
	while (v_list.vals[i] < key) i++;

	bool lo = (i == 0);
	bool hi = (i >= (v_list.n - 1));
	bool eq = v_list.vals[i] == key;

	switch (mode) {
		case CLOSEST:
			if (lo || eq) return i;
			double ch = (v_list.vals[i]) / key;
			double cl = key / (v_list.vals[i-1]);
			return (ch > cl) ? i-1 : i;
			break;
		case GREATER:
			if (hi) return i;
			return (eq) ? i+1 : i;
			break;
		case LOWER:
			if (lo) return i;
			return (eq) ? i-1 : i;
			break;
		case GE:
			return i;
			break;
		case LE:
			if (lo || eq) return i;
			return i-1;
			break;
	}
	printf("oopsie in vfind_i! key=%g, mode=%d\n", key, mode);
	return -1;
}
// The same thing but returns a numerical value
double vfind(double key, enum find_mode mode) {

	int ret = vfind_i(key, mode);
	return v_list.vals[ret];
}

// Parallel resistor connection
double prl(double r1, double r2) {
	return (r1*r2)/(r1+r2);
}

// Parallel triple resistor connection
double prl3(double r1, double r2, double r3) {
	return (r1*r2*r3)/((r1*r2)+(r2*r3)+(r3*r1));
}

// Evaluate resistor network struct
double eval_res(struct res * res) {
	switch(res->type) {
		case T_NONE:
			return -1;
			break;
		case T_SINGLE: 
			return res->i[0];
			break;
		case T_2S:
			return res->i[0] + res->i[1];
			break;
		case T_2P:
			return prl(res->i[0], res->i[1]);
			break;
		case T_1S2S:
			return res->i[0] + res->i[1] + res->i[2];
			break;
		case T_1S2P:
			return res->i[0] + prl(res->i[1], res->i[2]);
			break;
		case T_1P2S:
			return prl(res->i[0], res->i[1] + res->i[2]);
			break;
		case T_1P2P:
			return prl3(res->i[0], res->i[1], res->i[2]);
	}
	printf("oopsie in eval_res! res.type=%d\n", res->type);
	return -1;
}

bool isresmax(struct res * r) {
	switch(r->type) {
		case T_SINGLE: 
			return eval_res(r) >= v_list.max;
			break;
		case T_2S:
			return eval_res(r) >= v_list.max*2;
			break;
		case T_2P:
			return eval_res(r) >= v_list.max/2;
			break;
		case T_1S2S:
			return eval_res(r) >= v_list.max*3;
			break;
		case T_1S2P:
			return eval_res(r) >= v_list.max*1.5;
			break;
		case T_1P2S:
			return eval_res(r) >= v_list.max*2/3;
			break;
		case T_1P2P:
			return eval_res(r) >= v_list.max/3;
			break;
	}
	printf("oopsie in isresmax! res.type=%d\n", r->type);
	return -1;
}

// Print resistor network struct in a human readable form
// No newline at the end
void print_res(struct res * res) {
	switch(res->type) {
		case T_NONE:
			break;
		case T_SINGLE: 
			r_print(res, 0);
			break;
		case T_2S:
			r_print(res, 0);
			printf("+");
			r_print(res, 1);
			break;
		case T_2P:
			r_print(res, 0);
			printf("||");
			r_print(res, 1);
			break;
		case T_1S2S:
			r_print(res, 0);
			printf("+");
			r_print(res, 1);
			printf("+");
			r_print(res, 2);
			break;
		case T_1S2P:
			r_print(res, 0);
			printf("+(");
			r_print(res, 1);
			printf("||");
			r_print(res, 2);
			printf(")");
			break;
		case T_1P2S:
			r_print(res, 0);
			printf("||(");
			r_print(res, 1);
			printf("+");
			r_print(res, 2);
			printf(")");
			break;
		case T_1P2P:
			r_print(res, 0);
			printf("||");
			r_print(res, 1);
			printf("||");
			r_print(res, 2);
			break;
		default:
			printf("Oopsie in print_res! res_type=%d\n", res->type);
	}

}

// Finds a resistor network matching a specified value.
// find_res_t looks for a resistor network of a specific type.
// find_res aggregates calls of find_res_t of the same number of resistors in a network.
// The obtained value is returned.
// Parameters:
// - r: A pointer to a resistor structure, in which a result will be stored (accepts NULL)
// - value: The target value
// - rs: Complexity of a resistor network (1, 2 or 3) (find_res only)
// - rt: Resistor group type (find_res_t only)
// - mode: accepts CLOSEST, GREATER, LOWER, GE, LE
// - nopush: Do not store result in "r_list" results list

double find_res_t (struct res * r, double value, enum res_type rt, enum find_mode mode, bool nopush) {
	bool str = (r != NULL);
	int i;
	double r1, r2;
	struct res r23;
	struct res rb;
	struct res rx;
	double e;
	double eb = 1e6;
	double rv;		// Return value
	double rvb;		// Return value

	rb.type = T_NONE;
	rx.type = rt;

	if (rt == T_SINGLE) {
		r1 = vfind(value, mode);
		e = (r1/value)-1;
		rx.i[0] = r1;
		if (!nopush) r_list_push(&rx, e);
		rb = rx;
		rvb = r1;
	}
	if (rt == T_2S) {
		int imax = vfind_i(value, LE);
		for (i = vfind_i(value/2, GE); i <= imax; i++) {
			r1 = v_list.vals[i];
			r2 = vfind(value - r1, mode);
			e = r1+r2;
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r2;
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == T_2P) {
		int imax = vfind_i(value*2, LE);
		for (i = vfind_i(value, GE); i <= imax; i++) {
			r1 = v_list.vals[i];
			r2 = vfind(r1*value/(r1-value), mode);
			e = prl(r1, r2);
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r2;
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == TS_2X) {
		int imax = vfind_i(value*2, LE);
		for (i = vfind_i(value/2, GE); i <= imax; i++) {
			r1 = v_list.vals[i];
			if (r1 < value) { 	// Series
				r2 = vfind(value - r1, mode);
				rx.type = T_2S;
				e = r1+r2;
			} 
			if (r1 > value) {	// Parallel
				r2 = vfind(r1*value/(r1-value), mode);
				rx.type = T_2P;
				e = prl(r1, r2);
			}
			if (r1 == value) {
				r2 = v_list.max;
				rx.type = T_2P;
				e = prl(r1, r2);
			}
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r2;
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == T_1S2S) {
		int imax = vfind_i(value, LOWER);
		for (i = vfind_i(value/3, LE); i <= imax; i++) {
			r1 = v_list.vals[i];
			r2 = find_res_t(&r23, value - r1, T_2S, mode, 1);
			e = r1+r2;
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r23.i[0]; rx.i[2] = r23.i[1];
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == T_1S2P) {
		int imax = vfind_i(value, LOWER);
		for (i = 0; i <= imax; i++) {
			r1 = v_list.vals[i];
			r2 = find_res_t(&r23, value - r1, T_2P, mode, 1); 
			e = r1+r2;
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r23.i[0]; rx.i[2] = r23.i[1];
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == T_1P2S) {
		int imax = v_list.n - 1;
		for (i = vfind_i(value, GREATER); i <= imax; i++) {
			r1 = v_list.vals[i];
			r2 = find_res_t(&r23, (r1*value/(r1-value)), T_2S, mode, 1);
			e = prl(r1, r2);
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r23.i[0]; rx.i[2] = r23.i[1];
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == T_1P2P) {
		int imax = vfind_i(value*3, LE);
		for (i = 0; i <= imax; i++) {
			r1 = v_list.vals[i];
			r2 = find_res_t(&r23, (r1*value/(r1-value)), T_2P, mode, 1);
			e = prl(r1, r2);
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r23.i[0]; rx.i[2] = r23.i[1];
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (rt == TS_3X) {
		int imax = vfind_i(value*3, LE);
		for (i = vfind_i(value/3, GE); i <= imax; i++) {
			r1 = v_list.vals[i];
			if (r1 < value) { 	// Series		
				r2 = find_res_t(&r23, value - r1, TS_2X, mode, 1);
				rx.type = T_1S2S + (r23.type - 2);
				e = r1+r2;
			} 
			if (r1 > value) {	// Parallel
				r2 = find_res_t(&r23, (r1*value/(r1-value)), TS_2X, mode, 1);
				rx.type = T_1P2S + (r23.type - 2);
				e = prl(r1, r2);
			}
			if (r1 == value) { 
				r2 = find_res_t(&r23, v_list.min/3, T_2P, MIN, 1);
				rx.type = T_1S2P;
				e = prl(r1, r2);
			}
			rv = e;
			e = (e/value)-1;
			rx.i[0] = r1; rx.i[1] = r23.i[0]; rx.i[2] = r23.i[1];
			if (!nopush) r_list_push(&rx, e);
			if (fabs(e) < eb) {
				rb = rx;
				eb = fabs(e);
				rvb = rv;
			}
		}
	}
	if (str) *r = rb;
	return rvb;
}

double find_res (struct res * r, double value, int rs, enum find_mode mode, bool nopush) {
	bool str = (r != NULL);
	double e;
	double eb = 1e6;
	struct res rr, rb;
	int i;
	if (rs == 1) {
		find_res_t(&rb, value, T_SINGLE, mode, nopush);
	}
	if (rs == 2) {
		find_res_t(&rb, value, TS_2X, mode, nopush);
	}
	if (rs == 3) {
		find_res_t(&rb, value, TS_3X, mode, nopush);
	}
	if (str) *r = rb;
	return eval_res(&rb);
}


/*----------------------------------------------------------------------------*/
/*                        RESULTS LIST FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

void r_list_init(int nr) {

	nres = nr;
	r_list_length = 1024;
	r_list_items = 0;
	free(r_list);
	free(r_res_list);

	r_res_list = malloc(sizeof(struct res) * r_list_length * nres);
	r_list = malloc(sizeof(struct result) * r_list_length);
}

void r_list_push(struct res * rs, double e) {

	memcpy(&r_res_list[nres*r_list_items], rs, nres * sizeof(struct res));
	r_list[r_list_items].rs = &r_res_list[nres*r_list_items];
	r_list[r_list_items].e = e;

	r_list_items++;
	if (r_list_items > r_list_length-3) {
		r_list_length *= 2;
		r_res_list = realloc(r_res_list, sizeof(struct res) * (r_list_length+1) * nres);
		r_list = realloc(r_list, sizeof(struct result) * (r_list_length+1));
		int j;
		for (j=0; j<r_list_items;j++){
			r_list[j].rs = &r_res_list[nres*j];
		}
		if (debug) printf("Realloc. r_list_length=%d\n", r_list_length);
	}
	if (debug) {
		printf("r_list_push #%d, res: ", r_list_items-1);
		print_res(rs); printf("\t error: %.2g%%\n", e*100);
	}
}

void r_list_sort() {
	qsort(r_list, r_list_items, sizeof(struct result), cmpr);
}

void r_list_trim(int items) {
	// It is assumed that the list is sorted
	
	int i = 0;
	int j = 0;

	// Looking for the point where abs(e) is the smallest
	// More than one result may have the minimal error, we'll preserve all of them.

	while (r_list[i].e < 0 && i < r_list_items-1) i++;
	if (i>0) {
		if (fabs(r_list[i-1].e) < r_list[i].e) i--;
	}
	j = i + 1; j = fmin(j, r_list_items-1);
	while (r_list[j].e <= r_list[i].e * (1.01) && j < r_list_items-1) j++;
	j--;

	// At this point, i is an index of the lowest error result.
	// j is an index of the last lowest error result.

	// Let's sort results with minimal error by the resistance of the first item
	qsort(&r_list[i], j-i+1, sizeof(struct result), cmprr);

	// And now trim the undesired results
	i -= items;
	j += items;
	i = fmax(i, 0);
	j = fmin(j, r_list_items-1);
	r_list_items = j - i + 1;

	memmove(r_list, &r_list[i], r_list_items * sizeof(struct result));
}

void r_list_print(enum func f) {

	int i = 0;
	int j = 0;

	// Looking for the point where abs(e) is the smallest
	// More than one result may have the minimal error, we'll preserve all of them.
	// This time this is necessary to bolden the best results

	while (r_list[i].e < 0 && i < r_list_items-1) i++;
	if (i>0) {
		if (fabs(r_list[i-1].e) < r_list[i].e) i--;
	}
	j = i + 1; j = fmin(j, r_list_items);

	while (r_list[j].e <= r_list[i].e * (1.01)  && j < r_list_items-1) j++;
	j--;
	if (f == F_COMB) {
		int k;
		for (k=0; k<r_list_items; k++) {

			if (k == i) printf(bold);
			if (k > j) printf(normal);
			printf("%d: ",k+1);
			curx(5);
			print_res(r_list[k].rs);
			curx(5+resfw);
			printf("= ");
			rf_print(eval_res(r_list[k].rs));
			curx(5+resfw+10);
			printf("\tError: %.2g%%\n", r_list[k].e*100);
		}
	}
	if (f == F_RATIO) {
		int k, l;
		for (k=0; k<r_list_items; k++) {

			if (k == i) printf(bold);
			if (k > j) printf(normal);
			printf("%d: ",k+1);
			for (l=0; l<nres; l++) {
				curx(5+resfw*l);
				print_res(&(r_list[k].rs[l]));
				//if (l < nres-1) printf(" : ");
			}
			curx(5+resfw*l);
			printf("Error: %.2g%%\n", r_list[k].e*100);
		}
	}
}

// Finds the best match of n resistors with specified n weights
// n - a number of weights
// wr - an array of weights
// er - a number of extra resistors to try
// Returns the error of the best solution.
//
double find_weights(int n, double * wr, int er)
{
	// A local copy of weights
	double * r = malloc(n*sizeof(double));
	memcpy(r, wr, n*sizeof(double));

	double e;		// Current error
	struct res * trs = malloc(n*sizeof(struct res));	// resistor networks
	double ptr;
	int * tes = malloc(n*sizeof(int));			// A number of extra resistors per each network
	int * tesc = malloc(2*sizeof(int));			// multi resistor iteration counter
	int sc = 0;
	double be = 1e6;	// the best (lowest) error so far
	double minr, maxr;	// Minimum and maximum trs/r ratio
	int minp;		// The position of minr
	double lv = 0;		// The lowest value
	double hv = 0;		// The highest value
	int i = 0; // for loops

	tesc[0] = 0;
	tesc[1] = 0;

	do {
		lv = 0;
		hv = 0;
		int j = 0; // for other things

		// Resetting tes (number of extra resistors)
		for(i=0; i<n; i++) tes[i] = 1;
		
		// Populating tes according to the state of tesc
		for (i=0; i<er; i++) {
			tes[tesc[i]] += 1;
		}

		// Advancing tesc for the next iteration
		tesc[0]++;
		if (tesc[0] > (n-1)) {
			tesc[1]++;
			tesc[0] = tesc[1];
		}

		// Setting up the first guess with minimal resistances from the list 
		i = 0;
		while (i < n) {
			find_res(&trs[i], v_list.min/3, tes[i], MIN, 1);
			i++;
		}
		
		// Iterating through other possibilities
		// by incrementing resistors with the lowest
		// value / weight ratio,
		while (1) {
			// Find minimum and maximum trs/r
			// and the lowest/highest values
			i = 0;
			while (i < n) {
				double tx = eval_res(&trs[i]);
				if (i == 0) {
					lv = tx;
					hv = tx;
					minr = tx/r[i];
					maxr = tx/r[i];
					minp = 0;
				}
				else {
					if (tx < lv) lv = tx;
					if (tx > hv) hv = tx;
					if (tx/r[i] < minr) {
						minr = tx/r[i];
						minp = i;
					}
					if (tx/r[i] > maxr) maxr = tx/r[i];
				}
				i++;
			}
			
			e = (maxr/minr) - 1;

			// Store the error value if it's the best so far
			if (e < be) be = e;

			// Store this result
			r_list_push(trs, e);

			if (debug) {
				printf("find_weights: ");
				for (i=0;i<n;i++) {
					print_res(&trs[i]);
					printf("\t");
				}
				printf("\n");
			}

			// Exit loop if the maxed out resistor is promoted for incrementation
			if (isresmax(&trs[minp])) break;

			// Advance the proper resistor
			// Since the advancement step might be too small, 
			// we have to make sure that the next step 
			// isn't the same as the last one.
			// The second attempt should give the perfect result, if available.
			// In case error = 0, the first next step is +1%
			i = 1;
			ptr = eval_res(&trs[minp]);
			if (e < 1e-6) e = 4e-2;
			else e /= 4;
			while (ptr >= eval_res(&trs[minp])) {
				find_res(&trs[minp], eval_res(&trs[minp])*(1+(i*e)), tes[minp], GE, 1);
				i++;
			}
		}
	} 
	while (tes[n-1] < er+1);

	free(trs);
	free(tes);
	free(tesc);
	free(r);
	
	return be;
}


//----------------------------------------------------------------------------//
void usage()
{
	printf("Resistor Tool by Tomek Szczęsny 2024\n\n");
	printf("Usage:\n");
	printf("rt [options] [function] value0 value1 value2...\n");
	printf("\n");
	printf("options:\n");
	printf("\n");
	printf("\t-l [name]  Use a list of values from 'name'. (unspecified uses 'default')\n");
	printf("\t\t   Search precedence:\n");
	printf("\t\t     /Absolute/Path/name\n");
	printf("\t\t     ./lists/name\n");
	printf("\t\t     ~/.config/rt/lists/name\n");
	printf("\t\t     /var/lib/rt/lists/name\n");
	printf("\n");
	printf("function: (one function only)\n");
	printf("\n");
	printf("\t-c\t  Find a combination of resistors approximating a single given value. (default)\n");
	printf("\n");
	printf("\t-r\t  Look for a set of resistors satisfying a given ratio between them.\n");
	printf("\t\t  If only one value is given analyze for the ratio value:1 otherwise,\n");
	printf("\t\t  additional values are interpreted as weights of each resistor.\n");
	printf("\t\t  Error is the product of the maximum and minimum resistance/weight ratio, minus one.\n");
	printf("\n");
	printf("Examples:\n");
	printf("\t\t  Find the best approximation of 12.34k resistance:\n");
	printf("\t\t  rt 12.34k\n");
	printf("\n");
	printf("\t\t  Find the best approximation of 12.34k resistance in E24 series:\n");
	printf("\t\t  rt -l e24 12.34k\n");
	printf("\n");
	printf("\t\t  Find the best set of resistors to build a 5-bit DAC resistor ladder:\n");
	printf("\t\t  rt -r 1 2 4 8 16\n");
	exit(0);
}

int main(int argc, char **argv)
{
	// Default values, to be overwritten by the user provided options
	double e_d = 0;
	enum func func_d = F_COMB;
	char * lf = malloc(80);
	lf = "default";
	printf("\n");
	
	if (argc == 1) usage();

	int i = 0;
	while (i < (argc-1)) {
		i++;
		if (!strcmp(argv[i], "-l")) {
			i++;
			lf = argv[i];
			continue;
		}
		if (!strcmp(argv[i], "-d")) {
			debug = 1;
			continue;
		}
		 	
		// If no other option is detected, try parsing a function switch
		break;
	}

	get_list(lf);

	// function
	if (!strcmp(argv[i], "-c")) {func_d = F_COMB; i++; goto args;}
	if (!strcmp(argv[i], "-r")) {func_d = F_RATIO; i++; goto args;}

args:
	// the rest are the numerical values. To be fetched by function code.

	if (func_d == F_COMB) {
		printf(bold);
		printf("Input value: ");
		curx(5+resfw);
		printf("= ");
		rf_print(r_get(argv[i]));
		printf("\n\n");
		printf(normal);

		r_list_init(1);
		find_res (NULL, r_get(argv[i]), 1, CLOSEST, 0);
		printf(bold);
		printf("Single resistor:\n");
		printf(normal);
		r_list_print(func_d);

		printf(bold);
		printf("\nTwo resistors networks:\n");
		printf(normal);
		r_list_init(1);
		find_res (NULL, r_get(argv[i]), 2, CLOSEST, 0);
		r_list_sort(); r_list_trim(5);
		r_list_print(func_d);

		printf(bold);
		printf("\nThree resistors networks:\n");
		printf(normal);
		r_list_init(1);
		find_res (NULL, r_get(argv[i]), 3, CLOSEST, 0);
		r_list_sort(); r_list_trim(5);
		r_list_print(func_d);
	}
	if (func_d == F_RATIO) {
		int n = argc-i;
		if (n == 0) usage();
		if (n == 1) n++;
		double * ws = malloc(n*sizeof(double));
		int j = 0;
		bool trymore = 1;
		while (i < argc) {
			ws[j] = r_get(argv[i]);
			i++; j++;
		}
		if (j == 1) ws[j] = 1;

		// At this point, we have a list of "n" weights in "ws".

		printf(bold);
		printf("\nInput weights:\n");
		for (j=0; j<n; j++) {
			curx(5+resfw*j);
			prncf(ws[j],resfw);
			//if (l < nres-1) printf(" : ");
		}

		printf("\n\n");
		printf(normal);

		r_list_init(n);
		trymore = (find_weights(n, ws, 0) > 1e-6);
		r_list_sort(); r_list_trim(5);
		r_list_print(func_d);

		if (!trymore) return 0;
		printf(bold);
		printf("\nWith one additional resistor:\n");
		printf(normal);
		r_list_init(n);
		trymore = (find_weights(n, ws, 1) > 1e-6);
		r_list_sort(); r_list_trim(5);
		r_list_print(func_d);

		if (!trymore) return 0;
		printf(bold);
		printf("\nWith two additional resistors:\n");
		printf(normal);
		r_list_init(n);
		find_weights(n, ws, 2);
		r_list_sort(); r_list_trim(5);
		r_list_print(func_d);
		free(ws);
	}
	printf("\n");
	return 0;
}


