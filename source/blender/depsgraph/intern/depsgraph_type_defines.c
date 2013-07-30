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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2013 Blender Foundation.
 * All rights reserved.
 *
 * Original Author: Joshua Leung
 * Contributor(s): None Yet
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 * Defines and code for core node types
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_ghash.h"
#include "BLI_utildefines.h"

#include "DNA_anim_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "BKE_animsys.h"
#include "BKE_depsgraph.h"

#include "RNA_access.h"
#include "RNA_types.h"

#include "depsgraph_types.h"
#include "depsgraph_intern.h"


/* ******************************************************** */
/* Generic Nodes */

/* ID Node ================================================ */

/* Initialise 'id' node - from pointer data given */
static void dnti_id_ref__init_data(DepsNode *node, ID *id)
{
	IDDepsNode *id_node = (IDDepsNode *)node;
	
	/* store ID-pointer */
	BLI_assert(id != NULL);
	id_node->id = id;
	
	/* init components hash - eDepsNode_Type : ComponentDepsNode */
	id_node->component_hash = BLI_ghash_int_new("IDDepsNode Component Hash");
	
	/* NOTE: components themselves are created if/when needed.
	 * This prevents problems with components getting added 
	 * twice if an ID-Ref needs to be created to house it...
	 */
}

/* Helper for freeing ID nodes - Used by component hash to free data... */
static void dnti_id_ref__hash_free_component(void *component_p)
{
	DEG_free_node((DepsNode *)component_p);
}

/* Free 'id' node */
static void dnti_id_ref__free_data(DepsNode *node)
{
	IDDepsNode *id_node = (IDDepsNode *)node;
	
	/* free components (and recursively, their data) while we empty the hash */
	BLI_ghash_free(id_node->component_hash, NULL, dnti_id_ref__hash_free_component);
}

/* Copy 'id' node */
static void dnti_id_ref__copy_data(DepsgraphCopyContext *dcc, DepsNode *dst, const DepsNode *src)
{
	const IDDepsNode *src_node = (const IDDepsNode *)src;
	IDDepsNode *dst_node       = (IDDepsNode *)dst;
	
	GHashIterator *hashIter;
	
	/* create new hash for destination (src's one is still linked to it at this point) */
	dst->component_hash = BLI_ghash_int_new("IDDepsNode Component Hash Copy");
	
	/* iterate over items in original hash, adding them to new hash */
	GHASH_ITER(hashIter, src->component_hash) {
		/* get current <type : component> mapping */
		eDepsNode_Type c_type   = GET_INT_FROM_POINTER(BLI_ghashIterator_getKey(hashIter));
		DepsNode *old_component = BLI_ghashIterator_getValue(hashIter);
		
		/* make a copy of component */
		DepsNode *component     = DEG_copy_node(old_component);
		
		/* add new node to hash... */
		BLI_ghash_insert(dst->component_hash, SET_INT_IN_POINTER(c_type), old_component);
	}
	BLI_ghashIterator_free(hashIter);
	
	// TODO: perform a second loop to fix up links?
}

/* Add 'id' node to graph */
static void dnti_id_ref__add_to_graph(Depsgraph *graph, DepsNode *node, ID *id)
{
	/* add to hash so that it can be found */
	BLI_ghash_insert(graph->id_hash, id, node);
}

/* Remove 'id' node from graph */
static void dnti_id_ref__remove_from_graph(Depsgraph *graph, DepsNode *node)
{
	/* remove toplevel node and hash entry, but don't free... */
	BLI_ghash_remove(graph->nodehash, id, NULL, NULL);
}

