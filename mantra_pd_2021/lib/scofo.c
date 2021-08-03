/* scofo -- score follower

copyright 2003 Miller Puckette.

Permission is granted to use this software for any purpose whatsoever, as
long as you retain this notice.

NODBODY MAKES ANY WARRANTY, EXPRESS OR IMPLIED, IN CONNECTION WITH THIS
SOFTWARE!

This file might be downloadable from http://www.crca.ucsd.edu/~msp .
*/

static char scofo_version[] = "scofo version 0.2.8";
/*
0.2.7 - take out FTS support; revise tempo scoring (experimentally)
0.2.8 - use canvas_open to find files better
*/
 
/*
stuff to try later ---
    pickups
    reading scores from explodes or pds
    pitch drift for vocal following
    octave errors
    trills
    output tempo; include real time expected to next input
    feature to play "following" score?
*/

#ifdef PD
#include <stdio.h>
#include "m_pd.h"
#endif /* PD */
#ifdef MSW
#include <io.h>
#include <string.h>
#endif

#define NPOSITION 32 	    	    /* window size in notes */
#define DEF_HIT_SCORE_IN_TUNE 5.
#define DEF_SKIP_PENALTY 10.
#define DEF_PITCH_ACCURACY 1.
#define DEF_TEMPO_PENALTY_PER_SEC 0.    /* unused */
#define DEF_TEMPO_PENALTY_LIMIT 0.     
#define DEF_TEMPO_JITTER 30.
#define DEF_TEMPO_PENALTY_SLOPE 0.
#define DEF_TEMPO_TRACK_SPEED 1.
#define DEF_SCORE_CHORD_MSEC -1
#define DEF_PERF_CHORD_MSEC -1

    /* each note in the current window of scored notes
    is given a "theory" as defined here.  The theory
    describes the strength of evidence that this is the most recently
    matched note, and if so, what the current tempo is (as a line in point
    slope form.)  There are also fields for describing the possibility that
    we're currently playing a chord starting with the given note.

    It might later turn out that we want to maintain multiple theories for
    the same note in case they have drastically different tempos.
    
    Polyphony, if enabled, is handled by a notion of "chord." 
    A scored note, followed by another one in less than "score chord threshold"
    milliseconds, is considered part of a chord.  The first note of each chord
    gets an extra theory for the whole chord. */

typedef struct _theory
{
    double t_realmatchtime;
    float t_scorematchtime;
    float t_sperr;  	    	/* inverse tempo, score time per real time */
    float t_value;  	    	/* value, or strength of evidence */
} t_theory;

typedef struct _note
{
    float n_time;   	    	    /* cumulative time in score */
    float n_pitch;  	    	    /* pitch */
    float n_key;    	    	    /* "event key" to output */
    float n_skip_penalty;   	    /* value we lose if player skips us */
    float n_tempo_penalty_slope;    /* value lost to double or zero tempo */
    float n_tempo_penalty_limit;    /* maximum value lost for bad timing */
    float n_pitch_accuracy; 	    /* maximum possible deviation in pitch */
} t_note;

typedef struct _position
{
    t_note *p_note; 	    /* the scored note we correspond to */
    t_theory p_theory;	    /* theory for this note; might later be an array */
    int p_chord;    	    /* if nonzero, # of notes in chord starting here */
    struct _position *p_next;
} t_position;

typedef struct _scofo
{
#ifdef PD
    t_object x_obj;		    /* object header */
    t_outlet *x_matchoutlet;	    /* outlet for notes we actually match */
    t_outlet *x_passoutlet; 	    /* outlet for notes we match or pass */
    t_canvas *x_canvas;     	    /* owning canvas */
#endif /* PD */
    double x_elapsedrealtime;
    t_note *x_notes;
    int x_nnotes;
    t_note *x_firstnote;
    t_position x_pos[NPOSITION];
    t_position *x_tail;
    t_position *x_head;
    t_position *x_lastout;
    float x_hit_score_in_tune;	/* parameters */
    float x_skip_penalty;
    float x_pitch_accuracy;
    float x_tempo_penalty_slope;
    float x_tempo_penalty_limit;
    float x_tempo_jitter;
    float x_tempo_track_speed;
    float x_score_chord_msec;   /* score chord threshold in msec */
    float x_perf_chord_msec;    /* performance chord threshold (UNUSED) */
    float x_prescore;		/* score for not having matched anything yet */
    char x_running;		/* true if we're turned on */
    char x_atend;		/* true if we've seen the last event */
    char x_lookedahead;		/* true if we've output a speculative event */
} t_scofo;

