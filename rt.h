// Value Series Structure
// Contains a pointer to value array and its size.
// "vals" array should end with 10 to simplify other things.
struct vss {
	double * vals;
	int n;
	double min;
	double max;
};

// The instance of value list
struct vss v_list;

// Types of connections between resistor grups
// Turns out there are 4 ways of connecting 3 resistors together, for example.
// Unfortunately stuff gets really complicated for 4 resistors and beyond
// So let's pretend we're good with just three.
//
enum res_type {	
		T_SINGLE = 1,
		T_2S = 2,
		T_2P = 3,
		T_1S2S = 4,
		T_1S2P = 5,
		T_1P2S = 6,
		T_1P2P = 7,
		// Those below are only used as search modes, never to be returned
		TS_2X = 130,
		TS_3X = 131
};

// Resistance Structure
// - Resistance structure type
// - An array of resistor values
// At this time this structure can hold all possible combinations up to 3 resistors.
struct res{
	enum res_type type;
	double i[3];
};

bool isresmax(struct res * r);
double eval_res(struct res * res);

// Modes of find_res
// May look for the closest match, 
// or the first greater or lower match than the target value.
// This is useful in constructing ratioed resistance algorithm
// than brute forcing through all combinations.
enum find_mode{
	CLOSEST = 0,
	GREATER = 1,
	LOWER,
	GE,
	LE,
	MIN
};
// Terminal output style
const char bold[] = {0x1b, 0x5b, 0x31, 0x6d, 0x00};
const char normal[] = {0x1b, 0x28, 0x42, 0x1b, 0x5b, 0x6d, 0x00};
// Terminal code for moving to a specified column
#define curx(x) printf("\033[%dG", (x))
//printing centered within specified width:
void prncf(double n, int fw) {
	char *txt = malloc(fw + 1);
	int len = snprintf(txt, fw + 1, "%g", n);
	int pad = (fw - len) / 2;
	printf("%*s%s%*s", pad, "", txt, pad, "");
	free(txt);
} 
void prncs(char * txt, int fw) {
	int pad = (fw - strlen(txt)) / 2;
	printf("%*s%s%*s", pad, "", txt, pad, "");
	free(txt);
} 
// Res field width
const int resfw = 20;

char * cwd = "./";
char * gsd = "/var/lib/rt/";
char usd[100];

enum func {F_COMB, F_RATIO, F_DIV};

bool debug = 0;

// The structure for keeping "results" (sortable afterwards)
// Each "result" may have one or more res structs associated with them.
// This is dependent on the elaboration parameters and kept in "nres" variable.
// For example, in standard mode there is ony one "res" struct in a result.
// In ratioed mode, there is as many "res" structs as user specified weighs.
// 
// "r_res_list" is a memory space for "res" structs, which are parts of each "result".
// "result" is a struct holding a pointer to the first "res" struct in "r_res_list", and numerical error of the result.
// "r_list", finally, is an array of results.
//
// A comparison function is provided for sorting "r_list" in ascending order.
// Result list management functions are declared here as well.

int r_list_length;
int r_list_items;
int nres;
struct res * r_res_list = NULL;
struct result{
	struct res * rs;
	double e;
};
struct result * r_list = NULL;
// Compare result structs by error
int cmpr(const void * v1, const void * v2) {
	const struct result * va = v1;
	const struct result * vb = v2;
	if (va->e == vb->e) return 0;
	else return (va->e > vb->e) ? 1 : -1;
}
// Compare result structs by resistance of the first item 
int cmprr(const void * v1, const void * v2) {
	const struct result * va = v1;
	const struct result * vb = v2;
	double r1 = eval_res(&(va->rs[0]));
	double r2 = eval_res(&(vb->rs[0]));
	if (r1 == r2) return 0;
	else return (r1 > r2) ? 1 : -1;
}
void r_list_init(int nres);
void r_list_push(struct res * rs, double e);
void r_list_sort();
void r_list_trim(int items);
void r_list_print(enum func f);


// Compare doubles
int cmpf(const void * f1, const void * f2) {
	const double * fa;
	const double * fb;
	fa = f1;
	fb = f2;
	if (fa == fb) return 0;
	else return (fa > fb) ? 1 : -1;
}

int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}


static double e6_vals[7]   = {1.0, 1.5, 2.2, \
			     3.3, 4.7, \
			     6.8, 10.0};
struct vss e6 = {e6_vals, 6};

static double e12_vals[13] = {1.0, 1.2, 1.5, 1.8, 2.2, \
			     2.7, 3.3, 3.9, 4.7, 5.6, \
			     6.8, 8.2, 10.0};
struct vss e12 = {e12_vals, 12};

static double e24_vals[25] = {1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, \
			     2.7, 3.0, 3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, \
			     6.8, 7.5, 8.2, 9.1, 10.0};
struct vss e24 = {e24_vals, 24};

