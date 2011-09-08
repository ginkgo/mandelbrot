#include "WireGLRenderer.h"

#include "Projection.h"

#define P(pi,pj) patch.P[pi][pj]

namespace Reyes
{

    void WireGLRenderer::prepare()
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void WireGLRenderer::finish()
    {
        glfwSwapBuffers();
    }

    void WireGLRenderer::set_projection(const Projection& projection)
    {
        mat4 proj;
        projection.calc_projection(proj);
                                         
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(proj));
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void WireGLRenderer::draw_patch(const BezierPatch& patch)
    {
        vec3 p;

        glColor3f(1,1,1);

        glBegin(GL_LINE_LOOP);
    
        int n = 16;

        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(0,0), P(1,0), P(2,0), P(3,0), t, p);
            glVertex3f(p.x, p.y, p.z);
        }
    
        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(3,0), P(3,1), P(3,2), P(3,3), t, p);
            glVertex3f(p.x, p.y, p.z);
        }
    
        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(3,3), P(2,3), P(1,3), P(0,3), t, p);
            glVertex3f(p.x, p.y, p.z);
        }
    
        for (int i = 0; i < n; ++i) {
            float t = float(i)/n;

            eval_spline(P(0,3), P(0,2), P(0,1), P(0,0), t, p);
            glVertex3f(p.x, p.y, p.z);
        }

        glEnd();
    }
}
