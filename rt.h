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
// Terminal escape code for moving the cursor to a specified column
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
char usd[100];	// Filled during runtime

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

