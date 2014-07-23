#include <stdio.h>
//#include <ufo/ufo.h>
#include <math.h>
#include <ufo-metadata.h>
#include <ufo-ir-sart.h>

int main(int n_args, char *argv[])
{
    /* Method
    */
    UfoIrMethod *m = ufo_ir_sart_new ();
    ufo_method_setup (m, NULL, NULL);

    /* Metadata test
    */
    UfoMetaData *md = ufo_metadata_new ();
    ufo_metadata_set_string (md, "genre","Rock");
    ufo_metadata_set_scalar (md, "number", 91.223f);
    ufo_metadata_set_pointer (md, "pointer", md);

    ufo_metadata_set_scalar (md, "number", 21.2123f);

    g_print ("\nINFO: %s | %f | %p    Metadata: %p \n",
             ufo_metadata_string (md, "genre"),
             ufo_metadata_scalar (md, "number"),
             ufo_metadata_pointer (md, "pointer"),
             md);

    return 0;
}