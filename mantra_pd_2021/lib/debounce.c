/* debounce -- disclavier filter */

/*  Copyright 1999 Miller Puckette.
Permission is granted to use this software for any purpose provided you
keep this copyright notice intact.

THE AUTHOR AND HIS EMPLOYERS MAKE NO WARRANTY, EXPRESS OR IMPLIED,
IN CONNECTION WITH THIS SOFTWARE.

This file is downloadable from http://www.crca.ucsd.edu/~msp .
*/

#include "m_pd.h"
static t_class *debounce_class;
#define DIMENSION 128

typedef struct _debounce
{
    t_object x_obj;
    double x_bouncetime;
    double x_ontime[DIMENSION];
    double x_offtime[DIMENSION];    /* NOT ACTUALLY USED RIGHT NOW */
} t_debounce;

static void *debounce_new(t_float bouncetime)
{
    t_debounce *x = (t_debounce *)pd_new(debounce_class);
    double d = clock_getsystime();
    int i;
    outlet_new(&x->x_obj, gensym("list"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("list2"));
    x->x_bouncetime = (bouncetime > 0 ? bouncetime : 500);
    for (i = 0; i < DIMENSION; i++)
    	x->x_ontime[i] = x->x_offtime[i] = d;
    return (x);
}

static void debounce_print(t_debounce *x)
{
    int i;
    for (i = 0; i < DIMENSION; i++)
    {
    	post("on %d off %d",
    	    (int)(1000 * clock_gettimesince(x->x_ontime[i])),
    	    (int)(1000 * clock_gettimesince(x->x_offtime[i])));    
    }
}

static void debounce_list(t_debounce *x, t_symbol *s, int argc, t_atom *argv)
{
    float pit = atom_getfloatarg(0, argc, argv);
    float vel = atom_getfloatarg(1, argc, argv);
    int ipit = (pit < 0 ? 0 : (pit >= DIMENSION - 1 ? DIMENSION-1 : pit));
    double timesince = (vel > 0 ? clock_gettimesince(x->x_ontime[ipit]) :
    	clock_gettimesince(x->x_offtime[ipit]));
    if (!vel || (timesince > x->x_bouncetime))
    	outlet_list(x->x_obj.ob_outlet, s, argc, argv);
}

static void debounce_list2(t_debounce *x, t_symbol *s, int argc, t_atom *argv)
{
    float pit = atom_getfloatarg(0, argc, argv);
    float vel = atom_getfloatarg(1, argc, argv);
    int ipit = (pit < 0 ? 0 : (pit >= DIMENSION - 1 ? DIMENSION-1 : pit));
    if (vel > 0)
    	x->x_ontime[ipit] = clock_getsystime();
    else
    	x->x_offtime[ipit] = clock_getsystime();
}

void debounce_setup(void)
{
    debounce_class = class_new(gensym("debounce"), (t_newmethod)debounce_new,
        0, sizeof(t_debounce), 0, A_DEFFLOAT, 0);
    class_addmethod(debounce_class, (t_method)debounce_list2,
    	gensym("list2"), A_GIMME, 0);
    class_addmethod(debounce_class, (t_method)debounce_print,
    	gensym("print"), A_NULL);
    class_addlist(debounce_class, debounce_list);
}

