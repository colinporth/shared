/*{{{  includes*/
#include "common.h"
#include "structs.h"

#include <stdlib.h>
#include <string.h>
#include "syntax.h"
#include "drc.h"
/*}}}*/

/*{{{*/
drc_info* drc_init (real_t cut, real_t boost) {

  drc_info *drc = (drc_info*)faad_malloc(sizeof(drc_info));
  memset(drc, 0, sizeof(drc_info));

  drc->ctrl1 = cut;
  drc->ctrl2 = boost;

  drc->num_bands = 1;
  drc->band_top[0] = 1024/4 - 1;
  drc->dyn_rng_sgn[0] = 1;
  drc->dyn_rng_ctl[0] = 0;

  return drc;
  }
/*}}}*/
/*{{{*/
void drc_end (drc_info* drc) {

  faad_free(drc);
  }
/*}}}*/

/*{{{*/
void drc_decode (drc_info* drc, real_t* spec) {

  uint16_t i, bd, top;
  real_t factor, exp;
  uint16_t bottom = 0;

  if (drc->num_bands == 1)
    drc->band_top[0] = 1024/4 - 1;

  for (bd = 0; bd < drc->num_bands; bd++) {
    top = 4 * (drc->band_top[bd] + 1);

    /* Decode DRC gain factor */
    if (drc->dyn_rng_sgn[bd])  /* compress */
      exp = -drc->ctrl1 * (drc->dyn_rng_ctl[bd] - (DRC_REF_LEVEL - drc->prog_ref_level))/REAL_CONST(24.0);
    else /* boost */
      exp = drc->ctrl2 * (drc->dyn_rng_ctl[bd] - (DRC_REF_LEVEL - drc->prog_ref_level))/REAL_CONST(24.0);
    factor = (real_t)pow(2.0, exp);

    /* Apply gain factor */
    for (i = bottom; i < top; i++)
      spec[i] *= factor;

    bottom = top;
    }
  }
/*}}}*/