static void scofo_stop(t_scofo *x)
{
    x->x_running = 0;
}


#include <ctype.h>

static void scofo_read(t_scofo *x, char *filename)
{
#ifdef PD 
    char buf[MAXPDSTRING], *bufptr;
    int filedesc;
#endif
    FILE *fd;
    scofo_stop(x);
#ifdef PD 
#ifdef MSW
    {
	    /* for some reason fdopen() faile in MSW??? */
	char namebuf[MAXPDSTRING];
    
	if ((filedesc = canvas_open(x->x_canvas,
    	    filename, "", buf, &bufptr, MAXPDSTRING, 0)) < 0)
	{
    	    error("%s: can't open", filename);
    	    return;
	}
	else close(filedesc);
	namebuf[0] = 0;
	if (*buf)
    	    strcat(namebuf, buf), strcat(namebuf, "/");
	strcat(namebuf, bufptr);
	sys_bashfilename(namebuf, namebuf);
	if (!(fd = fopen(namebuf, "r")))
	{
    	    error("%s: can't open", namebuf);
    	    return;
	}
    }
#else
    if ((filedesc = canvas_open(x->x_canvas,
    	    filename, "", buf, &bufptr, MAXPDSTRING, 0)) < 0 
	    	|| !(fd = fdopen(filedesc, "r")))
    {
    	error("scofo: %s: can't open", filename);
    	return;
    }
#endif /* MSW */
#endif /* PD */
    {
    	int nnotes = x->x_nnotes;
	int size = nnotes * sizeof(t_note);
	float dtime;
	double cumtime = 0;
	x->x_notes = (t_note *)t_resizebytes(x->x_notes, size, 0);
	nnotes = size = 0;
	while (1)
	{
	    char instring[1000], *insp;
	    float key=0, pitch=0, time=0, skip_penalty = 0,
	    	tempo_penalty_slope = -1, tempo_penalty_limit = -1,
		    pitch_accuracy = 0;
	    if (!fgets(instring, 998, fd))
	    	break;
	    insp = instring;
	    while (isspace(*insp) && *insp)
	    	insp++;
	    if (insp[0] == ';' || insp[0] == '#')
	    	continue;
	    if (sscanf(insp, "%f%f%f%f%f%f%f", &dtime, &pitch, &key,
	    	&skip_penalty, &pitch_accuracy, &tempo_penalty_slope,
		    &tempo_penalty_limit) < 2)
	    	continue;
	    x->x_notes = (t_note *)t_resizebytes(x->x_notes,
	    	size, size + sizeof(t_note));
	    size += sizeof(t_note);
	    cumtime += dtime;
	    x->x_notes[nnotes].n_time = cumtime;
	    x->x_notes[nnotes].n_pitch = pitch;
	    x->x_notes[nnotes].n_key = key;
	    x->x_notes[nnotes].n_skip_penalty = skip_penalty;
	    x->x_notes[nnotes].n_pitch_accuracy = pitch_accuracy;
	    x->x_notes[nnotes].n_tempo_penalty_slope = tempo_penalty_slope;
	    x->x_notes[nnotes].n_tempo_penalty_limit = tempo_penalty_limit;
#if 0
	    fprintf(stderr, "%f   %f   %f   %f   %f   %f\n",
		 x->x_notes[nnotes].n_time,
		 x->x_notes[nnotes].n_pitch,
		 x->x_notes[nnotes].n_key,
		 x->x_notes[nnotes].n_skip_penalty,
		 x->x_notes[nnotes].n_pitch_accuracy, 
		 x->x_notes[nnotes].n_tempo_penalty_slope,
		 x->x_notes[nnotes].n_tempo_penalty_limit);
#endif
	    nnotes++;
	}
	x->x_nnotes = nnotes;
	post("scofo: read %d notes", x->x_nnotes);
	fclose(fd);
    }
}


    /* routine to refill the "position" array by reading forward in the
    score.  The positions are initialized and we detect chords. */ 
