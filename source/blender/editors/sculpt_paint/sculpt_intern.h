/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2006 by Nicholas Bishop
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/editors/sculpt_paint/sculpt_intern.h
 *  \ingroup edsculpt
 */
 

#ifndef __SCULPT_INTERN_H__
#define __SCULPT_INTERN_H__

#include "DNA_listBase.h"
#include "DNA_vec_types.h"
#include "DNA_key_types.h"
#include "DNA_brush_types.h"
#include "DNA_view3d_types.h"
#include "DNA_screen_types.h"
#include "DNA_scene_types.h"
#include "DNA_mesh_types.h"

#include "BLI_bitmap.h"
#include "BLI_dial.h"
#include "BLI_task.h"
#include "BLI_threads.h"

#include "BKE_pbvh.h"

#include "ED_view3d.h"

#include "paint_intern.h"

#include "bmesh.h"
#include "bmesh_tools.h"

struct bContext;
struct KeyBlock;
struct Object;
struct SculptUndoNode;

int sculpt_mode_poll(struct bContext *C);
int sculpt_mode_poll_view3d(struct bContext *C);
/* checks for a brush, not just sculpt mode */
int sculpt_poll(struct bContext *C);
int sculpt_poll_view3d(struct bContext *C);

/* Stroke */
bool sculpt_stroke_get_location(bContext *C, float out[3], const float mouse[2]);

/* Dynamic topology */
void sculpt_pbvh_clear(Object *ob);
void sculpt_dyntopo_node_layers_add(struct SculptSession *ss);
void sculpt_update_after_dynamic_topology_toggle(bContext *C);
void sculpt_dynamic_topology_enable(struct bContext *C);
void sculpt_dynamic_topology_disable(struct bContext *C,
                                     struct SculptUndoNode *unode);

/* Undo */

typedef enum {
	SCULPT_UNDO_COORDS,
	SCULPT_UNDO_HIDDEN,
	SCULPT_UNDO_MASK,
	SCULPT_UNDO_DYNTOPO_BEGIN,
	SCULPT_UNDO_DYNTOPO_END,
	SCULPT_UNDO_DYNTOPO_SYMMETRIZE,
} SculptUndoType;

typedef struct SculptUndoNode {
	struct SculptUndoNode *next, *prev;

	SculptUndoType type;

	char idname[MAX_ID_NAME];   /* name instead of pointer*/
	void *node;                 /* only during push, not valid afterwards! */

	float (*co)[3];
	float (*orig_co)[3];
	short (*no)[3];
	float *mask;
	int totvert;

	/* non-multires */
	int maxvert;                /* to verify if totvert it still the same */
	int *index;                 /* to restore into right location */
	BLI_bitmap *vert_hidden;

	/* multires */
	int maxgrid;                /* same for grid */
	int gridsize;               /* same for grid */
	int totgrid;                /* to restore into right location */
	int *grids;                 /* to restore into right location */
	BLI_bitmap **grid_hidden;

	/* bmesh */
	struct BMLogEntry *bm_entry;
	bool applied;
	CustomData bm_enter_vdata;
	CustomData bm_enter_edata;
	CustomData bm_enter_ldata;
	CustomData bm_enter_pdata;
	int bm_enter_totvert;
	int bm_enter_totedge;
	int bm_enter_totloop;
	int bm_enter_totpoly;

	/* shape keys */
	char shapeName[sizeof(((KeyBlock *)0))->name];
} SculptUndoNode;

/************** Access to original unmodified vertex data *************/

typedef struct SculptOrigVertData {
	BMLog *bm_log;

	SculptUndoNode *unode;
	float(*coords)[3];
	short(*normals)[3];
	const float *vmasks;

	/* Original coordinate, normal, and mask */
	const float *co;
	const short *no;
	float mask;
} SculptOrigVertData;


void sculpt_orig_vert_data_unode_init(SculptOrigVertData *data,
	Object *ob,
	SculptUndoNode *unode);
void sculpt_orig_vert_data_init(SculptOrigVertData *data,
	Object *ob,
	PBVHNode *node);
void sculpt_orig_vert_data_update(SculptOrigVertData *orig_data,
	PBVHVertexIter *iter);

/* Factor of brush to have rake point following behind
* (could be configurable but this is reasonable default). */
#define SCULPT_RAKE_BRUSH_FACTOR 0.25f

struct SculptRakeData {
	float follow_dist;
	float follow_co[3];
};

/** \name SculptProjectVector
*
* Fast-path for #project_plane_v3_v3v3
*
* \{ */

typedef struct SculptProjectVector {
	float plane[3];
	float len_sq;
	float len_sq_inv_neg;
	bool  is_valid;

} SculptProjectVector;

/* Single struct used by all BLI_task threaded callbacks, let's avoid adding 10's of those... */
typedef struct SculptThreadedTaskData {
	bContext *C;
	Sculpt *sd;
	Object *ob;
	Brush *brush;
	PBVHNode **nodes;
	int totnode;

	VPaint *vp;
	VPaintData *vpd;
	WPaintData *wpd;
	WeightPaintInfo *wpi;
	unsigned int* lcol;
	MeshElemMap **vertToLoopMaps;
	Mesh *me;


	/* Data specific to some callbacks. */
	/* Note: even if only one or two of those are used at a time, keeping them separated, names help figuring out
	*       what it is, and memory overhead is ridiculous anyway... */
	float flippedbstrength;
	float angle;
	float strength;
	bool smooth_mask;
	bool has_bm_orco;

	SculptProjectVector *spvc;
	float *offset;
	float *grab_delta;
	float *cono;
	float *area_no;
	float *area_no_sp;
	float *area_co;
	float(*mat)[4];
	float(*vertCos)[3];

	/* 0=towards view, 1=flipped */
	float(*area_cos)[3];
	float(*area_nos)[3];
	int *count;

	ThreadMutex mutex;

} SculptThreadedTaskData;

