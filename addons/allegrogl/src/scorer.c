/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file scorer.c
  * \brief Scoring system for screen mode selection (internal).
  */

#include <allegro.h>

#include "alleggl.h"
#include "allglint.h"


static int best, best_score;


#define target allegro_gl_display_info
#define req __allegro_gl_required_settings
#define sug __allegro_gl_suggested_settings

#define PREFIX_I                "agl-scorer INFO: "


/* __allegro_gl_fill_in_info()
 * Will fill in missing settings by 'guessing'
 * what the user intended.
 */
void __allegro_gl_fill_in_info() {
	
	int all_components = AGL_RED_DEPTH | AGL_GREEN_DEPTH | AGL_BLUE_DEPTH
	                   | AGL_ALPHA_DEPTH;

	/* If all color components were set, but not the color depth */
	if ((((req | sug) & AGL_COLOR_DEPTH) == 0)
	 && (((req | sug) & all_components) == all_components)) {

		target.colour_depth = target.pixel_size.rgba.r
		                    + target.pixel_size.rgba.g
		                    + target.pixel_size.rgba.b
		                    + target.pixel_size.rgba.a;
			
		/* Round depth to 8 bits */
		target.colour_depth = (target.colour_depth + 7) / 8;
	}
	/* If only some components were set, guess the others */
	else if ((req | sug) & all_components) {
		
		int avg = ((req | sug) & AGL_RED_DEPTH   ? target.pixel_size.rgba.r: 0)
		        + ((req | sug) & AGL_GREEN_DEPTH ? target.pixel_size.rgba.g: 0)
		        + ((req | sug) & AGL_BLUE_DEPTH  ? target.pixel_size.rgba.b: 0)
		        + ((req | sug) & AGL_ALPHA_DEPTH ? target.pixel_size.rgba.a: 0);
			
		int num = ((req | sug) & AGL_RED_DEPTH   ? 1 : 0)
		        + ((req | sug) & AGL_GREEN_DEPTH ? 1 : 0)
		        + ((req | sug) & AGL_BLUE_DEPTH  ? 1 : 0)
		        + ((req | sug) & AGL_ALPHA_DEPTH ? 1 : 0);

		avg /= (num ? num : 1);
		
		if (((req | sug) & AGL_RED_DEPTH )== 0) {
			sug |= AGL_RED_DEPTH;
			target.pixel_size.rgba.r = avg;
		}
		if (((req | sug) & AGL_GREEN_DEPTH) == 0) {
			sug |= AGL_GREEN_DEPTH;
			target.pixel_size.rgba.g = avg;
		}
		if (((req | sug) & AGL_BLUE_DEPTH) == 0) {
			sug |= AGL_BLUE_DEPTH;
			target.pixel_size.rgba.b = avg;
		}
		if (((req | sug) & AGL_ALPHA_DEPTH) == 0) {
			sug |= AGL_ALPHA_DEPTH;
			target.pixel_size.rgba.a = avg;
		}
		
		/* If color depth wasn't defined, figure it out */
		if (((req | sug) & AGL_COLOR_DEPTH) == 0) {
			__allegro_gl_fill_in_info();
		}
	}
	
	/* If the user forgot to set a color depth in AGL, but used the
	 * Allegro one instead
	 */
	if ((((req | sug) & AGL_COLOR_DEPTH) == 0) && (target.colour_depth == 0)) {
		BITMAP *temp = create_bitmap(1, 1);
		if (temp) {
			allegro_gl_set(AGL_COLOR_DEPTH, bitmap_color_depth(temp));
			allegro_gl_set(AGL_REQUIRE, AGL_COLOR_DEPTH);
			destroy_bitmap(temp);
		}
	}


	/* Prefer double-buffering */
	if (!((req | sug) & AGL_DOUBLEBUFFER)) {
		allegro_gl_set(AGL_DOUBLEBUFFER, 1);
		allegro_gl_set(AGL_SUGGEST, AGL_DOUBLEBUFFER);
	}

	/* Prefer no multisamping */
	if (!((req | sug) & (AGL_SAMPLE_BUFFERS | AGL_SAMPLES))) {
		allegro_gl_set(AGL_SAMPLE_BUFFERS, 0);
		allegro_gl_set(AGL_SAMPLES,        0);
		allegro_gl_set(AGL_SUGGEST, AGL_SAMPLE_BUFFERS | AGL_SAMPLES);
	}

	/* Prefer monoscopic */
	if (!((req | sug) & AGL_STEREO)) {
		allegro_gl_set(AGL_STEREO, 0);
		allegro_gl_set(AGL_SUGGEST, AGL_STEREO);
	}

	/* Prefer unsigned normalized buffers */
	if (!((req | sug) & (AGL_FLOAT_COLOR | AGL_FLOAT_Z))) {
		allegro_gl_set(AGL_FLOAT_COLOR, 0);
		allegro_gl_set(AGL_FLOAT_Z, 0);
		allegro_gl_set(AGL_SUGGEST, AGL_FLOAT_COLOR | AGL_FLOAT_Z);
	}
}