static void scofo_refill(t_scofo *x)
{
    t_note *note, *enote = x->x_notes + x->x_nnotes;
    t_position *pos, *end;
    float value, sperr, scorematchtime;
    double realmatchtime;

    if (x->x_atend)
    	return;
    pos = x->x_head->p_next;
    note = x->x_head->p_note + 1;
    value = x->x_head->p_theory.t_value;
    sperr = x->x_head->p_theory.t_sperr;
    scorematchtime = x->x_head->p_theory.t_scorematchtime;
    realmatchtime = x->x_head->p_theory.t_realmatchtime;

    	/* add and initialize the new positions */
    for (; note < enote && pos != x->x_tail; pos = pos->p_next)
    {
    	float penalty = (note->n_skip_penalty > 0 ?
	    note->n_skip_penalty : x->x_skip_penalty);
	value -= penalty;
	pos->p_note = note;
	pos->p_theory.t_value = value;
	pos->p_theory.t_sperr = sperr;
	pos->p_theory.t_realmatchtime = realmatchtime;
	pos->p_theory.t_scorematchtime = scorematchtime;
	pos->p_chord = 0;
	x->x_head = pos;
	note++;
    }
    if (note == enote)
    	x->x_atend = 1;

    	/* search for chords.  Here we don't care if part of a chord
	has fallen off the beginning of the window; we may mark the
	first note as a chord if it's actually the middle of one. */

    if (x->x_score_chord_msec >= 0)
    {
	t_position *chordpos = 0;
	int ninchord = 0;
	for (pos = x->x_tail, end = x->x_head; pos != end; pos = pos->p_next)
	{
            pos->p_chord = 0;
    	    if (chordpos)
	    {
		if (pos == end || pos->p_note->n_time >
		    chordpos->p_note->n_time + x->x_score_chord_msec)
		{
		    chordpos->p_chord = ninchord + (pos == end);
		    chordpos = 0;
		}
		else 
	    	    ninchord++;
	    }
	    if (!chordpos)
	    {
	    	if ((pos != end) && (pos->p_next->p_note->n_time <=
		    pos->p_note->n_time + x->x_score_chord_msec))
		{
		    chordpos = pos;
		    ninchord = 1;
		}
	    }
	}
    }
}

    /* send an event.  if the "match" flag is set, send it to the "match"
    outlet; otherwise to the "pass" outlet. */

static void scofo_sendevt(t_scofo *x, int match, float indx, float pitch,
    float key)
{
#ifdef PD
    t_outlet *z = (match ? x->x_matchoutlet : x->x_passoutlet);
    t_atom at[3];
    SETFLOAT(at, indx);
    SETFLOAT(at + 1, pitch);
    SETFLOAT(at + 2, key);
    outlet_list(z, 0, 3, at);
#endif /* PD */
}

    /* advance to note pointed to by "newnote" */ 
static void scofo_advanceto(t_scofo *x, t_position *newpos)
{
    int noteindex;
    t_note *note = (x->x_lastout ? x->x_lastout->p_note + 1 : x->x_firstnote),
    	*enote = x->x_notes + x->x_nnotes;
    if (newpos->p_note < note)
    	return;

	/* output passed notes */
    for (noteindex = note - x->x_notes;
    	note <= newpos->p_note && note < enote ; note++, noteindex++)
    	    scofo_sendevt(x, 0,
    	    	noteindex, note->n_pitch, note->n_key);

	/* output matched note */
    scofo_sendevt(x, 1, 
    	newpos->p_note - x->x_notes, newpos->p_note->n_pitch,
	    newpos->p_note->n_key);

	/* update value of "lastoutput" */
    x->x_lastout = newpos;

    if (x->x_atend && note == enote)
    	scofo_stop(x);
    else if (!x->x_atend)
    {
    	int npasttail;
    	t_position *wanttail;

	    /* we should have fewer than NPOSITION/2 theories buffered
	    behind the current position.  If there are more, drop them
	    from the tail and try to add new ones to the head. */
	
	npasttail = newpos - x->x_tail;
	if (npasttail < 0) npasttail += NPOSITION;
	if (npasttail >= NPOSITION/2)
	{
	    t_position *newtail = newpos - (NPOSITION/2 - 1);
	    if (newtail < x->x_pos)
	    	newtail += NPOSITION;
	    x->x_tail = newtail;
	    scofo_refill(x);
	    x->x_prescore = -1E20;  /* can't be at beginning anymore */
	}
    }
}

