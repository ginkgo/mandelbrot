/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#include "PatchDrawer.h"

#include "Projection.h"
#include "Config.h"

namespace Reyes
{
    void bound_n_split(const BezierPatch& patch, const Projection& projection,
                       PatchDrawer& patch_drawer)
    {
        BBox box;

        calc_bbox(patch, box);

        vec2 size;
        bool cull;

        projection.bound(box, size, cull);
    
        if (cull) return;

        int s = config.bound_n_split_limit();

        if (box.min.z < 0 && size.x < s && size.y < s) {
	    patch_drawer.draw_patch(patch);
        } else {
            BezierPatch p0, p1;
            mat4 proj;
            projection.calc_projection(proj);
            pisplit_patch(patch, p0, p1, proj);
            bound_n_split(p0, projection, patch_drawer);
            bound_n_split(p1, projection, patch_drawer);
        }
    }
}