void __allegro_gl_reset_scorer(void)
{
	best = -1;
	best_score = -1;
}



static int get_score(struct allegro_gl_display_info *dinfo)
{
	int score = 0;

	if (dinfo->colour_depth != target.colour_depth) {
		if (req & AGL_COLOR_DEPTH) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Color depth requirement not met."));
			return -1;
		}
	}
	else {
		/* If requested color depths agree */
		score += 128;
	}
	

	if (sug & AGL_COLOR_DEPTH) {
		if (dinfo->colour_depth < target.colour_depth)
			score += (96 * dinfo->colour_depth) / target.colour_depth;
		else
			score += 96 + 96 / (1 + dinfo->colour_depth - target.colour_depth);
	}

	
	/* check colour component widths here and Allegro formatness */
	if ((req & AGL_RED_DEPTH)
	 && (dinfo->pixel_size.rgba.r != target.pixel_size.rgba.r)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Red depth requirement not met."));
		return -1;
	}

	if (sug & AGL_RED_DEPTH) {
		if (dinfo->pixel_size.rgba.r < target.pixel_size.rgba.r) {
			score += (16 * dinfo->pixel_size.rgba.r) / target.pixel_size.rgba.r;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->pixel_size.rgba.r - target.pixel_size.rgba.r);
		}
	}

	if ((req & AGL_GREEN_DEPTH)
	 && (dinfo->pixel_size.rgba.g != target.pixel_size.rgba.g)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Green depth requirement not met."));
		return -1;
	}

	if (sug & AGL_GREEN_DEPTH) {
		if (dinfo->pixel_size.rgba.g < target.pixel_size.rgba.g) {
			score += (16 * dinfo->pixel_size.rgba.g) / target.pixel_size.rgba.g;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->pixel_size.rgba.g - target.pixel_size.rgba.g);
		}
	}

	if ((req & AGL_BLUE_DEPTH)
	 && (dinfo->pixel_size.rgba.b != target.pixel_size.rgba.b)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Blue depth requirement not met."));
		return -1;
	}

	if (sug & AGL_BLUE_DEPTH) {
		if (dinfo->pixel_size.rgba.b < target.pixel_size.rgba.b) {
			score += (16 * dinfo->pixel_size.rgba.b) / target.pixel_size.rgba.b;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->pixel_size.rgba.b - target.pixel_size.rgba.b);
		}
	}

	if ((req & AGL_ALPHA_DEPTH)
	 && (dinfo->pixel_size.rgba.a != target.pixel_size.rgba.a)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Alpha depth requirement not met."));
		return -1;
	}

	if (sug & AGL_ALPHA_DEPTH) {
		if (dinfo->pixel_size.rgba.a < target.pixel_size.rgba.a) {
			score += (16 * dinfo->pixel_size.rgba.a) / target.pixel_size.rgba.a;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->pixel_size.rgba.a - target.pixel_size.rgba.a);
		}
	}


	if ((req & AGL_ACC_RED_DEPTH)
	 && (dinfo->accum_size.rgba.r != target.accum_size.rgba.r)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		         get_config_text("Accumulator Red depth requirement not met."));
		return -1;
	}

	if (sug & AGL_ACC_RED_DEPTH) {
		if (dinfo->accum_size.rgba.r < target.accum_size.rgba.r) {
			score += (16 * dinfo->accum_size.rgba.r) / target.accum_size.rgba.r;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->accum_size.rgba.r - target.accum_size.rgba.r);
		}
	}

	if ((req & AGL_ACC_GREEN_DEPTH)
	 && (dinfo->accum_size.rgba.g != target.accum_size.rgba.g)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		       get_config_text("Accumulator Green depth requirement not met."));
		return -1;
	}

	if (sug & AGL_ACC_GREEN_DEPTH) {
		if (dinfo->accum_size.rgba.g < target.accum_size.rgba.g) {
			score += (16 * dinfo->accum_size.rgba.g) / target.accum_size.rgba.g;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->accum_size.rgba.g - target.accum_size.rgba.g);
		}
	}

	if ((req & AGL_ACC_BLUE_DEPTH)
	 && (dinfo->accum_size.rgba.b != target.accum_size.rgba.b)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		        get_config_text("Accumulator Blue depth requirement not met."));
		return -1;
	}

	if (sug & AGL_ACC_BLUE_DEPTH) {
		if (dinfo->accum_size.rgba.b < target.accum_size.rgba.b) {
			score += (16 * dinfo->accum_size.rgba.b) / target.accum_size.rgba.b;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->accum_size.rgba.b - target.accum_size.rgba.b);
		}
	}

	if ((req & AGL_ACC_ALPHA_DEPTH)
	 && (dinfo->accum_size.rgba.a != target.accum_size.rgba.a)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		       get_config_text("Accumulator Alpha depth requirement not met."));
		return -1;
	}

	if (sug & AGL_ACC_ALPHA_DEPTH) {
		if (dinfo->accum_size.rgba.a < target.accum_size.rgba.a) {
			score += (16 * dinfo->accum_size.rgba.a) / target.accum_size.rgba.a;
		}
		else {
			score += 16
			   + 16 / (1 + dinfo->accum_size.rgba.a - target.accum_size.rgba.a);
		}
	}



	if (!dinfo->doublebuffered != !target.doublebuffered) {
		if (req & AGL_DOUBLEBUFFER) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			             get_config_text("Double Buffer requirement not met."));
			return -1;
		}
	}
	else {
		score += (sug & AGL_DOUBLEBUFFER) ? 256 : 1;
	}

	if (!dinfo->stereo != !target.stereo) {
		if (req & AGL_STEREO) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			             get_config_text("Stereo Buffer requirement not met."));
			return -1;
		}
	}
	else {
		if (sug & AGL_STEREO) {
			score += 128;
		}
	}

	if ((req & AGL_AUX_BUFFERS) && (dinfo->aux_buffers < target.aux_buffers)) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Aux Buffer requirement not met."));
		return -1;
	}
	
	if (sug & AGL_AUX_BUFFERS) {
		if (dinfo->aux_buffers < target.aux_buffers) {
			score += (64 * dinfo->aux_buffers) / target.aux_buffers;
		}
		else {
			score += 64 + 64 / (1 + dinfo->aux_buffers - target.aux_buffers);
		}
	}

	if ((req & AGL_Z_DEPTH) && (dinfo->depth_size != target.depth_size)) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Z-Buffer requirement not met."));
		return -1;
	}
	if (sug & AGL_Z_DEPTH) {
		if (dinfo->depth_size < target.depth_size) {
			score += (64 * dinfo->depth_size) / target.depth_size;
		}
		else {
			score += 64 + 64 / (1 + dinfo->depth_size - target.depth_size);
		}
	}

	if ((req & AGL_STENCIL_DEPTH)
	 && (dinfo->stencil_size != target.stencil_size)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Stencil depth requirement not met."));
		return -1;
	}
		
	if (sug & AGL_STENCIL_DEPTH) {
		if (dinfo->stencil_size < target.stencil_size) {
			score += (64 * dinfo->stencil_size) / target.stencil_size;
		}
		else {
			score += 64 + 64 / (1 + dinfo->stencil_size - target.stencil_size);
		}
	}

	if ((req & AGL_RENDERMETHOD)
	  && ((dinfo->rmethod != target.rmethod) || (target.rmethod == 2))) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Render Method requirement not met"));
		return -1;
	}

	if ((sug & AGL_RENDERMETHOD) && (dinfo->rmethod == target.rmethod)) {
		score += 1024;
	}
	else if (dinfo->rmethod == 1) {
		score++; /* Add 1 for h/w accel */
	}

	if ((req & AGL_SAMPLE_BUFFERS)
	 && (dinfo->sample_buffers != target.sample_buffers)) {
			ustrzcpy(allegro_gl_error, AGL_ERROR_SIZE,
			        get_config_text("Multisample Buffers requirement not met"));
			return -1;
	}
        
	if (sug & AGL_SAMPLE_BUFFERS) {
		if (dinfo->sample_buffers < target.sample_buffers) {
			score += (64 * dinfo->sample_buffers) / target.sample_buffers;
		}
		else {
			score += 64
			       + 64 / (1 + dinfo->sample_buffers - target.sample_buffers);
		}
	}

	if ((req & AGL_SAMPLES) && (dinfo->samples != target.samples)) {
		ustrzcpy(allegro_gl_error, AGL_ERROR_SIZE,
				get_config_text("Multisample Samples requirement not met"));
		return -1;
	}
        
	if (sug & AGL_SAMPLES) {
		if (dinfo->samples < target.samples) {
			score += (64 * dinfo->samples) / target.samples;
		}
		else {
			score += 64 + 64 / (1 + dinfo->samples - target.samples);
		}
	}


	if (!dinfo->float_color != !target.float_color) {
		if (req & AGL_FLOAT_COLOR) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Float Color requirement not met."));
			return -1;
		}
	}
	else {
		if (sug & AGL_FLOAT_COLOR) {
			score += 128;
		}
	}

	if (!dinfo->float_depth != !target.float_depth) {
		if (req & AGL_FLOAT_Z) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Float Depth requirement not met."));
			return -1;
		}
	}
	else {
		if (sug & AGL_FLOAT_Z) {
			score += 128;
		}
	}
	
	TRACE(PREFIX_I "Score is : %i\n", score);
	return score;
}

#undef target
#undef req
#undef sug



int __allegro_gl_score_config(int refnum,
							  struct allegro_gl_display_info *dinfo)
{
	int score = get_score(dinfo);
	if (score == -1) {
		TRACE(PREFIX_I "score_config: %s\n", allegro_gl_error);
		return score;
	}
	
	if (score == best_score) {
		/*
		TRACE(PREFIX_I "score_config: score == best_score, should we change "
		      "scoring algorithm?\n");
		*/
	}

	if (score > best_score) {
		best_score = score;
		best = refnum;
	}

	return score;
}



int __allegro_gl_best_config(void)
{
	return best;
}