static void scofo_note(t_scofo *x, t_floatarg dtime, t_floatarg pitch,
    t_floatarg extra_note_penalty_in_tune,
    t_floatarg extra_note_penalty_out_of_tune)
{
    float bestvalue = -1E20;
    t_position *pos, *matchpos, *end, *bestpos = 0;
    int hitlast = 0, goforward = 0, leftinchord = 0;
    float param_accuracy = (x->x_pitch_accuracy > .001 ?
    	x->x_pitch_accuracy : .001);
    float param_skip_penalty = x->x_skip_penalty;
    float param_tempo_penalty_slope = x->x_tempo_penalty_slope;
    float param_tempo_penalty_limit = x->x_tempo_penalty_limit;
    float param_tempo_jitter = x->x_tempo_jitter;
    float param_matchvalue = x->x_hit_score_in_tune;
    float param_extravalue = -extra_note_penalty_in_tune;
    float param_extradetune =
	extra_note_penalty_in_tune - extra_note_penalty_out_of_tune;
    float param_trackspeed = x->x_tempo_track_speed * 0.001;
    double elapsedrealtime;
	    /* best theories of the previous position (or beginning
	    of score as a position), before and after having eaten
	    the new note. */
    t_theory lasttheorybefore, lasttheoryafter;
    lasttheoryafter.t_scorematchtime = lasttheoryafter.t_realmatchtime = 0;
    lasttheorybefore.t_scorematchtime = lasttheorybefore.t_realmatchtime = 0;
    elapsedrealtime = (x->x_elapsedrealtime += dtime);
    
    lasttheorybefore.t_sperr = lasttheoryafter.t_sperr = -1;
    lasttheorybefore.t_value = x->x_prescore;
    x->x_prescore = lasttheoryafter.t_value =
    	x->x_prescore - extra_note_penalty_out_of_tune;

    for (matchpos = pos = x->x_tail, end = x->x_head; 1; pos = pos->p_next)
    {
    	t_note *note;
    	float param_my_accuracy, param_my_skip_penalty;
	float param_my_tempo_penalty_slope, param_my_tempo_penalty_limit;
	float pitch_deviation;
	float value_drop, value_match, value_skip, time_penalty;
    	if (leftinchord)
	{
	    	/* we're in a chord and "matchpos" already points to the
		closest note in chord to the one we received. */
	    leftinchord--;
	}
    	else if (pos->p_chord)
	{
	    	/* this is the first note in a chord which has at least 2 notes
		in it.  Find the note in the chord that matches the pitch best
		and we'll use that note in place of each succeeding note in
		the chord for matching. */
    	    int notestotry = (leftinchord = pos->p_chord), i;
    	    t_position *trymatch;
	    float bestdeviation = 1e20;
	    for (trymatch = matchpos = pos; notestotry;
	    	trymatch = trymatch->p_next, notestotry--)
	    {
    	    	note = trymatch->p_note;
    	    	param_my_accuracy = (note->n_pitch_accuracy > 0 ?
	    	    note->n_pitch_accuracy : param_accuracy);
	    	pitch_deviation = (pitch - note->n_pitch) /
	    	    param_my_accuracy;
	    	if (pitch_deviation < 0)
		    pitch_deviation = -pitch_deviation;
    	    	if (pitch_deviation < bestdeviation)
		{
		    matchpos = trymatch;
		    bestdeviation = pitch_deviation;
		}
	    }
	    leftinchord--;
	}
	else
	{
	    	/* the scored note isn't in a chord. */
	    matchpos = pos;
	}
	
    	note = matchpos->p_note;
    	param_my_accuracy = (note->n_pitch_accuracy > 0 ?
	    note->n_pitch_accuracy : param_accuracy);
	    
	    /* pitch deviation normalized to (0,1) over accuracy range */
	pitch_deviation = (pitch - note->n_pitch) /
	    param_my_accuracy;
	if (pitch_deviation < 0)
	    pitch_deviation = -pitch_deviation;
	if (pitch_deviation > 1)
	    pitch_deviation = 1;
	    /* now fix it so there's no "deviation" for half the range,
	    but between 1/2 and 1, rise linearly to a deviation of 1. */
	pitch_deviation = 2 * pitch_deviation - 1;
	if (pitch_deviation < 0)
	    pitch_deviation = 0;
	param_my_skip_penalty = (note->n_skip_penalty > 0 ?
	    note->n_skip_penalty : param_skip_penalty);
	param_my_tempo_penalty_slope = (note->n_tempo_penalty_slope >= 0 ?
	    note->n_tempo_penalty_slope : param_tempo_penalty_slope);
	param_my_tempo_penalty_limit = (note->n_tempo_penalty_limit >= 0 ?
	    note->n_tempo_penalty_limit : param_tempo_penalty_limit);
        if (param_my_tempo_penalty_limit <= 0 ||
            param_my_tempo_penalty_slope <= 0 ||
                lasttheorybefore.t_sperr <= 0)
                    time_penalty = 0;
        else
        {
                /* time_deviation is seconds early(-) or late(+) compared to
                time this event was expected given the previous tempo */
	    float time_deviation =
	    	lasttheorybefore.t_realmatchtime +
	    	(note->n_time - lasttheorybefore.t_scorematchtime) /
	    	    lasttheorybefore.t_sperr
	    	        - elapsedrealtime;

                 /* reduce (toward zero) by allowed measurement jitter */
            if (time_deviation > param_tempo_jitter)
	    	time_deviation -= param_tempo_jitter;
	    else if (time_deviation < -param_tempo_jitter)
	    	time_deviation += param_tempo_jitter;
            else time_deviation = 0;
            if (time_deviation)
            {
                    /* figure out how much the time slope is off from the
                    previous tempo estimate.  deltarealtime is elapsed
                    real time since last match in this theory. */
	        float deltarealtime =
                    elapsedrealtime - lasttheorybefore.t_realmatchtime;

                    /* tempo deviation is relative change between
                    time slope (new minus prev. score time)/(deltarealtime)
                    compared with the previous tempo. */  
                float tempo_deviation = 1 -
                    (note->n_time - lasttheorybefore.t_scorematchtime) /
                           ((deltarealtime > 0.001 ? deltarealtime : 0.001) *
                                 lasttheorybefore.t_sperr);
                if (tempo_deviation < 0)
                    tempo_deviation = -tempo_deviation;
    	        time_penalty = param_my_tempo_penalty_slope * tempo_deviation;
	        if (time_penalty > param_my_tempo_penalty_limit)
	            time_penalty = param_my_tempo_penalty_limit;
            }
            else time_penalty = 0;
	}
	    /* value_drop is the value of staying at "pos" by dropping
	    the new note. */
	value_drop = pos->p_theory.t_value + param_extravalue +
	    param_extradetune * pitch_deviation;

	    /* value_match is the value of matching the incoming note to
	    the current scored note. */
	value_match =  lasttheorybefore.t_value + param_matchvalue -
	    (extra_note_penalty_out_of_tune + param_my_skip_penalty +
	    	param_matchvalue) * pitch_deviation -
		    time_penalty;

	    /* value_skip is the value of skipping the current note. */
	value_skip = lasttheoryafter.t_value - param_my_skip_penalty;

#if 0	/* debugging printout */
    	if (pos->p_note->n_key == 24 && (pitch > 66.85 && pitch < 67.05))
	{
	    fprintf(stderr, "value_match %f, value_drop %f, value_skip %f\n",
	    	value_match, value_drop, value_skip);
	    fprintf(stderr, "lasttheorybefore %f, lasttheoryafter %f\n",
	    	lasttheorybefore.t_value, lasttheoryafter.t_value);
	    fprintf(stderr, "dev %f, deduct %f, time %f\n",
	    	pitch_deviation, (extra_note_penalty_out_of_tune +
		    	    param_my_skip_penalty +
	    	    	    param_matchvalue) * pitch_deviation,
			    	time_penalty);
	}
#endif

	if (value_match > value_drop && value_match > value_skip)
	{
	    	/* the best score was gained by matching the incoming note.
    	    	We incorporate "matchpos" into the theory. */
	    lasttheoryafter.t_value = value_match;
	    if (lasttheorybefore.t_sperr < 0)
	    {
	    	    /* we're the first note for this theory.  Set default
		    tempo. */
		lasttheoryafter.t_sperr = 1;
		lasttheoryafter.t_scorematchtime = note->n_time;
		lasttheoryafter.t_realmatchtime = elapsedrealtime;
	    }
	    else
	    {
	    	    /* update tempo to incorporate note into theory */
		float newscoretime = note->n_time;
		float predictedrealtime = lasttheorybefore.t_realmatchtime
		    + lasttheorybefore.t_sperr *
			(newscoretime - lasttheorybefore.t_scorematchtime);
		float lateness = elapsedrealtime - predictedrealtime;
		if (lateness > 500.) lateness = 500.;
		else if (lateness < -500.) lateness = -500.;
		lasttheoryafter.t_sperr = lasttheorybefore.t_sperr
		    + param_trackspeed * lateness;
		lasttheoryafter.t_scorematchtime = newscoretime;
		lasttheoryafter.t_realmatchtime = elapsedrealtime;
	    }
	}
	else if (value_drop > value_skip)
	{
	    	/* just stay at the current note where we were already.
		 */
	    lasttheoryafter.t_value = value_drop;
	    lasttheoryafter.t_sperr = pos->p_theory.t_sperr;
	    lasttheoryafter.t_scorematchtime =
	    	pos->p_theory.t_scorematchtime;
	    lasttheoryafter.t_realmatchtime =
	    	pos->p_theory.t_realmatchtime;
	}
	else
	{
	    	/* skip here after already matching the note to some previous
		one.  We adopt the theory in place from lasttheoryafter, so we
		only have to update the value.*/
	    lasttheoryafter.t_value = value_skip;
	}

	lasttheorybefore = pos->p_theory;
	pos->p_theory = lasttheoryafter;
	if (lasttheoryafter.t_value > bestvalue)
	{
	    bestvalue = lasttheoryafter.t_value;
	    bestpos = pos;
	}
	if (pos == end)
	    break;
    }
    if (bestpos &&
    	((x->x_lastout && bestpos->p_note > x->x_lastout->p_note) ||
    	    (!x->x_lastout && bestvalue > x->x_prescore)))
    {
	scofo_advanceto(x, bestpos);
	x->x_lookedahead = 0;
    }
}