/*************** Brush testing declarations ****************/
typedef struct SculptBrushTest {
	float radius_squared;
	float location[3];
	float dist;
	int mirror_symmetry_pass;

	/* View3d clipping - only set rv3d for clipping */
	RegionView3D *clip_rv3d;
} SculptBrushTest;

typedef struct {
	Sculpt *sd;
	SculptSession *ss;
	float radius_squared;
	bool original;
} SculptSearchSphereData;

void sculpt_brush_test_init(SculptSession *ss, SculptBrushTest *test);
bool sculpt_brush_test(SculptBrushTest *test, const float co[3]);
bool sculpt_brush_test_sq(SculptBrushTest *test, const float co[3]);
bool sculpt_brush_test_fast(const SculptBrushTest *test, const float co[3]);
bool sculpt_brush_test_cube(SculptBrushTest *test, const float co[3], float local[4][4]);
bool sculpt_search_sphere_cb(PBVHNode *node, void *data_v);
float tex_strength(SculptSession *ss, Brush *br,
	const float point[3],
	const float len,
	const short vno[3],
	const float fno[3],
	const float mask,
	const int thread_id);


/* Cache stroke properties. Used because
* RNA property lookup isn't particularly fast.
*
* For descriptions of these settings, check the operator properties.
*/

typedef struct StrokeCache {
	/* Invariants */
	float initial_radius;
	float scale[3];
	int flag;
	float clip_tolerance[3];
	float initial_mouse[2];

	/* Variants */
	float radius;
	float radius_squared;
	float true_location[3];
	float true_last_location[3];
	float location[3];
	float last_location[3];
	bool is_last_valid;

	bool pen_flip;
	bool invert;
	float pressure;
	float mouse[2];
	float bstrength;
	float normal_weight;  /* from brush (with optional override) */

	/* The rest is temporary storage that isn't saved as a property */

	bool first_time; /* Beginning of stroke may do some things special */

	/* from ED_view3d_ob_project_mat_get() */
	float projection_mat[4][4];

	/* Clean this up! */
	ViewContext *vc;
	Brush *brush;

	float special_rotation;
	float grab_delta[3], grab_delta_symmetry[3];
	float old_grab_location[3], orig_grab_location[3];

	/* screen-space rotation defined by mouse motion */
	float   rake_rotation[4], rake_rotation_symmetry[4];
	bool is_rake_rotation_valid;
	struct SculptRakeData rake_data;

	int symmetry; /* Symmetry index between 0 and 7 bit combo 0 is Brush only;
				  * 1 is X mirror; 2 is Y mirror; 3 is XY; 4 is Z; 5 is XZ; 6 is YZ; 7 is XYZ */
	int mirror_symmetry_pass; /* the symmetry pass we are currently on between 0 and 7*/
	float true_view_normal[3];
	float view_normal[3];

	/* sculpt_normal gets calculated by calc_sculpt_normal(), then the
	* sculpt_normal_symm gets updated quickly with the usual symmetry
	* transforms */
	float sculpt_normal[3];
	float sculpt_normal_symm[3];

	/* Used for area texture mode, local_mat gets calculated by
	* calc_brush_local_mat() and used in tex_strength(). */
	float brush_local_mat[4][4];

	float plane_offset[3]; /* used to shift the plane around when doing tiled strokes */
	int tile_pass;

	float last_center[3];
	int radial_symmetry_pass;
	float symm_rot_mat[4][4];
	float symm_rot_mat_inv[4][4];
	bool original;
	float anchored_location[3];

	float vertex_rotation; /* amount to rotate the vertices when using rotate brush */
	Dial *dial;

	char saved_active_brush_name[MAX_ID_NAME];
	char saved_mask_brush_tool;
	int saved_smooth_size; /* smooth tool copies the size of the current tool */
	bool alt_smooth;

	float plane_trim_squared;

	bool supports_gravity;
	float true_gravity_direction[3];
	float gravity_direction[3];

	rcti previous_r; /* previous redraw rectangle */
	rcti current_r; /* current redraw rectangle */

} StrokeCache;

void sculpt_cache_free(StrokeCache *cache);

SculptUndoNode *sculpt_undo_push_node(Object *ob, PBVHNode *node, SculptUndoType type);
SculptUndoNode *sculpt_undo_get_node(PBVHNode *node);
void sculpt_undo_push_begin(const char *name);
void sculpt_undo_push_end(const struct bContext *C);

void sculpt_vertcos_to_key(Object *ob, KeyBlock *kb, float (*vertCos)[3]);

void sculpt_update_object_bounding_box(struct Object *ob);

bool sculpt_get_redraw_rect(ARegion *ar, RegionView3D *rv3d, Object *ob, rcti *rect);

#define SCULPT_THREADED_LIMIT 4

#endif
