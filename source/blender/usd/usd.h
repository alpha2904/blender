/*
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
 */

/** \file
 * \ingroup USD
 */

#ifndef __BLENDER_USD_H__
#define __BLENDER_USD_H__

#ifdef __cplusplus
extern "C" {
#endif

struct Scene;
struct bContext;

struct USDExportParams {
  bool do_animation;
  bool selected_objects_only;
  bool visible_objects_only;
};

/* The USD_export takes a as_background_job parameter, and returns a boolean.
 *
 * When as_background_job=true, returns false immediately after scheduling
 * a background job.
 *
 * When as_background_job=false, performs the export synchronously, and returns
 * true when the export was ok, and false if there were any errors.
 */

bool USD_export(struct Scene *scene,
                struct bContext *C,
                const char *filepath,
                const struct USDExportParams *params,
                bool as_background_job);

#ifdef __cplusplus
}
#endif

#endif /* __BLENDER_USD_H__ */