static void scofo_ft1(t_scofo *x, t_floatarg f)
{
    t_position *p;
    float deviation;
    if (!x->x_running || x->x_lookedahead || x->x_lastout == x->x_head)
    	return;
    p = (x->x_lastout ? x->x_lastout->p_next : x->x_tail);
    deviation = p->p_note->n_pitch - f;
    if (deviation < 0.) deviation = -deviation;
    if (deviation <= .5 * x->x_pitch_accuracy)
    {
	scofo_advanceto(x, p);
	x->x_lookedahead = 1;
    }
}

static void scofo_hit_score_in_tune(t_scofo *x, t_floatarg f)
{
    x->x_hit_score_in_tune = f;
}

static void scofo_pitch_accuracy(t_scofo *x, t_floatarg f)
{
    x->x_pitch_accuracy = f;
}

static void scofo_tempo_track_speed(t_scofo *x, t_floatarg f)
{
    x->x_tempo_track_speed = f;
}

static void scofo_tempo_penalty_per_sec(t_scofo *x, t_floatarg f)
{
    post("scofo: I don't use scofo_tempo_penalty_per_sec any more");
}

static void scofo_tempo_penalty_limit(t_scofo *x, t_floatarg f)
{
    x->x_tempo_penalty_limit = f;
}

static void scofo_tempo_penalty_slope(t_scofo *x, t_floatarg f)
{
    x->x_tempo_penalty_slope = f;
}