/* ID Node Type Info */
static DepsNodeTypeInfo DNTI_ID_REF = {
	/* type */               DEPSNODE_TYPE_ID_REF,
	/* size */               sizeof(IDDepsNode),
	/* name */               "ID Node",
	
	/* init_data() */        dnti_id_ref__init_data,
	/* free_data() */        dnti_id_ref__free_data,
	/* copy_data() */        dnti_id_ref__copy_data,
	
	/* add_to_graph() */     dnti_id_ref__add_to_graph,
	/* remove_from_graph()*/ dnti_id_ref__remove_from_graph,
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* ******************************************************** */
/* Outer Nodes */

/* Standard Component Methods ============================= */

/* Initialise 'component' node - from pointer data given */
static void dnti_component__init_data(DepsNode *node, ID *UNUSED(id))
{
	ComponentDepsNode *component = (ComponentDepsNode *)node;
	
	/* create op-node hash */
	component->op_hash = BLI_ghash_str_new("DepsNode Component - Operations Hash");
	
	/* hook up eval context? */
	// XXX: maybe this needs a special API?
}

/* Copy 'component' node */
static void dnti_component__copy_data(DepsgraphCopyContext *dcc, DepsNode *dst, const DepsNode *src)
{
	const ComponentDepsNode *src_node = (const ComponentDepsNode *)src;
	ComponentDepsNode *dst_node       = (ComponentDepsNode *)dst;
	
	DepsNode *op_node;
	
	/* create new op-node hash (to host the copied data) */
	dst_node->op_hash = BLI_ghash_str_new("DepsNode Component - Operations Hash (Copy");
	
	/* duplicate list of operation nodes */
	
}

/* Free 'component' node */
static void dnti_component__free_data(DepsNode *node)
{
	ComponentDepsNode *component = (ComponentDepsNode *)node;
	DepsNode *node, *next;
	
	/* free nodes and list of nodes */
	for (node = component->ops.first; node; node = next) {
		next = node->next;
		
		DEG_free_node(node);
		BLI_freelinkN(&component->ops, node);
	}
	
	/* free hash too - no need to free as it should be empty now */
	BLI_ghash_free(component->op_hash, NULL, NULL);
	component->op_hash = NULL;
}

/* Add 'component' node to graph */
static void dnti_component__add_to_graph(Depsgraph *graph, DepsNode *node, ID *id)
{
	IDDepsNode *id_node;
	
	/* find ID node that we belong to (and create it if it doesn't exist!) */
	id_node = (IDDepsNode *)DEG_get_node(graph, id, DEPSNODE_TYPE_ID_REF, NULL);
	BLI_assert(id_node != NULL);
	
	/* add component to id */
	BLI_ghash_insert(id_node->component_hash, SET_INT_IN_POINTER(node->type), node);
}

/* Standard Component Defines ============================= */

/* Parameters */
static DepsNodeTypeInfo DNTI_PARAMETERS = {
	/* type */               DEPSNODE_TYPE_PARAMETERS,
	/* size */               sizeof(ComponentDepsNode),
	/* name */               "Parameters Component",
	
	/* init_data() */        dnti_component__init_data,
	/* free_data() */        dnti_component__free_data,
	/* copy_data() */        dnti_component__copy_data,
	
	/* add_to_graph() */     dnti_component__add_to_graph,
	/* remove_from_graph()*/ NULL, // XXX...
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* Animation */
static DepsNodeTypeInfo DNTI_ANIMATION = {
	/* type */               DEPSNODE_TYPE_ANIMATION,
	/* size */               sizeof(ComponentDepsNode),
	/* name */               "Animation Component",
	
	/* init_data() */        dnti_component__init_data,
	/* free_data() */        dnti_component__free_data,
	/* copy_data() */        dnti_component__copy_data,
	
	/* add_to_graph() */     dnti_component__add_to_graph,
	/* remove_from_graph()*/ NULL, // XXX...
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* Transform */
static DepsNodeTypeInfo DNTI_TRANSFORM = {
	/* type */               DEPSNODE_TYPE_TRANSFORM,
	/* size */               sizeof(ComponentDepsNode),
	/* name */               "Transform Component",
	
	/* init_data() */        dnti_component__init_data,
	/* free_data() */        dnti_component__free_data,
	/* copy_data() */        dnti_component__copy_data,
	
	/* add_to_graph() */     dnti_component__add_to_graph,
	/* remove_from_graph()*/ NULL, // XXX...
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* Proxy */
static DepsNodeTypeInfo DNTI_PROXY = {
	/* type */               DEPSNODE_TYPE_TRANSFORM,
	/* size */               sizeof(ComponentDepsNode),
	/* name */               "Proxy Component",
	
	/* init_data() */        dnti_component__init_data,
	/* free_data() */        dnti_component__free_data,
	/* copy_data() */        dnti_component__copy_data,
	
	/* add_to_graph() */     dnti_component__add_to_graph,
	/* remove_from_graph()*/ NULL, // XXX...
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* Geometry */
static DepsNodeTypeInfo DNTI_GEOMETRY = {
	/* type */               DEPSNODE_TYPE_GEOMETRY,
	/* size */               sizeof(ComponentDepsNode),
	/* name */               "Transform Component",
	
	/* init_data() */        dnti_component__init_data,
	/* free_data() */        dnti_component__free_data,
	/* copy_data() */        dnti_component__copy_data,
	
	/* add_to_graph() */     dnti_component__add_to_graph,
	/* remove_from_graph()*/ NULL, // XXX...
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* Pose Component ========================================= */

/* Initialise 'pose eval' node - from pointer data given */
static void dnti_pose_eval__init_data(DepsNode *node, ID *id)
{
	PoseComponentDepsNode *pcomp = (PoseComponentDepsNode *)node;
	
	/* generic component-node... */
	dnti_component__init_data(node, id);
	
	/* pose-specific data... */
	pcomp->bone_hash = BLI_ghash_str_new("Pose Component Bone Hash"); /* <String, BoneNode> */
}

/* Copy 'component' node */
static void dnti_pose_eval__copy_data(DepsgraphCopyContext *dcc, DepsNode *dst, const DepsNode *src)
{
	const PoseComponentDepsNode *src_node = (const PoseComponentDepsNode *)src;
	PoseComponentDepsNode *dst_node       = (PoseComponentDepsNode *)dst;
	
	/* generic component node... */
	dnti_component__copy_data(dst, src);
	
	/* pose-specific data... */
	// copy bonehash...
}

/* Free 'component' node */
static void dnti_pose_eval__free_data(DepsNode *node)
{
	PoseComponentDepsNode *pcomp = (PoseComponentDepsNode *)node;
	
	/* pose-specific data... */
	BLI_ghash_free(pcomp->bone_hash, NULL, /*dnti_pose_eval__hash_free_bone*/NULL);
	
	/* generic component node... */
	dnti_component__free_data(node);
}

/* Pose Evaluation */
static DepsNodeTypeInfo DNTI_EVAL_POSE = {
	/* type */               DEPSNODE_TYPE_EVAL_POSE,
	/* size */               sizeof(PoseComponentDepsNode),
	/* name */               "Pose Eval Component",
	
	/* init_data() */        dnti_pose_eval__init_data,
	/* free_data() */        dnti_pose_eval__free_data,
	/* copy_data() */        dnti_pose_eval__copy_data,
	
	/* add_to_graph() */     dnti_component__add_to_graph,
	/* remove_from_graph()*/ NULL, // XXX...
	
	/* validate_links() */   NULL // XXX: ensure cleanup ops are first/last and hooked to whatever depends on us
};

/* ******************************************************** */
/* Inner Nodes */


/* ******************************************************** */
/* External API */

/* Global type registry */

/* NOTE: For now, this is a hashtable not array, since the core node types
 * currently do not have contiguous ID values. Using a hash here gives us
 * more flexibility, albeit using more memory and also sacrificing a little
 * speed. Later on, when things stabilise we may turn this back to an array
 * since there are only just a few node types that an array would cope fine...
 */
static GHash *_depsnode_typeinfo_registry = NULL;

/* Registration ------------------------------------------- */

/* Register node type */
void DEG_register_node_typeinfo(DepsNodeTypeInfo *typeinfo)
{
	if (typeinfo) {
		eDepsNode_Type type = typeinfo->type;
		BLI_ghash_insert(_depsnode_typeinfo_registry, SET_INT_IN_POINTER(type), typeinfo);
	}
}


/* Register all node types */
void DEG_register_node_types(void)
{
	/* initialise registry */
	_depsnode_typeinfo_registry = BLI_ghash_int_new("Depsgraph Node Type Registry");
	
	/* register node types */
	/* GENERIC */
	DEG_register_node_typeinfo(DNTI_ROOT);
	DEG_register_node_typeinfo(DNTI_TIMESOURCE);
	DEG_register_node_typeinfo(DNTI_ID_REF);
	
	/* OUTER */
	DEG_register_node_typeinfo(DNTI_PARAMETERS);
	DEG_register_node_typeinfo(DNTI_PROXY);
	DEG_register_node_typeinfo(DNTI_ANIMATION);
	DEG_register_node_typeinfo(DNTI_TRANSFORM);
	DEG_register_node_typeinfo(DNTI_GEOMETRY);
	
	DEG_register_node_typeinfo(DNTI_EVAL_POSE);
	DEG_register_node_typeinfo(DNTI_EVAL_PARTICLES);
	
	/* INNER */
	DEG_register_node_typeinfo(DNTI_OP_PARAMETER);
	DEG_register_node_typeinfo(DNTI_OP_PROXY);
	DEG_register_node_typeinfo(DNTI_OP_ANIMATION);
	DEG_register_node_typeinfo(DNTI_OP_TRANSFORM);
	DEG_register_node_typeinfo(DNTI_OP_GEOMETRY);
	
	DEG_register_node_typeinfo(DNTI_OP_UPDATE);
	DEG_register_node_typeinfo(DNTI_OP_DRIVER);
	
	DEG_register_node_typeinfo(DNTI_OP_BONE);
	DEG_register_node_typeinfo(DNTI_OP_PARTICLE);
	DEG_register_node_typeinfo(DNTI_OP_RIGIDBODY);
}

/* Free registry on exit */
void DEG_free_node_types(void)
{
	BLI_ghash_free(_depsnode_typeinfo_registry, NULL, NULL);
}

/* Getters ------------------------------------------------- */

/* Get typeinfo for specified type */
DepsNodeTypeInfo *DEG_get_node_typeinfo(eDepsNode_Type type)
{
	/* look up type - at worst, it doesn't exist in table yet, and we fail */
	return BLI_ghash_lookup(_depsnode_typeinfo_registry, type);
}

/* Get typeinfo for provided node */
DepsNodeTypeInfo *DEG_node_get_typeinfo(DepsNode *node)
{
	DepsNodeTypeInfo *nti = NULL;
	
	if (node) {
		nti = DEG_get_node_typeinfo(node->type);
	}
	return nti;
}

/* ******************************************************** */