static double e48_vals[49] = {1.00, 1.05, 1.10, 1.15, 1.21, 1.27, 1.33, 1.40, \
			     1.47, 1.54, 1.62, 1.69, 1.78, 1.87, 1.96, 2.05, \
			     2.15, 2.26, 2.37, 2.49, 2.61, 2.74, 2.87, 3.01, \
			     3.16, 3.32, 3.48, 3.65, 3.83, 4.02, 4.22, 4.42, \
			     4.64, 4.87, 5.11, 5.36, 5.62, 5.90, 6.19, 6.49, \
			     6.81, 7.15, 7.50, 7.87, 8.25, 8.66, 9.09, 9.53, 10.0};
struct vss e48 = {e48_vals, 48};

static double e96_vals[97] = {1.00, 1.02, 1.05, 1.07, 1.10, 1.13, 1.15, 1.18, \
			     1.21, 1.24, 1.27, 1.30, 1.33, 1.37, 1.40, 1.43, \
			     1.47, 1.50, 1.54, 1.58, 1.62, 1.65, 1.69, 1.74, \
			     1.78, 1.82, 1.87, 1.91, 1.96, 2.00, 2.05, 2.10, \
			     2.15, 2.21, 2.26, 2.32, 2.37, 2.43, 2.49, 2.55, \
			     2.61, 2.67, 2.74, 2.80, 2.87, 2.94, 3.01, 3.09, \
			     3.16, 3.24, 3.32, 3.40, 3.48, 3.57, 3.65, 3.74, \
			     3.83, 3.92, 4.02, 4.12, 4.22, 4.32, 4.42, 4.53, \
			     4.64, 4.75, 4.87, 4.99, 5.11, 5.23, 5.36, 5.49, \
			     5.62, 5.76, 5.90, 6.04, 6.19, 6.34, 6.49, 6.65, \
			     6.81, 6.98, 7.15, 7.32, 7.50, 7.68, 7.87, 8.06, \
			     8.25, 8.45, 8.66, 8.87, 9.09, 9.31, 9.53, 9.76, 10.0};
struct vss e96 = {e96_vals, 96};

static double e192_vals[193] = {1.00, 1.01, 1.02, 1.04, 1.05, 1.06, 1.07, 1.09, \
			       1.10, 1.11, 1.13, 1.14, 1.15, 1.17, 1.18, 1.20, \
			       1.21, 1.23, 1.24, 1.26, 1.27, 1.29, 1.30, 1.32, \
			       1.33, 1.35, 1.37, 1.38, 1.40, 1.42, 1.43, 1.45, \
			       1.47, 1.49, 1.50, 1.52, 1.54, 1.56, 1.58, 1.60, \
			       1.62, 1.64, 1.65, 1.67, 1.69, 1.72, 1.74, 1.76, \
			       1.78, 1.80, 1.82, 1.84, 1.87, 1.89, 1.91, 1.93, \
			       1.96, 1.98, 2.00, 2.03, 2.05, 2.08, 2.10, 2.13, \
			       2.15, 2.18, 2.21, 2.23, 2.26, 2.29, 2.32, 2.34, \
			       2.37, 2.40, 2.43, 2.46, 2.49, 2.52, 2.55, 2.58, \
			       2.61, 2.64, 2.67, 2.71, 2.74, 2.77, 2.80, 2.84, \
			       2.87, 2.91, 2.94, 2.98, 3.01, 3.05, 3.09, 3.12, \
			       3.16, 3.20, 3.24, 3.28, 3.32, 3.36, 3.40, 3.44, \
			       3.48, 3.52, 3.57, 3.61, 3.65, 3.70, 3.74, 3.79, \
			       3.83, 3.88, 3.92, 3.97, 4.02, 4.07, 4.12, 4.17, \
			       4.22, 4.27, 4.32, 4.37, 4.42, 4.48, 4.53, 4.59, \
			       4.64, 4.70, 4.75, 4.81, 4.87, 4.93, 4.99, 5.05, \
			       5.11, 5.17, 5.23, 5.30, 5.36, 5.42, 5.49, 5.56, \
			       5.62, 5.69, 5.76, 5.83, 5.90, 5.97, 6.04, 6.12, \
			       6.19, 6.26, 6.34, 6.42, 6.49, 6.57, 6.65, 6.73, \
			       6.81, 6.90, 6.98, 7.06, 7.15, 7.23, 7.32, 7.41, \
			       7.50, 7.59, 7.68, 7.77, 7.87, 7.96, 8.06, 8.16, \
			       8.25, 8.35, 8.45, 8.56, 8.66, 8.76, 8.87, 8.98, \
			       9.09, 9.20, 9.31, 9.42, 9.53, 9.65, 9.76, 9.88, 10.0};
struct vss e192 = {e192_vals, 192};

struct res res_round(double in, struct vss * series);
struct res res_ceil(double in, struct vss * series);
struct res res_floor(double in, struct vss * series);
void res_inc (struct res * r);
void res_dec (struct res * r);
double res_f(struct res * r);
void r_print(struct res * r, int i);
void res_print(struct res * r);