static void scofo_tempo_jitter(t_scofo *x, t_floatarg f)
{
    x->x_tempo_jitter = f;
}

static void scofo_skip_penalty(t_scofo *x, t_floatarg f)
{
    x->x_skip_penalty = f;
}

static void scofo_poly(t_scofo *x, t_floatarg f1, t_floatarg f2)
{
    if (f1 <= 0)
    	x->x_score_chord_msec = x->x_perf_chord_msec = -1;
    else
    {
    	if (f1 > 0 && f2 <= 0)
	    f2 = f1;
    	x->x_score_chord_msec = f1;
	x->x_perf_chord_msec = f2;
    }
}

static void scofo_print(t_scofo *x)
{
    if (x->x_running)
    {
    	t_position *p;
	if (!x->x_lastout) post("running at beginning (score %f}", 
	    x->x_prescore);
	else post("running: index %d", x->x_lastout->p_note - x->x_notes);
	post("    pitch     value     realtime  scoretime expansion chord");
	for (p = x->x_tail; ; p = p->p_next)
	{
	    if (p->p_chord)
	    	post("%9.0f %9.2f %9.2f %9.2f %9.2f      %d",
		p->p_note->n_pitch,
	    	p->p_theory.t_value,
		(float) p->p_theory.t_realmatchtime,
		p->p_theory.t_scorematchtime,
	    	p->p_theory.t_sperr, p->p_chord);
	    else 
	    	post("%9.0f %9.2f %9.2f %9.2f %9.2f",
		p->p_note->n_pitch,
	    	p->p_theory.t_value,
		(float) p->p_theory.t_realmatchtime,
		p->p_theory.t_scorematchtime,
	    	p->p_theory.t_sperr);

	    if (p == x->x_head)
	    	break;
	}
    }
    post("hit-score-in-tune %f", x->x_hit_score_in_tune);
    post("pitch-accuracy %f", x->x_pitch_accuracy);
    post("tempo-track-speed %f", x->x_tempo_track_speed);
    post("tempo-penalty-slope %f", x->x_tempo_penalty_slope);
    post("tempo-jitter %f", x->x_tempo_jitter);
    post("tempo-penalty-limit %f", x->x_tempo_penalty_limit);
    post("skip-penalty %f", x->x_skip_penalty);
    post("poly %f %f", x->x_score_chord_msec, x->x_perf_chord_msec);
}

    /* follow starting from the given index */
