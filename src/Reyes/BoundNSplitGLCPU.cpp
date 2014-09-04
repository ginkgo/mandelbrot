#include "BoundNSplitGLCPU.h"


#include "PatchIndex.h"
#include "Config.h"
#include "Statistics.h"


using namespace Reyes;


Reyes::BoundNSplitGLCPU::BoundNSplitGLCPU(shared_ptr<PatchIndex>& patch_index)
    : _patch_index(patch_index)
    , _active_handle(nullptr)
{
    _patch_index->enable_retain_vector();
}


void Reyes::BoundNSplitGLCPU::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    _active_handle = patches_handle;
    
    const vector<BezierPatch>& patches = _patch_index->get_patch_vector(_active_handle);
    
    _stack.resize(patches.size());
    for (size_t i = 0; i < patches.size(); ++i) {
        _stack[i] = PatchRange{Bound(0,0,1,1), 0, i};
    }

    _projection = projection;
    
    mat4 proj;
    _projection->calc_projection_with_aspect_correction(proj);
    
    _mvp = proj * matrix;
    _mv = matrix;
}


bool Reyes::BoundNSplitGLCPU::done()
{
    return _stack.size() == 0;
}


void Reyes::BoundNSplitGLCPU::do_bound_n_split(GL::IndirectVBO& vbo)
{
    const vector<BezierPatch>& patches = _patch_index->get_patch_vector(_active_handle);

    assert(vbo.get_max_vertex_count() >= 4);    
    vector<vec4> vertex_data;
    vertex_data.reserve(vbo.get_max_vertex_count());
    
    BBox box;
    float vlen, hlen;

    float s = config.bound_n_split_limit();

    PatchRange r0,r1;
    
    while (!_stack.empty()) {

        PatchRange r = _stack.back();
        _stack.pop_back();


        bound_patch_range(r, patches[r.patch_id], _mv, _mvp, box, vlen, hlen);

        vec2 size;
        bool cull;
        _projection->bound(box, size, cull);

        if (cull) continue;

        if (box.min.z < 0 && size.x < s && size.y < s) {
            
            const Bound& pr = r.range;
            const size_t pid = r.patch_id;
            
            vertex_data.push_back(vec4(pr.min.x, pr.min.y, pid,1));
            vertex_data.push_back(vec4(pr.max.x, pr.min.y, pid,1));
            vertex_data.push_back(vec4(pr.max.x, pr.max.y, pid,1));
            vertex_data.push_back(vec4(pr.min.x, pr.max.y, pid,1));

            if (vertex_data.size() + 4 >= vbo.get_max_vertex_count()) {
                // VBO full, end this batch
                break; 
            }

        } else if (r.depth > config.max_split_depth()) {
            // TODO: Add low-overhead warning mechanism for this
            // cout << "Warning: Split limit reached" << endl
        } else {
            if (vlen < hlen) {
                vsplit_range(r, _stack);
            } else {
                hsplit_range(r, _stack);
            }
        }

    }

    vbo.load_vertices(vertex_data);
    vbo.load_indirection(vertex_data.size(), 1, 0, 0);
    
}




void Reyes::BoundNSplitGLCPU::vsplit_range(const PatchRange& r, vector<PatchRange>& stack)
{
    float cy = (r.range.min.y + r.range.max.y) * 0.5f;

    stack.emplace_back(r.range.min.x, r.range.min.y, r.range.max.x, cy,    r.depth + 1, r.patch_id);
    stack.emplace_back(r.range.min.x, cy, r.range.max.x, r.range.max.y,    r.depth + 1, r.patch_id);
}
    
void Reyes::BoundNSplitGLCPU::hsplit_range(const PatchRange& r, vector<PatchRange>& stack)
{
    float cx = (r.range.min.x + r.range.max.x) * 0.5f;
    
    stack.emplace_back(r.range.min.x, r.range.min.y, cx, r.range.max.y,    r.depth + 1, r.patch_id);   
    stack.emplace_back(cx, r.range.min.y, r.range.max.x, r.range.max.y,    r.depth + 1, r.patch_id); 
}



void Reyes::BoundNSplitGLCPU::bound_patch_range (const PatchRange& r, const BezierPatch& p,
                                                     const mat4& mv, const mat4& mvp,
                                                     BBox& box, float& vlen, float& hlen)
{
    const size_t RES = 3;
        
    //vec2 pp[RES][RES];
    vec3 ps[RES][RES];
    vec3 pos;
     
    box.clear();
        
    for (size_t iu = 0; iu < RES; ++iu) {
        for (size_t iv = 0; iv < RES; ++iv) {
            float v = r.range.min.x + (r.range.max.x - r.range.min.x) * iu * (1.0f / (RES-1));
            float u = r.range.min.y + (r.range.max.y - r.range.min.y) * iv * (1.0f / (RES-1));

            eval_patch(p, u, v, pos);

            vec3 pt = vec3(mv * vec4(pos,1));
                
            box.add_point(pt);

            // pp[iu][iv] = project(mvp * vec4(pos,1));
            ps[iu][iv] = pt;
        }
    }

    vlen = 0;
    hlen = 0;

    for (size_t i = 0; i < RES; ++i) {
        float h = 0, v = 0;
        for (size_t j = 0; j < RES-1; ++j) {
            // v += glm::distance(pp[j][i], pp[j+1][i]);
            // h += glm::distance(pp[i][j], pp[i][j+1]);
            v += glm::distance(ps[j][i], ps[j+1][i]);
            h += glm::distance(ps[i][j], ps[i][j+1]);
        }
        vlen = maximum(v, vlen);
        hlen = maximum(h, hlen);
    }
}