static void scofo_followindex(t_scofo *x, int noteindex)
{
    scofo_stop(x);
    x->x_atend = 0;
    if (noteindex >= x->x_nnotes)
    {
	scofo_stop(x);
	return;
    }
    x->x_firstnote = x->x_notes + noteindex;
    x->x_pos[0].p_note = x->x_notes + noteindex;
    x->x_pos[0].p_theory.t_value =
    	- x->x_notes[noteindex].n_skip_penalty;
    x->x_pos[0].p_theory.t_sperr = -1;
    x->x_pos[0].p_theory.t_realmatchtime = 0;
    x->x_pos[0].p_theory.t_scorematchtime = 0;
    x->x_head = x->x_tail = x->x_pos;
    x->x_lastout = 0;
    x->x_lookedahead = 0;
    x->x_prescore = 0;
    x->x_running = 1;
    x->x_atend = 0;
    x->x_elapsedrealtime = 0;
    scofo_refill(x);
}

    /* follow from beginning */
void scofo_follow(t_scofo *x)
{
    scofo_followindex(x, 0);
}

    /* follow from first note matching the key */
void scofo_followat(t_scofo *x, t_floatarg key)
{
    int noteindex;
    for (noteindex = 0; noteindex < x->x_nnotes; noteindex++)
    	if (x->x_notes[noteindex].n_key == key)
    {
    	if (noteindex+1 == x->x_nnotes)
	{
	    post("end of score");
	    scofo_stop(x);
    	}
	else scofo_followindex(x, noteindex+1);
	return;
    }
    post("scofo: key %f not found", key);
    scofo_stop(x);
}

    /* initialize a new instance */
static void *scofo_doinit(t_scofo *x)
{
    t_position *p;
    int i;

    x->x_notes = (t_note *)t_getbytes(0);
    x->x_nnotes = 0;
    x->x_running = 0;
    for (i = 0, p = x->x_pos; i < NPOSITION; i++, p++)
	p->p_next = (i == NPOSITION-1 ? x->x_pos : p+1);

    scofo_hit_score_in_tune(x, DEF_HIT_SCORE_IN_TUNE);
    scofo_skip_penalty(x, DEF_SKIP_PENALTY);
    scofo_pitch_accuracy(x, DEF_PITCH_ACCURACY);
    scofo_tempo_jitter(x, DEF_TEMPO_JITTER);
    scofo_tempo_penalty_slope(x, DEF_TEMPO_PENALTY_SLOPE);
    scofo_tempo_penalty_limit(x, DEF_TEMPO_PENALTY_LIMIT);
    scofo_tempo_track_speed(x, DEF_TEMPO_TRACK_SPEED);
    scofo_poly(x, DEF_SCORE_CHORD_MSEC, DEF_PERF_CHORD_MSEC);
    scofo_stop(x);
    return x;
}


#ifdef PD /* PD glue code */

static void scofo_list(t_scofo *x, t_symbol *s, int ac, t_atom *av)
{
    if (!x->x_running)
    	return;

    if (ac < 4)
    	post("scofo_list: needs three elements");
    else
    	scofo_note(x, atom_getfloat(av), atom_getfloat(av + 1),
	    atom_getfloat(av + 2), atom_getfloat(av + 3));
}

static void scofo_pd_read(t_scofo *x, t_symbol *s)
{
    scofo_read(x, s->s_name);
}

static t_class *scofo_class;

static void *scofo_new( void)
{
    t_scofo *x;
    t_position *p;
    
    x = (t_scofo *)pd_new(scofo_class);

    x->x_matchoutlet = outlet_new(&x->x_obj, &s_list);
    x->x_passoutlet = outlet_new(&x->x_obj, &s_list);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_canvas = canvas_getcurrent();
    scofo_doinit(x);
    return x;
}

void scofo_setup( void)
{
    scofo_class = class_new(gensym("scofo"), (t_newmethod)scofo_new,
    	0, sizeof(t_scofo), 0, 0);

    class_addmethod(scofo_class, (t_method)scofo_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_print,
    	gensym("print"), 0);
    class_addmethod(scofo_class, (t_method)scofo_follow,
    	gensym("follow"), 0);
    class_addmethod(scofo_class, (t_method)scofo_followat,
    	gensym("followat"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_stop,
    	gensym("stop"), 0);
    class_addmethod(scofo_class, (t_method)scofo_hit_score_in_tune,
	gensym("hit-score-in-tune"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_pitch_accuracy,
	gensym("pitch-accuracy"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_tempo_track_speed,
	gensym("tempo-track-speed"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_tempo_penalty_per_sec,
	gensym("tempo-penalty-per-sec"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_tempo_penalty_limit,
	gensym("tempo-penalty-limit"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_tempo_jitter,
	gensym("tempo-jitter"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_tempo_penalty_slope,
	gensym("tempo-penalty-slope"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_skip_penalty,
	gensym("skip-penalty"), A_FLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_poly,
	gensym("poly"), A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(scofo_class, (t_method)scofo_pd_read,
	gensym("read"), A_SYMBOL, 0);
    class_addlist(scofo_class, scofo_list);
    post(scofo_version);
}
#endif /* PD */

